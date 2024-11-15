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

/*

Write a command line program in C ++ with this syntax:

read_int11 <filein.bin> <fileout.txt>

The first parameter is the name of a binary file that contains 11-bit numbers in 2’s complement, 
with the bits sorted from most significant to least significant. 
The program must create a new file, with the name passed as the second parameter, 
with the same numbers saved in decimal text format separated by a new line. 
Ignore any excess bits in the last byte.

*/

void error(const std::string& message) {
	std::cout << message << '\n';
	exit(EXIT_FAILURE);
}

void syntax() {
	error(
		"SYNTAX:\n"
		"write_int32 <filein.txt> <fileout.bin>\n"
	);
}

template<typename T>
void check_open(const T& stream, const std::string& filename) {
	if (!stream) {
		error(std::string("Cannot open file ") + filename);
	}
}

class bitreader {
	char curr_ch;
	uint16_t buffer_ = 0;
	size_t n_ = 0; 
	std::istream& is_;
	std::ostream& os_;

	void readbit(uint64_t curbit, int numbits) {
		buffer_ = (buffer_ << 1) | (curbit & 1);
		++n_;
		if (n_ == numbits) {
			os_ << buffer_ << std::dec << std::endl;
			n_ = 0;
			buffer_ = 0;
		}
	}

public:
	bitreader(std::istream& is, std::ostream& os) : is_(is), os_(os) {}

	~bitreader() {
		os_ << buffer_ << std::dec << std::endl;
	}

	std::istream& operator()(int numbits) {
		is_.get(curr_ch);
		uint64_t x = static_cast<uint64_t>(curr_ch);
		for (int bitnum = 7; bitnum >= 0; --bitnum) {
			readbit(x >> bitnum, numbits);
		}
		return is_;
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

	std::ofstream os(argv[2]);
	check_open(os, argv[2]);

	is.seekg(0, ios::end);
	auto filesize = is.tellg();
	is.seekg(0, ios::beg);

	bitreader br(is, os);
	for (int x = 0; x < filesize; ++x) {
		br(11);
	}

	return EXIT_SUCCESS;
}