#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <map>

struct PPM {
	size_t width_, height_;
	std::vector<uint8_t> data_;
	std::string pixel_;

	PPM(size_t w, size_t h, std::string pixel) : width_(w), height_(h), pixel_(pixel) {}

	void push_pixel(std::vector<uint8_t> rgb) {
		for (int i = 0; i < 3; ++i) {
			data_.push_back(rgb[i]);
		}
	}

	void string2data() {
		std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		std::map<char, uint32_t> base64;

		for (uint32_t i = 0; i < alphabet.size(); ++i) {
			base64[alphabet[i]] = i;
		}

		std::string fourtet;
		std::vector<uint8_t> rgb;
		uint32_t buffer = 0;
		for (int i = 0; i <= pixel_.size(); ++i) {
			if (i % 4 == 0 && i != 0) {

				for (char ch : fourtet) {
					uint32_t x = base64[ch];
					x <<= 2;
					buffer <<= 8;
					buffer += x;
					buffer >>= 2;
				}
				uint32_t byte = buffer;
				byte >>= 16;
				rgb.push_back(static_cast<uint8_t>(byte));
				byte = buffer;
				byte <<= 16;
				byte >>= 24;
				rgb.push_back(static_cast<uint8_t>(byte));
				byte = buffer;
				byte <<= 24;
				byte >>= 24;
				rgb.push_back(static_cast<uint8_t>(byte));

				push_pixel(rgb);
				
				fourtet.clear();
				rgb.clear();
			}
			fourtet.push_back(pixel_[i]);
		}
	}

	auto rawdata() { return reinterpret_cast<char*>(data_.data()); }
	auto rawsize() { return data_.size(); }
};

bool hidden(std::ifstream& is) {
	char ch;
	bool hid = true;

	while (is.get(ch)) {
		if (ch == '"') {
			is.get(ch);
			if (ch == 't') {
				hid = true;
			}
			else {
				hid = false;
			}
			break;
		}
	}
	std::string ignore;
	is >> ignore;

	return hid;
}

void end_object(std::ifstream& is, std::string obj) {
	std::string token = "";
	while (token != obj) {
		is >> token;
	}
	is >> token;
	if (token != "end") {
		std::cout << "Something wrong, not end of the object";
	}
}

PPM build_image(std::ifstream& is) {
	std::string token;
	size_t w = 0, h = 0;
	std::string pixel = "";
	char ch;

	is >> token; //==obj

	while (is >> token) {
		if (token == "width") {
			while (is.get(ch)) {
				if (ch == '"') {
					is >> token;
					token.pop_back();
					w = std::stoi(token);
					break;
				}
			}
		}
		if (token == "height") {
			while (is.get(ch)) {
				if (ch == '"') {
					is >> token;
					token.pop_back();
					h = std::stoi(token);
					break;
				}
			}
		}
		if (token == "pixel") {
			while (is.get(ch)) {
				if (ch == '"') {
					std::string pix;
					while (is.get(ch)) {
						if (ch == '"') {
							break;
						}
						if (std::isspace(ch)) {
							continue;
						}
						pix.push_back(ch);
					}
					pixel = pix;
					is.get(ch);
					break;
				}
			}
			break;
		}
	}
	PPM img(w, h, pixel);
	return img;
}

std::vector<PPM> find_images(std::ifstream& is) {
	std::string curobj, cur_token, last_token;
	is >> curobj;
	last_token = curobj;
	auto startpos = is.tellg();
	std::vector<PPM> images;

	while (is >> cur_token) {
		
		if (cur_token == "obj") {
			curobj = last_token;
		}
		if (cur_token == "hidden") {
			if (hidden(is)) {
				end_object(is, curobj); //read until the end of the object as it's hidden
			}
		}
		if (cur_token == "image") {
			bool hid = false;
			while (cur_token != "data") {
				is >> cur_token;
				if (cur_token == "hidden") {
					hid = hidden(is);
					if (hid) {
						end_object(is, "image");
						break;
					}
				}
			}
			if (!hid) {
				PPM img = build_image(is);
				images.push_back(img);
				end_object(is, "image");
			}
		}

		last_token = cur_token;
	}

	for (PPM& img : images) {
		img.string2data();
	}

	return images;
}

void decompress(std::ifstream& is, std::string prefix) {
	std::vector<PPM> images = find_images(is);

	int counter = 1;
	for (PPM& img : images) {
		std::string filename = prefix + std::to_string(counter) + ".ppm";
		std::ofstream os(filename, std::ios::binary);
		os << "P6\n" << img.width_ << " " << img.height_ << " " << 255 << "\n";
		os.write(img.rawdata(), img.rawsize());
	}
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		return EXIT_FAILURE;
	}
	std::ifstream is(argv[1]/*, std::ios::binary*/);
	if (!is) {
		return EXIT_FAILURE;
	}
	std::string prefix = argv[2];

	decompress(is, prefix);

	return EXIT_SUCCESS;
}