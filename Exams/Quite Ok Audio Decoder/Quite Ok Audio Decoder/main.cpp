#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <iostream>

class bitreader {
	uint8_t buffer_ = 0;
	int n_ = 0;
	int counter_ = 0;
	char curr_ch;
	uint8_t curr_x;
	std::ifstream& is_;

	void readbit(uint8_t curbit) {
		buffer_ = (buffer_ << 1) | (curbit & 1);
		++n_;
		if (n_ == 8) {
			n_ = 0;
		}
	}

public:
	bitreader(std::ifstream& is) : is_(is) {}

	uint8_t operator() (int numbits) {
		uint8_t value = 0;
		if (n_ == 0) {
			is_.get(curr_ch);
			curr_x = static_cast<uint8_t>(curr_ch);
		}
		while (counter_ != numbits) {
			for (int bitnum = 7 - n_; bitnum >= 0; --bitnum) {
				readbit(curr_x >> bitnum);
				++counter_;
				if (counter_ == numbits) {
					value = buffer_;
					buffer_ = 0;
					break;
				}
				else {
					if (n_ == 0) {
						is_.get(curr_ch);
						curr_x = static_cast<uint8_t>(curr_ch);
					}
				}
			}
		}
		counter_ = 0;

		return value;
	}
};

std::string BEread(std::ifstream& is, int count, std::string token) {
	char ch;
	token.clear();

	for (int i = 0; i < count; ++i) {
		is.get(ch);
		token.push_back(ch);
	}

	std::string hexstring;
	for (unsigned char uch : token) {
		char buffer[3];
		std::snprintf(buffer, sizeof(buffer), "%02X", uch);
		hexstring += buffer;
	}

	return hexstring;
}

struct framehdr {
	uint8_t num_channels_;
	uint32_t samplerate_;
	uint16_t fsamples_;
	uint16_t fsize_; //include the framehdr size

	framehdr(uint8_t nc, uint32_t sr, uint16_t fsam, uint16_t fsize) : num_channels_(nc), samplerate_(sr), fsamples_(fsam), fsize_(fsize){}

};

struct lms_state {
	std::vector<int16_t> history_;
	std::vector<int16_t> weights_;

	lms_state(){}

	void loadH(std::ifstream& is) {
		int16_t h;
		std::string token;
		for (int i = 0; i < 4; ++i) {
			token = BEread(is, 2, token);
			h = std::stoi(token, nullptr, 16);
			history_.push_back(h);
		}
	}
	void loadW(std::ifstream& is) {
		int16_t w;
		std::string token;
		for (int i = 0; i < 4; ++i) {
			token = BEread(is, 2, token);
			w = std::stoi(token, nullptr, 16);
			weights_.push_back(w);
		}
	}
};

struct frame {
	framehdr hdr_;
	std::vector<lms_state> states_;
	std::vector<uint8_t> slices_;
	std::ifstream& is_;

	frame(framehdr hdr, std::vector<lms_state> s, std::ifstream& is) : hdr_(hdr), states_(s), is_(is) {
		slices_.resize(256);
	}

};

void writeLittleEndian(std::ofstream& os, int16_t value) {
	uint8_t bytes[2] = {
		static_cast<uint8_t>((value << 8) & 0xFF),
		static_cast<uint8_t>((value >> 8) & 0xFF)
	};
	os.write(reinterpret_cast<char*>(bytes), 2);
}

std::vector<uint16_t> readslice(std::ifstream& is, std::ofstream& os, lms_state& state) {
	bitreader br(is);
	std::vector<uint16_t> slice;

	uint8_t sf_quant = br(4);
	double sf = std::round(std::pow(sf_quant + 1, 2.75));

	std::vector<double> dequant_tab = { 0.75, -0.75, 2.5, -2.5, 4.5, -4.5, 7, -7 };

	for (int i = 0; i < 20; ++i) {
		uint8_t qr = br(3);
		double r = sf * dequant_tab[qr]; //residual
		if (r < 0) {
			r = std::ceil(r - 0.5);
		}
		else {
			r = std::floor(r + 0.5);
		}
		int ri = static_cast<int>(r);

		int p = 0; //predicted sample
		for (int n = 0; n < 4; ++n) {
			p += state.history_[n] * state.weights_[n];
		}
		p >>= 13;
		//final sample
		int final_s = p + ri;
		if (final_s > 32767) {
			final_s = 32767;
		}
		else if (final_s < -32768) {
			final_s = -32768;
		}
		int16_t s = static_cast<int16_t>(final_s);
		//update the weights
		int delta = ri >> 4;
		for (int n = 0; n < 4; ++n) {
			if (state.history_[n] < 0) {
				state.weights_[n] += -delta;
			}
			else {
				state.weights_[n] += delta;
			}
		}
		//update the history
		for (int n = 0; n < 3; n++) {
			state.history_[n] = state.history_[n + 1];
		}
		state.history_[3] = s;

		//os.write(reinterpret_cast<char*>(&s), sizeof(s));
		slice.push_back(s);
	}
	return slice;
}

void alternate_write(std::ofstream& os, std::vector<std::vector<uint16_t>> slices) {

	for (int i = 0; i < 20; ++i) {
		for (std::vector<uint16_t> s : slices) {
			uint16_t x = s[i];
			os.write(reinterpret_cast<char*>(&x), sizeof(x));
		}
	}
	
}

void decompress(std::ifstream& is, std::ofstream& os) {
	std::string token;
	char magic[4];
	uint32_t samples;

	//filehdr
	is.read(magic, sizeof(magic));
	for (unsigned char ch : magic) {
		token.push_back(ch);
	}
	if (token != "qoaf")
		return;
	token = BEread(is, 4, token);
	samples = std::stoul(token, nullptr, 16); //953586 
	//endfilehdr
	int nframes = static_cast<int>(std::ceil(samples / (256 * 20)));

	for (int i = 0; i < nframes; ++i) { //for each frame(quanti sono???)
		//framehdr
		uint8_t num_channels;
		is.read(reinterpret_cast<char*>(&num_channels), sizeof(num_channels));
		token = BEread(is, 3, token);
		uint32_t samplerate = std::stoul(token, nullptr, 16);
		token = BEread(is, 2, token);
		uint16_t fsamples = static_cast<uint16_t>(std::stoul(token, nullptr, 16)); //nel primo frame ho 5120 (256*20)
		token = BEread(is, 2, token);
		uint16_t fsize = static_cast<uint16_t>(std::stoul(token, nullptr, 16));

		framehdr hdr(num_channels, samplerate, fsamples, fsize);
		std::vector<lms_state> states;
		states.resize(num_channels);

		for (lms_state& s : states) { //for each channel
			s.loadH(is);
			s.loadW(is);
		}

		uint16_t curpos = 8 + num_channels * 16; //read bytes since the beginning of the frame
		std::vector<std::vector<uint16_t>> slices;
		std::vector<uint16_t> slice;
		//until end of frame is reached
		for (int s = 0; s < 256; ++s) { //for each slice(they are 256)
			for (int c = 0; c < num_channels; ++c) {
				slice = readslice(is, os, states[c]);
				slices.push_back(slice);
			}
			alternate_write(os, slices);
			slices.clear();
		}
	}
}

void WAVhdr(std::ofstream& os) {
	os << "RIFF";
	int32_t size = 3814380;
	os.write(reinterpret_cast<char*>(&size), sizeof(size));
	os << "WAVE" << "fmt ";
	int32_t len = 16;
	os.write(reinterpret_cast<char*>(&len), sizeof(len));
	int16_t format = 1, channels = 2;
	os.write(reinterpret_cast<char*>(&format), sizeof(format));
	os.write(reinterpret_cast<char*>(&channels), sizeof(channels));
	int32_t hz = 44100;
	os.write(reinterpret_cast<char*>(&hz), sizeof(hz));
	int32_t sr = 176400;
	os.write(reinterpret_cast<char*>(&sr), sizeof(sr));
	int16_t bpsc = 4, bps = 16;
	os.write(reinterpret_cast<char*>(&bpsc), sizeof(bpsc));
	os.write(reinterpret_cast<char*>(&bps), sizeof(bps));
	os << "data";
	int32_t datasize = 3814344;
	os.write(reinterpret_cast<char*>(&datasize), sizeof(datasize));
}

int main(int argc, char* argv[]) {
	if (argc != 3)
		return EXIT_FAILURE;
	std::ifstream is(argv[1], std::ios::binary);
	if (!is)
		return EXIT_FAILURE;
	std::ofstream os(argv[2], std::ios::binary);
	if (!os)
		return EXIT_FAILURE;

	WAVhdr(os);

	decompress(is, os);

	return EXIT_SUCCESS;
}