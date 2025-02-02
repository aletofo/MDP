#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>

class PPMimg {
	size_t width_, height_, maxval_;
	std::vector<uint8_t> data_;
	std::ifstream& is_;

public:
	PPMimg(size_t w, size_t h, size_t mv, std::ifstream& is) : width_(w), height_(h), maxval_(mv), is_(is), data_(w * h) {}

	void filldata() {
		char ch;
		uint8_t byte;

		for (size_t r = 0; r < height_; ++r) {
			for (size_t c = 0; c < width_; ++c) {
				is_.get(ch);
				byte = static_cast<uint8_t>(ch);

				data_[r * width_ + c] = byte;
			}
		}

		while (data_.size() % 4 != 0) {
			data_.push_back(0);
		}
	}

	size_t get_size() { return data_.size(); }

	double get_buffer(size_t index) {
		std::string buf;
		char ch;

		for (size_t i = 0; i < 4; ++i) {
			char buffer[3];
			ch = static_cast<char>(data_[index + i]);
			std::snprintf(buffer, sizeof(buffer), "%02X", ch);
			buf += buffer;
		}
		
		double val = static_cast<double>(std::stoul(buf, nullptr, 16));
		return val;
	}

	auto rawdata() { return data_.data(); }
	auto rawsize() { return data_.size() * sizeof(uint8_t); }
};

void compress(std::ifstream& is, std::ofstream& os, size_t N) {
	std::string token;
	std::string width, height, maxval;
	size_t w, h, mv;

	//header
	is >> token;
	if (token != "P6")
		return;
	is >> width;
	is >> height;
	is >> maxval;

	w = std::stoul(width);	
	h = std::stoul(height);	
	mv = std::stoul(maxval);
	is.get(); //last 0A byte before the data stream
	//end header

	//data stream
	PPMimg img(w * 3, h, mv, is);
	img.filldata();

	os << width << "," << height << ",";

	size_t data_size = img.get_size();
	double buffer, og_value;
	std::vector<std::vector<size_t>> z85;
	z85.resize(data_size / 4);
	int counter = 4;

	for (size_t i = 0; i < data_size / 4; ++i) {
		z85[i].resize(5);
		std::fill(z85[i].begin(), z85[i].end(), 0);
		og_value = img.get_buffer(i * 4);
		buffer = og_value;
		while (og_value > 85.0) {
			while (buffer > 85.0) {
				buffer /= 85.0;
				--counter;
			}
			z85[i][counter] = static_cast<size_t>(buffer);
			counter = 4;
			buffer = std::floor(buffer);
			while (buffer < og_value) {
				buffer *= 85.0;
			}
			if (buffer == og_value)
				break;
			buffer /= 85.0;
			og_value -= buffer;
			buffer = og_value;
		}
	}
	//now let's output the z85 characters considering N rotation
	std::vector<char> alphabet = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
									'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
									'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
									'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D',
									'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
									'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
									'Y', 'Z', '.', '-', ':', '+', '=', '^', '!', '/',
									'*', '?', '&', '<', '>', '(', ')', '[', ']', '{',
									'}', '@', '%', '$', '#' };
	size_t curr_N = 0;
	size_t tmp;
	for (std::vector<size_t> v : z85) {
		for (size_t index : v) {
			if (curr_N > index) {
				tmp = curr_N;
				while (tmp > 85) {
					tmp = curr_N - 85;
				}
				os << alphabet[85 - tmp + index];
			}
			else {
				os << alphabet[index - curr_N];
			}
			curr_N += N;
		}
	}
}

void decompress(std::ifstream& is, std::ofstream& os, size_t N) {

}

int main(int argc, char* argv[]) {
	if (argc != 5) {
		return EXIT_FAILURE;
	}
	std::string command = argv[1];
	size_t N = std::stoul(argv[2]);
	if (command == "c") {
		std::ifstream is(argv[3], std::ios::binary);
		if (!is)
			return EXIT_FAILURE;
		std::ofstream os(argv[4]);
		if (!os)
			return EXIT_FAILURE;
		compress(is, os, N);
	}
	else if (command == "d") {
		std::ifstream is(argv[3]);
		if (!is)
			return EXIT_FAILURE;
		std::ofstream os(argv[4], std::ios::binary);
		if (!os)
			return EXIT_FAILURE;
		decompress(is, os, N);
	}

	return EXIT_SUCCESS;
}