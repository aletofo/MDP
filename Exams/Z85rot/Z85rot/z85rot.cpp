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
#include <filesystem>

class PPMimg {
	size_t width_, height_, maxval_;
	std::vector<uint8_t> data_;
	std::ifstream& is_;

public:
	PPMimg(size_t w, size_t h, size_t mv, std::ifstream& is) : width_(w), height_(h), maxval_(mv), is_(is), data_(w * h) {}

	void pad(uint8_t byte) {
		data_.push_back(byte);
	}

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
			pad(0);
		}
	}

	void popdata() {
		data_.pop_back();
	}

	size_t get_size() { return data_.size(); }

	double get_buffer(size_t index) {
		std::string buf;
		unsigned char ch;

		for (size_t i = 0; i < 4; ++i) {
			char buffer[3];
			ch = static_cast<unsigned char>(data_[index + i]);
			std::snprintf(buffer, sizeof(buffer), "%02X", ch);
			buf += buffer;
		}
		
		double val = static_cast<double>(std::stoul(buf, nullptr, 16));
		return val;
	}

	auto rawdata() { return data_.data(); }
	auto rawsize() { return data_.size() * sizeof(uint8_t); }
};

void skip_comments(std::ifstream& is) {
	char ch;
	while (is >> ch) {  // Legge il primo carattere non whitespace
		if (ch == '#') {
			is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Ignora tutto fino a fine riga
		}
		else {
			is.unget(); // Rimettiamo il carattere valido nel flusso
			break;
		}
	}
}

void compress(std::ifstream& is, std::ofstream& os, size_t N) {
	std::string token;
	std::string width, height, maxval;
	size_t w, h, mv;
	std::vector<char> alphabet = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
									'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
									'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
									'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D',
									'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
									'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
									'Y', 'Z', '.', '-', ':', '+', '=', '^', '!', '/',
									'*', '?', '&', '<', '>', '(', ')', '[', ']', '{',
									'}', '@', '%', '$', '#' };

	//header
	is >> token;
	if (token != "P6")
		return;
	skip_comments(is);
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
	int pos = 4, last_pos = 4;

	for (size_t i = 0; i < data_size / 4; ++i) {
		z85[i].resize(5);
		std::fill(z85[i].begin(), z85[i].end(), 0);
		og_value = img.get_buffer(i * 4);
		buffer = og_value;
		pos = 4;
		
		while (1) {
			while (buffer >= 85.0) {
				buffer /= 85.0;
				--pos;
			}
			z85[i][pos] = static_cast<size_t>(buffer);
			buffer = std::floor(buffer);
			while (buffer < og_value) {
				buffer *= 85.0;
			}
			if (buffer == og_value)
				break;
			buffer /= 85.0;
			og_value -= buffer;
			buffer = og_value;
			if (og_value < 85) {
				z85[i][4] = static_cast<size_t>(buffer);
				break;
			}
			
			pos = 4;
		}

	}
	//now let's output the z85 characters considering N rotation
	size_t curr_N = 0;
	size_t tmp;
	for (std::vector<size_t> v : z85) {
		for (size_t index : v) {
			if (curr_N > index) {
				tmp = curr_N - index;
				while (tmp > 85) {
					tmp -= 85;
				}
				os.put(alphabet[85 - tmp]);
			}
			else {
				os.put(alphabet[index - curr_N]);
			}
			curr_N += N;
		}
	}
}

std::map<char, uint32_t> build_map(std::vector<char> v) {
	std::map<char, uint32_t> map;
	uint32_t i = 0;

	for (unsigned char ch : v) {
		map.emplace(ch, i);
		++i;
	}
	return map;
}

void write_bigendian(uint32_t& value, std::ofstream& os) {
	uint8_t bytes[4] = {
	   static_cast<uint8_t>((value >> 24) & 0xFF),  // Byte più significativo
	   static_cast<uint8_t>((value >> 16) & 0xFF),
	   static_cast<uint8_t>((value >> 8) & 0xFF),
	   static_cast<uint8_t>(value & 0xFF)           // Byte meno significativo
	};

	os.write(reinterpret_cast<char*>(bytes), sizeof(bytes));
}

void decompress(std::ifstream& is, std::ofstream& os, size_t N) {
	std::string width, height;
	size_t w, h;
	char ch;
	std::vector<char> alphabet = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
									'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
									'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
									'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D',
									'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
									'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
									'Y', 'Z', '.', '-', ':', '+', '=', '^', '!', '/',
									'*', '?', '&', '<', '>', '(', ')', '[', ']', '{',
									'}', '@', '%', '$', '#' };

	std::map<char, uint32_t> map = build_map(alphabet);
	
	while (is.get(ch)) {
		if (ch == ',') {
			break;
		}
		width.push_back(ch);
	}
	while (is.get(ch)) {
		if (ch == ',') {
			break;
		}
		height.push_back(ch);
	}
	
	w = std::stoul(width);
	h = std::stoul(height);

	os << "P6\n" << width << " " << height << "\n" << 255 << "\n";

	PPMimg img(w * 3, h, 255, is);
	while (img.get_size() % 4 != 0) {
		img.pad(0);
	}
	size_t filesize = img.get_size();
	
	uint32_t total = 0;
	uint32_t byte;
	size_t curr_N = 0, tmp;

	if (N == 0) {
		for (int j = 0; j < filesize / 4; ++j) {

			for (int i = 4; i >= 0; --i) {
				is.get(ch);
				byte = map[ch];

				total += byte * static_cast<uint32_t>(std::pow(85, i));
			}
			write_bigendian(total, os);
			total = 0;
		}
	}
	else {
		for (int j = 0; j < filesize / 4; ++j) {

			for (int i = 4; i >= 0; --i) {
				is.get(ch);
				if (map[ch] + curr_N < 85) {
					byte = map[ch] + curr_N;
				}
				else {
					tmp = curr_N;
					if (tmp > 85) {
						while (tmp > 85) {
							tmp -= 85;
						}
						if (tmp + map[ch] < 85) {
							byte = map[ch] + tmp;
						}
						else {
							tmp = (map[ch] + tmp) - 85;
							byte = tmp;
						}
					}
					else {
						tmp = 85 - map[ch];
						tmp = curr_N - tmp;
						byte = tmp;
					}
				}
				total += byte * static_cast<uint32_t>(std::pow(85, i));
				curr_N += N;
			}
			write_bigendian(total, os);
			total = 0;
		}
	}
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
		std::ofstream os(argv[4], std::ios::binary);
		if (!os)
			return EXIT_FAILURE;
		compress(is, os, N);
	}
	else if (command == "d") {
		std::ifstream is(argv[3], std::ios::binary);
		if (!is)
			return EXIT_FAILURE;
		std::ofstream os(argv[4], std::ios::binary);
		if (!os)
			return EXIT_FAILURE;
		decompress(is, os, N);
	}

	return EXIT_SUCCESS;
}