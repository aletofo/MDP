#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <array>
#include <chrono>
#include <format>
#include <vector>
#include <iterator>
#include <algorithm>
#include <ranges>

void error(const std::string& message) {
	std::cout << message << '\n';
	exit(EXIT_FAILURE);
}

void syntax() {
	error(
		"SYNTAX:\n"
		"read_int11 <filein.bin> <fileout.txt>\n"
	);
}

template<typename T>
void check_open(const T& stream, const std::string& filename) {
	if (!stream) {
		error(std::string("Cannot open file ") + filename);
	}
}

class bitreader {
	uint8_t buffer_;
	size_t n_ = 0; // 0-8
	std::istream& is_;

	uint64_t readbit() {
		if (n_ == 0) {
			buffer_ = is_.get();
			n_ = 8;
		}
		--n_;
		return (buffer_ >> n_) & 1;
	}

public:
	bitreader(std::istream& is) : is_(is) {}

	uint64_t operator()(int numbits) {
		uint64_t u = 0;
		for (int bitnum = numbits - 1; bitnum >= 0; --bitnum) {
			u = (u << 1) | readbit();
		}
		return u;
	}

	bool fail() const {
		return is_.fail();
	}

	operator bool() const {
		return !fail();
	}
};

int main(int argc, char* argv[])
{
	using namespace std;

	if (argc != 3) {
		syntax();
	}

	std::ifstream is(argv[1], std::ios::binary);
	check_open(is, argv[1]);

	bitreader br(is);

	vector<int> v;
	while (true) {
		int num = static_cast<int>(br(11));
		if (!br) {
			break;
		}
		if (num >= 1024) {
			num = num - 2048;
		}
		v.push_back(num);
	}

	std::ofstream os(argv[2]/*, std::ios::binary*/);
	check_open(os, argv[2]);

	std::ranges::copy(v, ostream_iterator<int>(os, "\n"));

	return EXIT_SUCCESS;
}