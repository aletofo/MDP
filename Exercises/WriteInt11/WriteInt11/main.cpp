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

Write a command line program in C++ with this syntax:

write_int11 <filein.txt> <fileout.bin>

The first parameter is the name of a text file that contains base 10 integers from -1000 to 1000 separated by whitespace. 
The program must create a new file, with the name passed as the second parameter, with the same numbers saved as 11-bit binary in 2’s complement. 
The bits are inserted in the file from the most significant to the least significant. 
The last byte of the file, if incomplete, is filled with bits equal to 0. 
Since the incomplete byte will have at most 7 padding bits, there’s no risk of interpreting padding as another value.

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

class bitwriter {
	uint8_t buffer_;
	size_t n_ = 0; // 0-8
	std::ostream& os_;

	void writebit(uint64_t curbit) {
		buffer_ = (buffer_ << 1) | (curbit & 1);
		++n_;
		if (n_ == 8) {
			os_.put(buffer_);
			n_ = 0;
		}
	}

public:
	bitwriter(std::ostream& os) : os_(os) {}

	~bitwriter() {
		flush();
	}

	std::ostream& operator()(uint64_t x, int numbits) {
		for (int bitnum = numbits - 1; bitnum >= 0; --bitnum) {
			writebit(x >> bitnum);
		}
		return os_;
	}

	std::ostream& flush(int padbit = 0) {
		while (n_ > 0) {
			writebit(padbit);
		}
		return os_;
	}
};

int main(int argc, char* argv[])
{
	using namespace std;

	if (argc != 3) {
		syntax();
	}

	std::ifstream is(argv[1]/*, std::ios::binary*/);
	check_open(is, argv[1]);

	vector<int> v{ istream_iterator<int>(is),
		istream_iterator<int>() };

	//v = { 7, -64, 510, 15, -128, 1020, 31, -256, };

	std::ofstream os(argv[2], std::ios::binary);
	check_open(os, argv[2]);

	bitwriter bw(os);
	for (const auto& x : v) {
		bw(x, 11);
	}

	return EXIT_SUCCESS;
}