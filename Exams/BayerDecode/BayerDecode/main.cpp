#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

class PGM {
	size_t width_, height_, maxval_;
	std::vector<uint8_t> data_;
	std::ifstream& is_;

public:
	PGM(size_t w, size_t h, size_t maxval, std::ifstream& is) : width_(w), height_(h), maxval_(maxval), is_(is){
		if (maxval > 255) {
			data_.resize(w * 2 * h);
		}
		else {
			data_.resize(w * h);
		}
	}

	size_t w() { return width_; }
	size_t h() { return height_; }
	size_t size() { return height_ * width_; }
	std::vector<uint8_t> get_data() { return data_; }

	void fill_data(this auto&& self) {
		char ch;
		uint8_t byte;

		for (size_t r = 0; r < self.height_; ++r) {
			for (size_t c = 0; c < self.width_ * 2; ++c) {
				self.is_.get(ch);
				byte = static_cast<uint8_t>(ch);

				self.data_[r * (self.width_ * 2) + c] = byte;
			}
		}
	}
	
	void insert_byte(this auto&& self, size_t r, size_t c, uint8_t byte) {
		self.data_[r * self.width_ + c] = byte;
	}

	void insert_rgb(this auto&& self, size_t r, size_t c, char channel, uint8_t byte) {
		if (channel == 'r') {
			self.data_[r * self.w() + c] = byte;
			self.data_[r * self.w() + c + 1] = 0;
			self.data_[r * self.w() + c + 2] = 0;
		}
		if (channel == 'g') {
			self.data_[r * self.w() + c] = 0;
			self.data_[r * self.w() + c + 1] = byte;
			self.data_[r * self.w() + c + 2] = 0;
		}
		if (channel == 'b') {
			self.data_[r * self.w() + c] = 0;
			self.data_[r * self.w() + c + 1] = 0;
			self.data_[r * self.w() + c + 2] = byte;
		}
	}

	auto rawdata() { return reinterpret_cast<char*>(data_.data()); }
	auto rawsize() { return data_.size(); }
};

struct bayer_pattern {
	size_t width_, height_;
	std::vector<char> data_;

	bayer_pattern(size_t w, size_t h) : width_(w), height_(h), data_(w * h) {
		for (size_t r = 0; r < h; ++r) {
			for (size_t c = 0; c < w; ++c) {
				if (r == 0 || r % 2 == 0) {
					if (c == 0 || c % 2 == 0) {
						data_[r * w + c] = 'r';
					}
					else {
						data_[r * w + c] = 'g';
					}
				}
				else {
					if (c == 0 || c % 2 == 0) {
						data_[r * w + c] = 'b';
					}
					else {
						data_[r * w + c] = 'g';
					}
				}
			}
		}
			
	}

	auto get_data() { return data_; }
};

uint16_t swap_endian(uint16_t value) {
	return (value >> 8) | (value << 8);
}

PGM PGMheader(std::ifstream& is) {
	std::string token;
	size_t w, h, maxval;
	PGM failure(0, 0, 0, is);

	is >> token;
	if (token != "P5")
		return failure;
	is >> token;
	w = std::stoul(token);
	is >> token;
	h = std::stoul(token);
	is >> token;
	maxval = std::stoul(token);
	if (maxval != 65535)
		return failure;

	is.get(); //0A non letto prima del bytestream

	PGM input_img(w, h, maxval, is);
	return input_img;
}

void PGM_gray(std::ifstream& is, std::ofstream& os, PGM& img) {
	os << "P5 " << img.w() << " " << img.h() << " " << "255\n";

	//char ch;
	uint16_t val;
	uint8_t byte;

	for (size_t r = 0; r < img.h(); ++r) {
		for (size_t c = 0; c < img.w(); ++c) {
			is.read(reinterpret_cast<char*>(&val), sizeof(val));
			val = swap_endian(val);
			byte = val / 256;
			img.insert_byte(r, c, byte);
		}
	}

	os.write(img.rawdata(), img.rawsize());
}

void PGM_bayer(std::ifstream& is, std::ofstream& os, PGM& bayer_img, PGM& gray_img, std::vector<char> pattern) {
	os << "P6 " << gray_img.w() << " " << gray_img.h() << " " << "255\n";

	char channel;
	uint8_t byte;
	std::vector<uint8_t> gray_data = gray_img.get_data();
	size_t bayer_c = 0;

	for (size_t r = 0; r < gray_img.h(); ++r) {
		for (size_t c = 0; c < gray_img.w(); ++c) {
			channel = pattern[r * gray_img.w() + c];
			byte = gray_data[r * gray_img.w() + c];

			bayer_img.insert_rgb(r, bayer_c, channel, byte);
			bayer_c += 3;
		}
		bayer_c = 0;
	}

	os.write(bayer_img.rawdata(), bayer_img.rawsize());
}

int main(int argc, char* argv[]) {
	if (argc != 3)
		return EXIT_FAILURE;
	std::ifstream is(argv[1], std::ios::binary);
	if (!is)
		return EXIT_FAILURE;
	std::string prefix = argv[2];
	
	PGM input_img = PGMheader(is);

	//bayer pattern
	bayer_pattern bp(input_img.w(), input_img.h());
	std::vector<char> pattern = bp.get_data();

	//gray image
	std::string out = prefix + "_gray.pgm";
	std::ofstream os1(out, std::ios::binary);
	if (!os1)
		return EXIT_FAILURE;
	PGM out_gray(input_img.w(), input_img.h(), 255, is);
	PGM_gray(is, os1, out_gray);
	out.clear();

	//first bayer reconstruction
	out = prefix + "_bayer.ppm";
	std::ofstream os2(out, std::ios::binary);
	if (!os2)
		return EXIT_FAILURE;
	PGM out_bayer(input_img.w() * 3, input_img.h(), 255, is);
	PGM_bayer(is, os2, out_bayer, out_gray, pattern);
	out.clear();

	return EXIT_SUCCESS;
}