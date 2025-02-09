#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

class bitreader {
	uint8_t buffer_;
	size_t n_ = 0;
	std::istream& is_;
	uint8_t shifted_buf_;

	uint16_t readbit() {
		if (n_ == 0) {
			buffer_ = is_.get();
			n_ = 8;
		}
		--n_;
		shifted_buf_ = buffer_ << n_;
		shifted_buf_ = shifted_buf_ >> 7;
		return shifted_buf_ & 1;
	}
public:
	bitreader(std::istream& is) : is_(is) {}
	uint16_t operator()(uint8_t numbits) {
		uint16_t u = 0;
		uint8_t nbits = numbits;
		uint8_t maxbits = 15;
		while (numbits-- > 0) {
			u = (u >> 1) | (readbit() << maxbits);
		}
		u = u >> (maxbits - nbits + 1);
		return u;
	}
	bool fail() const {
		return is_.fail();
	}
	operator bool() const {
		return !fail();
	}
};

bool flags(uint8_t SDflags, uint8_t& CRbits, uint8_t& bpp) {
	//bpp
	uint8_t tmp = SDflags << 5;
	bpp = tmp >> 5;
	//CRbits
	tmp = SDflags << 1;
	CRbits = tmp >> 5;

	++CRbits;
	++bpp;

	if (SDflags >> 7 == 1)
		return true;
	else return false;
}

void write_pixels(std::ofstream& os, std::map<uint8_t, std::vector<uint8_t>> colormap, std::map<uint16_t, 
	std::vector<uint16_t>> dict, std::vector<uint16_t> last_seq) 
{
	std::vector<uint8_t> rgb;
	for (uint16_t index : last_seq) {
		rgb = colormap[index];
		for (uint8_t byte : rgb) {
			os.put(byte);
		}
	}
}

void decompress(std::ifstream& is, std::ofstream& os) {
	char ch;
	std::string token;
	//header
	for (int i = 0; i < 6; ++i) {
		is.get(ch);
		token.push_back(ch);
	}
	if (token != "GIF87a")
		return;
	
	uint16_t width, height;
	is.read(reinterpret_cast<char*>(&width), sizeof(width));
	is.read(reinterpret_cast<char*>(&height), sizeof(height));

	uint8_t SDflags;
	is.read(reinterpret_cast<char*>(&SDflags), sizeof(SDflags));

	uint8_t GCMindex;
	is.read(reinterpret_cast<char*>(&GCMindex), sizeof(GCMindex));

	is.get(ch);

	uint8_t CRbits = 0, bpp = 0;
	std::map<uint8_t, std::vector<uint8_t>> colormap;

	if (flags(SDflags, CRbits, bpp)) {
		int num_colors = static_cast<int>(std::pow(2, bpp));
		
		for (int i = 0; i < num_colors; ++i) {
			std::vector<uint8_t> rgb;
			for (int channel = 0; channel < 3; ++channel) {
				is.get(ch);
				uint8_t byte = static_cast<uint8_t>(ch);
				rgb.push_back(byte);
			}
			colormap.emplace(i, rgb);
		}
	}
	//Image Descriptor
	is.get(ch);
	if (ch != ',') //separator
		return;

	uint16_t x, y, w, h; //in this exercise there will be just 1 Imgae Desc. that covers all the image
	is.read(reinterpret_cast<char*>(&x), sizeof(x));
	is.read(reinterpret_cast<char*>(&y), sizeof(y));
	is.read(reinterpret_cast<char*>(&w), sizeof(w));
	is.read(reinterpret_cast<char*>(&h), sizeof(h));

	uint8_t IDflags;
	is.read(reinterpret_cast<char*>(&IDflags), sizeof(IDflags));

	//build the PPM header
	os << "P6\n" << w << " " << h << " " << 255 << "\n";

	//Data Rasters
	std::map<uint16_t, std::vector<uint16_t>> dict;

	uint8_t codesize;
	is.read(reinterpret_cast<char*>(&codesize), sizeof(codesize));
	uint16_t CLEARcode = static_cast<uint16_t>(std::pow(2, codesize));
	uint16_t EOI = CLEARcode + 1;
	uint16_t dict_index = CLEARcode + 2;

	bitreader br(is);
	uint16_t raster_end = 1;

	while (raster_end != 0x3B00) {
		uint8_t size = codesize;
		uint8_t BBcount;
		is.read(reinterpret_cast<char*>(&BBcount), sizeof(BBcount));
		size_t bit_i = 0, BBcount_bits = BBcount * 8;
		std::vector<uint16_t> last_seq;

		while (bit_i <= BBcount_bits) {
			//add a bit to the the code size
			if (dict_index > (std::pow(2, size))) {
				if(size + 1 <= 12)
					++size;
				else {
					uint16_t color_index = br(static_cast<int>(size));
					if (color_index != CLEARcode) {
						std::cout << "stream ERROR\n";
						return;
					}
					else {
						dict.clear();
						for (uint16_t i = 0; i < CLEARcode + 2; ++i) {
							std::vector<uint16_t> v;
							v.push_back(i);
							dict.emplace(i, v);
						}
						dict_index = CLEARcode + 2;
						bit_i += size;
						continue;
					}
				}
			}
			if (bit_i == BBcount_bits) {
				break;
			}
			//read the code
			uint16_t color_index = br(static_cast<int>(size));
			last_seq.push_back(color_index);
			bit_i += size;
			//check if it's CLEAR code or EOI
			if (color_index == EOI) {
				//break;
				continue;
			}
			if (color_index == CLEARcode) {
				dict.clear();
				for (uint16_t i = 0; i < CLEARcode + 2; ++i) {
					std::vector<uint16_t> v;
					v.push_back(i);
					dict.emplace(i, v);
				}
				dict_index = CLEARcode + 2;
			}
			else {
				if (color_index < 256) {
					last_seq.push_back(color_index);
					dict.emplace(dict_index, last_seq);
				}
				else {
					std::vector<uint16_t> v = dict[color_index];
					for (uint16_t x : v) {
						last_seq.push_back(x);
					}
					dict.emplace(dict_index, last_seq);
				}
				write_pixels(os, colormap, dict, last_seq);
				last_seq.clear();
				last_seq.push_back(color_index);
				++dict_index;
			}

			auto curpos = is.tellg();
			is.read(reinterpret_cast<char*>(&raster_end), sizeof(raster_end));
			is.seekg(curpos);
			if (raster_end == 0x3B00) { //end
				break;
			}
		}
	}
	std::cout << is.tellg();
}

int main(int argc, char* argv[]) {
	if (argc != 3)
		return EXIT_FAILURE;
	std::ifstream is(argv[1], std::ios::binary);
	if (!is)
		return EXIT_FAILURE;
	std::ofstream os(argv[2], std::ios::binary);
	if(!os)
		return EXIT_FAILURE;

	decompress(is, os);

	return EXIT_SUCCESS;
}