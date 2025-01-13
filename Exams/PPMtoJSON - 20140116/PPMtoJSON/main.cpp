#include <cstdlib>
#include <vector>
#include <iostream>
#include <tuple>
#include <fstream>
#include <print>
#include <string>

template <typename T>
struct ppmimg {
	size_t rows_;
	size_t cols_;
	size_t maxval_;
	std::vector<T> data_;

	ppmimg(size_t rows = 0, size_t cols = 0) : rows_(rows), cols_(cols), data_(rows * cols) {}

	size_t rows() const { return rows_; }
	size_t cols() const { return cols_; }
	size_t size() const { return rows_ * cols_; }

	void operator() (this auto&& self, size_t r, size_t c, uint8_t byte) {
		self.data_[r * self.cols_ + c] = byte;
	}
	uint8_t at (this auto&& self, size_t r, size_t c) {
		return self.data_[r * self.cols_ + c];
	}
	auto rawsize() const { return size() * sizeof(T); }
	auto rawdata() {
		return reinterpret_cast<char*>(data_.data());
	}
	auto rawdata() const {
		return reinterpret_cast<const char*>(data_.data());
	}
};

void check(std::string s, int &count, size_t &r, size_t &c, size_t &mv) {

	if (s == "P6")
		return;

	for (int i = 0; i < s.size(); i++) {
		if (!isdigit(s[i]))
			return;
	}
	
	if (count == 0) {
		c = std::stoi(s) * 3; //by 3 because we need to condier the RGB format
		count++;
		return;
	}
	if (count == 1) {
		r = std::stoi(s);
		count++;
		return;
	}
	if (count == 2) {
		mv = std::stoi(s);
		count++;
		return;
	}
	return;
}

void ppmheader(std::ifstream &is, size_t &r, size_t &c, size_t &mv) {

	std::string buffer;
	char ch;
	int count = 0;

	while (is.get(ch)) {
		while (!std::isspace(static_cast<unsigned char>(ch))) {
			buffer.push_back(ch);
			is.get(ch);
		}
		check(buffer, count, r, c, mv);
		buffer.clear();
		if (count == 3) {
			break;
		}
	}

	return;
}

void datareading(std::ifstream& is, ppmimg<uint8_t>& img) {

	char ch;
	uint8_t byte;

	for (size_t r = 0; r < img.rows(); ++r) {
		for (size_t c = 0; c < img.cols(); ++c) {
			is.get(ch);
			byte = static_cast<uint8_t>(ch);
			img(r, c, byte);
		}
	}

	return;
}

void split(ppmimg<uint8_t>& img, ppmimg<uint8_t>& R, ppmimg<uint8_t>& G, ppmimg<uint8_t>& B) {

	for (size_t r = 0; r < img.rows(); ++r) {
		for (size_t c = 0; c < img.cols(); ++c) {
			for (size_t channel = 0; channel < 3; ++channel) {
				if (channel == 0) {
					R(r, c / 3, img.at(r, c));
					++c;
				}
				if (channel == 1) {
					G(r, c / 3, img.at(r, c));
					++c;
				}
				if (channel == 2) {
					B(r, c / 3, img.at(r, c));
				}
			}
		}
	}
}

int main(int argc, char* argv[]) {

	if (argc != 2) {
		std::cout << "ERROR: invalid number of arguments.\n";
		return EXIT_FAILURE;
	}
	
	//PPM image loading

	std::ifstream is(argv[1], std::ios::binary);
	if (!is) {
		std::cout << "ERROR: can't open input file.\n";
		return EXIT_FAILURE;
	}

	size_t rows = 0, cols = 0, maxvalue = 0;

	ppmheader(is, rows, cols, maxvalue);
	ppmimg<uint8_t> img(rows, cols);

	datareading(is, img);

	//splitting channels in R, G, B

	ppmimg<uint8_t> R(img.rows(), img.cols() / 3);
	ppmimg<uint8_t> G(img.rows(), img.cols() / 3);
	ppmimg<uint8_t> B(img.rows(), img.cols() / 3);

	split(img, R, G, B);

	std::ofstream Rbin("R.bin", std::ios::binary);
	std::ofstream Gbin("G.bin", std::ios::binary);
	std::ofstream Bbin("B.bin", std::ios::binary);

	Rbin.write(R.rawdata(), R.rawsize());
	Gbin.write(G.rawdata(), G.rawsize());
	Bbin.write(B.rawdata(), B.rawsize());

	return EXIT_SUCCESS;
}