#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <cmath>

class HDRmat {
	size_t rows_, cols_;
	std::vector<float> data_;
	std::ifstream& is_;
	std::ofstream& os_;

public:

	HDRmat(size_t r, size_t c, std::ifstream& is, std::ofstream& os) : rows_(r), cols_(c), is_(is), os_(os) {}

	size_t rows() { return rows_; }
	size_t cols() { return cols_; }
	size_t size() { return cols_ * rows_ * 3; }

	void push_data(this auto&& self, float byte) {
		self.data_.push_back(byte);
	}
	float get_data(this auto&& self, size_t index) {
		return self.data_[index];
	}


};

uint8_t readL(std::ifstream& is) {
	char ch;
	is.get(ch);
	uint8_t L = static_cast<uint8_t>(ch);

	return L;
}

void copy(std::ifstream& is, std::ofstream& os, uint8_t L, std::vector<uint8_t>& v) {
	char ch;
	uint8_t x;

	for (uint8_t i = 0; i < L; ++i) {
		is.get(ch);
		x = static_cast<uint8_t>(ch);
		v.push_back(x);
	}
}

void run(std::ifstream& is, std::ofstream& os, uint8_t L, std::vector<uint8_t>& v) {
	char ch;
	uint8_t x;

	is.get(ch);
	x = static_cast<uint8_t>(ch);

	for (uint8_t i = 0; i < L; ++i) {
		v.push_back(x);
	}
}


void newRLE(std::ifstream& is, std::ofstream& os, HDRmat& hdr, float& min, float& max) {

	int counter = 0; //I need to count the characters I read considering the RLE encoding. I will count 'cols' * 4 bytes for each row (?)
	uint8_t L;
	std::vector<uint8_t> RGBE;


	//R
	while (counter < hdr.cols()) {
		L = readL(is);

		if (L <= 127) {
			counter += L;
			copy(is, os, L, RGBE);
		}
		else {
			counter += L - 128;
			run(is, os, L - 128, RGBE);
		}
	}
	counter = 0;
	//G
	while (counter < hdr.cols()) {
		L = readL(is);

		if (L <= 127) {
			counter += L;
			copy(is, os, L, RGBE);
		}
		else {
			counter += L - 128;
			run(is, os, L - 128, RGBE);
		}
	}
	counter = 0;
	//B
	while (counter < hdr.cols()) {
		L = readL(is);

		if (L <= 127) {
			counter += L;
			copy(is, os, L, RGBE);
		}
		else {
			counter += L - 128;
			run(is, os, L - 128, RGBE);
		}
	}
	counter = 0;
	//E
	while (counter < hdr.cols()) {
		L = readL(is);

		if (L <= 127) {
			counter += L;
			copy(is, os, L, RGBE);
		}
		else {
			counter += L - 128;
			run(is, os, L - 128, RGBE);
		}
	}
	counter = 0;

	//now I have a 4 * cols sized vector of uint8_t
	uint8_t R, G, B, E;
	float Rf, Gf, Bf;

	//float vector building directly in hdr object

	for (size_t i = 0; i < hdr.cols(); ++i) {
		R = RGBE[i];
		G = RGBE[i + hdr.cols()];
		B = RGBE[i + hdr.cols() * 2];
		E = RGBE[i + hdr.cols() * 3];

		Rf = ((static_cast<float>(R) + 0.5) / 256.0) * std::pow(2, static_cast<float>(E) - 128);
		hdr.push_data(Rf);
		if (Rf < min)
			min = Rf;
		if (Rf > max)
			max = Rf;
		Gf = ((static_cast<float>(G) + 0.5) / 256.0) * std::pow(2, static_cast<float>(E) - 128);
		hdr.push_data(Gf);
		if (Gf < min)
			min = Gf;
		if (Gf > max)
			max = Gf;
		Bf = ((static_cast<float>(B) + 0.5) / 256.0) * std::pow(2, static_cast<float>(E) - 128);
		hdr.push_data(Bf);
		if (Bf < min)
			min = Bf;
		if (Bf > max)
			max = Bf;
	}

}

void decompress(std::ifstream& is, std::ofstream& os) {

	std::string token;
	char ch;
	size_t rows = 0, cols = 0;

	float min = 255.0, max = 0.0; 

	//header
	is >> token;
	if (token != "#?RADIANCE")
		return;

	while (token != "FORMAT=32-bit_rle_rgbe") {
		is >> token;
	}
	is >> token;
	if (token == "-Y") {
		is >> token;
		rows = std::stoul(token);
	}
	is >> token;
	if (token == "+X") {
		is >> token;
		cols = std::stoul(token);
	}
	//end header

	HDRmat hdr(rows, cols, is, os);

	//newRLE
	std::string scanline = "0202";
	std::string hextoken1, hextoken2;
	token.clear();

	for (size_t r = 0; r < rows; ++r) {
		for (int i = 0; i < 4; ++i) {
			is.get(ch);
			if (!std::isspace(ch))
				token.push_back(ch);
			else
				--i;
		}
		for (int i = 0; i < 4; ++i) {
			char buffer[3];
			std::snprintf(buffer, sizeof(buffer), "%02X", token[i]);
			if (i < 2)
				hextoken1 += buffer;
			else
				hextoken2 += buffer;
		}
		if (hextoken1 == scanline) {
			size_t sl_cols = std::stoul(hextoken2, nullptr, 16);
			if (sl_cols == cols) {
				newRLE(is, os, hdr, min, max);
				token.clear();
				hextoken1.clear();
				hextoken2.clear();
			}
			else {
				break;
			}
		}
	}

	std::vector<uint8_t> RGB;
	uint8_t byte;

	//final vector building

	for (size_t i = 0; i < hdr.size(); ++i) {
		byte = 255 * std::pow(((hdr.get_data(i) - min) / (max - min)), 0.45);
		RGB.push_back(byte);
	}

	os << "P7\n" << "WIDTH " << hdr.cols() << "\n" << "HEIGHT " << hdr.rows() << "\n" << "DEPTH 3\n" << "MAXVAL 255\n" << "TUPLTYPE RGB\n" << "ENDHDR\n";

	os.write(reinterpret_cast<char*>(RGB.data()), RGB.size() * sizeof(uint8_t));
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

	decompress(is, os);

	return EXIT_SUCCESS;
}