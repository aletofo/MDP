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

struct PKBcompresser {
	uint8_t L_ = 0;
	uint8_t run_ = 1;
	uint8_t copy_ = 0;
	char lastbyte_ = 'x';
	char byte_;
	std::vector<char> copyvector_;
	std::ifstream& is_;
	std::ofstream& os_;

	PKBcompresser(std::ifstream& is, std::ofstream& os) : is_(is), os_(os) {}

	void writerun() {
		os_ << L_ << lastbyte_;
	}
	void writecopy() {
		os_ << L_ << ",";
		for (int i = 0; i < copyvector_.size(); i++) {
			if (i < copyvector_.size() - 1)
				os_ << copyvector_.at(i);
			else
				os_ << copyvector_.at(i);
		}
	}

	void check() {
		if (run_ > 1) {
			L_ = 257 - run_;
			writerun();
			copyvector_.clear();
			run_ = 1;
			copy_ = 1;
			L_ = 0;
		}
		else if (copy_ > 1) {
			std::streampos curpos = is_.tellg();
			is_.seekg(0, std::ios::end);
			std::streampos filesize = is_.tellg();
			is_.seekg(curpos);

			char nextch;
			if (curpos < filesize) {
				is_.get(nextch);
				is_.seekg(curpos);
				if (byte_ == nextch) {
					--copy_;
					L_ = copy_;
					writecopy();
					copyvector_.clear();
					run_ = 1;
					copy_ = 1;
					L_ = 0;
				}
			}
			else {
				is_.seekg(curpos);
				L_ = copy_;
				copyvector_.push_back(byte_);
				writecopy();
				copyvector_.clear();
				run_ = 1;
				copy_ = 1;
				L_ = 0;
			}
		}
	}

	void operator() () {
		is_.get(byte_);
		copyvector_.push_back(byte_);
		if (byte_ == lastbyte_) {
			++run_;
		}
		else {
			++copy_;
			check();
		}
		lastbyte_ = byte_;
	}

};

class bitreader {
	char curr_ch;
	uint16_t buffer_ = 0;
	size_t n_ = 0;
	std::istream& is_;
	std::ostream& os_;
	std::string dictionary_ = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	void readbit(uint64_t curbit, int numbits) {
		buffer_ = (buffer_ << 1) | (curbit & 1);
		++n_;
		if (n_ == numbits) {
			os_ << dictionary_[buffer_];
			n_ = 0;
			buffer_ = 0;
		}
	}

public:
	bitreader(std::istream& is, std::ostream& os) : is_(is), os_(os) {}

	~bitreader() {}

	std::istream& operator()(int numbits) {
		is_.get(curr_ch);
		uint64_t x = static_cast<uint64_t>(curr_ch);
		for (int bitnum = 7; bitnum >= 0; --bitnum) {
			readbit(x >> bitnum, numbits);
		}
		return is_;
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

void compress(std::ifstream &is, std::ofstream &os) {

	is.seekg(0, std::ios::end);
	auto filesize = is.tellg();
	is.seekg(0, std::ios::beg);

	PKBcompresser pkbc(is,os);

	for (int i = 0; i < filesize; ++i) {
		pkbc();
	}
	pkbc.check();
	uint8_t terminator = 0x80;
	os << terminator;
}

void base64table() {

	std::string dictionary = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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

	Rbin.close();
	Gbin.close();
	Bbin.close();

	//Packbits compression

	std::ifstream isR("R.bin", std::ios::binary);
	std::ifstream isG("G.bin", std::ios::binary);
	std::ifstream isB("B.bin", std::ios::binary);

	std::ofstream osR("r.pkb", std::ios::binary);
	std::ofstream osG("g.pkb", std::ios::binary);
	std::ofstream osB("b.pkb", std::ios::binary);

	compress(isR,osR);
	compress(isG,osG);
	compress(isB,osB);

	osR.close();
	osG.close();
	osB.close();

	//Base64 compression

	std::ifstream isRpkb("r.pkb", std::ios::binary);
	std::ifstream isGpkb("g.pkb", std::ios::binary);
	std::ifstream isBpkb("b.pkb", std::ios::binary);

	std::ofstream osRtxt("r.txt");
	std::ofstream osGtxt("g.txt");
	std::ofstream osBtxt("b.txt");

	bitreader brR(isRpkb, osRtxt);
	bitreader brG(isGpkb, osGtxt);
	bitreader brB(isBpkb, osBtxt);

	std::string dictionary = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	isRpkb.seekg(0, std::ios::end);
	auto filesizeR = isRpkb.tellg();
	isRpkb.seekg(0, std::ios::beg);

	for (int x = 0; x < filesizeR; ++x) {
		brR(6);
	}

	isGpkb.seekg(0, std::ios::end);
	auto filesizeG = isGpkb.tellg();
	isGpkb.seekg(0, std::ios::beg);

	for (int x = 0; x < filesizeG; ++x) {
		brG(6);
	}
	int remainder = (filesizeG * 8) % 6;
	for (int i = 0; i < remainder; i++) {
		osGtxt << "=";
	}

	isBpkb.seekg(0, std::ios::end);
	auto filesizeB = isBpkb.tellg();
	isBpkb.seekg(0, std::ios::beg);

	for (int x = 0; x < filesizeB; ++x) {
		brB(6);
	}

	return EXIT_SUCCESS;
}