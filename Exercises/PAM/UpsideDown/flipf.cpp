#include <cstdlib>
#include <vector>
#include <iostream>
#include <tuple>
#include <fstream>
#include <print>
#include <string>

template <typename T>
struct mat {
	int64_t rows_;
	int64_t cols_;
	int64_t depth_;
	std::vector<T> data_;

	mat(int64_t rows = 0, int64_t cols = 0, int64_t depth = 0) :
		rows_(rows), cols_(cols * depth), depth_(depth),
		data_(rows * cols * depth) {}

	int64_t rows() const { return rows_; }
	int64_t cols() const { return cols_; }
	int64_t depth() const { return depth_; }
	int64_t size() const { return rows_ * cols_; } //299250
/*
	T& operator()(size_t r, size_t c) { //Actual position while exploring the PAM image tuples
		return data_[r * cols_ + c];
	}
*/
	void operator()(this auto&& self, int64_t r, int64_t c, uint8_t byte) {
		self.data_[r * self.cols_ + c] = byte;
	}
	auto rawsize() const { return size() * sizeof(T); }
	auto rawdata() {
		return reinterpret_cast<char*>(data_.data());
	}
	auto rawdata() const {
		return reinterpret_cast<const char*>(data_.data());
	}
};

void checkline(std::ofstream& os, std::string s, size_t x, size_t& check) {
	
	if (s == "P7") {
		os << s << '\n';
		check++;
	}
	if (s == "WIDTH") {
		os << s << ' ' << x << '\n';
		check++;
	}
	if (s == "HEIGHT") {
		os << s << ' ' << x << '\n';
		check++;
	}
	if (s == "DEPTH") {
		os << s << ' ' << x << '\n';
		check++;
	}
	if (s == "MAXVAL") {
		os << s << ' ' << x << '\n';
		check++;
	}
	if (s == "TUPLTYPE RGB" || s == "TUPLTYPE GRAYSCALE") {
		os << s << '\n';
		check++;
	}
}

bool build_header(std::ifstream& is, std::ofstream& os, int64_t& rows, int64_t& cols, int64_t& depth) {

	std::string hline, s_val;
	size_t val = 0, check = 0;
	char ch;


	while (is.get(ch)) {
		if (hline == "ENDHDR") {
			os << hline << '\n';
			break;
		}
		if (ch == ' ') {
			if (hline == "TUPLTYPE") {
				hline.push_back(ch);
			}
			else {
				while (is.get(ch)) {
					if (ch == '\n') {
						val = std::stoi(s_val);
						if (hline == "WIDTH") {
							cols = val;
						}
						if (hline == "HEIGHT") {
							rows = val;
						}
						if (hline == "DEPTH") {
							depth = val;
						}
						checkline(os, hline, val, check);
						s_val.clear();
						hline.clear();
						break;
					}
					s_val.push_back(ch);
				}
			}
		}
		else if (ch == '\n') {
			checkline(os, hline, val, check);
			hline.clear();
		}
		else {
			hline.push_back(ch);
		}
	}
	if (check != 6) {
		return false;
	}

	return true;
}

void flip(mat<uint8_t> img, std::ifstream& is, std::ofstream& os) {

	char ch;
	uint8_t byte;
	int64_t count = 0;
	
	
		for (int64_t r = img.rows() - 1; r >= 0; --r) {
			for (int64_t c = 0; c < img.cols(); ++c) {
				is.get(ch);
				byte = static_cast<uint8_t>(ch);
				img(r, c, byte);
				++count;
			}
		}
	
	std::cout << count;

	os.write(img.rawdata(), img.rawsize());
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cout << "ERROR: invalid number of arguments.\n";
		return EXIT_FAILURE;
	}
	std::ifstream is(argv[1], std::ios::binary);
	if (!is) {
		std::cout << "ERROR: can't open input file.\n";
		return EXIT_FAILURE;
	}
	std::ofstream os(argv[2], std::ios::binary);
	if (!os) {
		std::cout << "ERROR: can't open output file.\n";
		return EXIT_FAILURE;
	}
	
	int64_t rows, cols, depth;
	bool pam = build_header(is, os, rows, cols, depth);

	if (!pam) {
		std::cout << "ERROR: not PAM image.\n";
		return EXIT_FAILURE;
	}
	else {
		std::cout << "PAM image, let's continue.\n";
	}

	mat<uint8_t> img(rows, cols, depth);

	flip(img, is, os);

	return EXIT_SUCCESS;
}