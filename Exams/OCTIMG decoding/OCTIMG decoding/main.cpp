#include <cstdint>
#include <cmath>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <ranges>
#include <algorithm>
#include <iterator>
#include <iostream>

struct bitreader {
	uint64_t buffer_;
	int n_ = 0;
	std::ifstream& is_;

	uint64_t readbit() {
		char ch;
		if (n_ == 0) {
			is_.get(ch);
			buffer_ = static_cast<uint8_t>(ch);
			n_ = 8;
		}
		--n_;
		return (buffer_ >> n_) & 1;
	}

	bitreader(std::ifstream& is) : is_(is) {}

	uint64_t operator() (int numbits) {
		uint64_t x = 0;
		while (--numbits >= 0) {
			x = (x << 1) | readbit();
		}
		return x;
	}

	void reset(int start_n, std::streampos startpos) {
		char ch;

		is_.seekg(static_cast<int>(startpos) - 1);
		is_.get(ch);
		buffer_ = static_cast<uint8_t>(ch);
		n_ = start_n;
	}
	int get_n() { return n_; }
};

template <typename T>
void readBE(std::ifstream& is, T& value) {
	std::string token;
	char ch;

	for (int i = 0; i < sizeof(T); ++i) {
		is.get(ch);
		token.push_back(ch);
	}
	std::string hexstring;
	for (unsigned char uch : token) {
		char buffer[3];
		std::snprintf(buffer, sizeof(buffer), "%02X", uch);
		hexstring += buffer;
	}
	value = std::stoul(hexstring, nullptr, 16);
}

template <typename T>
std::unordered_map<T, std::tuple<uint8_t, uint8_t, uint8_t>> huftable(std::ifstream& is, uint16_t num_elem, bitreader& br) 
{ 
	//8bit R val - 8bit G/B val (they are always the same) - 5bit len value
	uint8_t R, GB, len;
	std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> table;
	std::tuple<uint8_t, uint8_t, uint8_t> entry;

	for (int i = 0; i < num_elem; ++i) {
		R = static_cast<uint8_t>(br(8));
		GB = static_cast<uint8_t>(br(8));
		len = static_cast<uint8_t>(br(5));
		entry = std::make_tuple(len, R, GB);
		table.push_back(entry);
	}
	std::ranges::sort(table);
	//now create the canonical codes
	std::unordered_map<T, std::tuple<uint8_t, uint8_t, uint8_t>> hufmap;

	size_t curlen = 0;
	T curcode = 0;
	for (auto& [l, r, gb] : table) {
		size_t addbits = l - curlen;
		curcode <<= addbits;
		entry = std::make_tuple(l, r, gb);
		hufmap.emplace(curcode, entry);
		curlen = l;
		curcode++;
	}

	return hufmap;
}

void build_img(std::ifstream& is, std::ofstream& os, std::unordered_map<size_t, std::tuple<uint8_t, uint8_t, uint8_t>> hufmap, 
	bitreader& br, int pixels) 
{
	uint64_t code;
	uint8_t len = 1;

	std::tuple<uint8_t, uint8_t, uint8_t> rgb;
	for (int i = 0; i < pixels; ++i) {
		auto startpos = is.tellg();
		int start_n = br.get_n();
		code = br(len);

		while (!hufmap.contains(code)) {
			br.reset(start_n, startpos);
			++len;
			code = br(len);
		}
		rgb = std::make_tuple(std::get<1>(hufmap[code]), std::get<2>(hufmap[code]), std::get<2>(hufmap[code])); //r-gb-gb
		os << std::get<0>(rgb) << std::get<1>(rgb) << std::get<2>(rgb);
		len = 1;
	}
}

void decompress(std::ifstream& is, std::string prefix) {
	std::string token;
	char ch;

	for (int i = 0; i < 6; ++i) {
		is.get(ch);
		token.push_back(ch);
	}
	if (token != "OCTIMG")
		return;

	uint32_t width, height;
	readBE<uint32_t>(is, width);
	readBE<uint32_t>(is, height);

	bitreader br(is);

	uint16_t num_img = br(10);
	uint16_t num_elem = br(16);

	std::unordered_map<size_t, std::tuple<uint8_t, uint8_t, uint8_t>> hufmap = huftable<size_t>(is, num_elem, br);
	//now the data bits
	int pixels = width * height; //num of pixels I need to read for every image
	for (int i = 0; i < num_img; ++i) {
		std::string filename = prefix + std::to_string(i) + ".ppm";
		std::ofstream os(filename, std::ios::binary);
		os << "P6\n" << width << " " << height << " " << 255 << "\n"; //header
		build_img(is, os, hufmap, br, pixels);
	}
}

int main(int argc, char* argv[]) {
	if (argc != 3)
		return EXIT_FAILURE;
	std::ifstream is(argv[1], std::ios::binary);
	if (!is)
		return EXIT_FAILURE;

	std::string prefix = argv[2];

	decompress(is, prefix);

	return EXIT_SUCCESS;
}