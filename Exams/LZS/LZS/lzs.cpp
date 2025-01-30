#include "lzs.h"
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <ios>

class bitreader {
	int n_ = 0;
	int readbits_ = 0;
	uint16_t buffer_ = 0;
	uint16_t bits_ = 0;
	char curr_ch;
	uint8_t byte_;
	std::istream& is_;
	std::ostream& os_;

	std::streampos curpos;

	void readbit(uint8_t curbit, int& numbits) {
		if (n_ < readbits_) {
			buffer_ = buffer_ << 0;
			++numbits;
		}
		else {
			buffer_ = (buffer_ << 1) | (curbit & 1);
		}
		++n_;
	}

	void pastbits() {
		while (n_ >= 8) {
			n_ -= 8;
		}
		readbits_ = n_;
	}

public:
	bitreader(std::istream& is, std::ostream& os) : is_(is), os_(os) {}

	uint16_t operator() (int numbits) {
		curpos = is_.tellg();
		is_.get(curr_ch);
		byte_ = static_cast<uint8_t>(curr_ch);

		while (n_ < numbits) {
			for (int bitnum = 7; bitnum >= 0; --bitnum) {
				readbit(byte_ >> bitnum, numbits);
				if (n_ == numbits) {
					pastbits();
					if (readbits_ != 0) {
						is_.seekg(curpos);
					}
					n_ = 0;
					bits_ = buffer_;
					buffer_ = 0;
					break;
				}
			}
			if (n_ == 0)
				break;
			curpos = is_.tellg();
			is_.get(curr_ch);
			byte_ = static_cast<uint8_t>(curr_ch);
		}

		return bits_;
	}

	uint16_t ignore_read (int numbits) {
		curpos = is_.tellg();
		is_.get(curr_ch);
		byte_ = static_cast<uint8_t>(curr_ch);

		while (n_ < numbits) {
			for (int bitnum = 7; bitnum >= 0; --bitnum) {
				readbit(byte_ >> bitnum, numbits);
				if (n_ == numbits) {
					is_.seekg(curpos);
					//pastbits(numbits); don't save the number of read bits as I will go back to the starting position
					n_ = 0;
					bits_ = buffer_;
					buffer_ = 0;
					break;
				}
			}
			if (n_ == 0)
				break;
			is_.get(curr_ch);
			byte_ = static_cast<uint8_t>(curr_ch);
			curpos = is_.tellg();
		}

		return bits_;
	}

	void set_readbits(int num) { readbits_ = num; }
};

bool end_marker(std::istream& is, bitreader& br) {
	auto curpos = is.tellg();

	uint16_t token = br.ignore_read(9);

	is.seekg(curpos);

	if (token == 384) {
		return true;
	}
	else {
		return false;
	}
}

uint16_t length_decode(std::istream& is, bitreader& br) {

	uint16_t code, length = 0;
	int N = 0;

	auto curpos = is.tellg();

	code = br.ignore_read(2);
	if (code == 0) {
		is.seekg(curpos);
		br(2);
		return 2;
	}
	if (code == 1) {
		is.seekg(curpos);
		br(2);
		return 3;
	}
	if (code == 2) {
		is.seekg(curpos);
		br(2);
		return 4;
	}
	is.seekg(curpos);
	
	code = br.ignore_read(4);
	if (code == 12) {
		is.seekg(curpos);
		br(4);
		return 5;
	}
	if (code == 13) {
		is.seekg(curpos);
		br(4);
		return 6;
	}
	if (code == 14) {
		is.seekg(curpos);
		br(4);
		return 7;
	}
	if (code == 15) {
		is.seekg(curpos);
		while (code == 15) {
			code = br(4);
			if(code == 15)
				++N;
		}
		length = code + (15 * N - 7);
	}
	
	return length;
}

void lzs_decompress(std::istream& is, std::ostream& os) {
	
	std::vector<uint16_t> buffer;
	bitreader br(is, os);

	uint16_t label_bit, offset_label;
	uint16_t literal, offset = 0, length = 0;

	is.seekg(0, std::ios::end);
	auto filesize = is.tellg();
	is.seekg(0, std::ios::beg);

	while(!end_marker(is, br)){	
		label_bit = br(1);

		if (label_bit == 0) { //literal
			literal = br(8);
			buffer.push_back(literal);
		}
		else if (label_bit == 1) { //offset-lenght

			offset_label = br(1);

			if (offset_label == 0) {
				offset = br(11);
			}
			else if (offset_label == 1) { //offset lower than 128
				offset = br(7);
			}

			length = length_decode(is, br);

			size_t size = buffer.size();
			size_t index = size - offset;

			for (int i = 0; i < length ; ++i) {
				buffer.push_back(buffer[index]);
				if (index < size - 1)
					++index;
			}
			
		}
	}

	for (uint16_t x : buffer) {
		char ch = static_cast<char>(x);
		os << ch;
	}
}


int main(int argc, char* argv[]) {
	std::ifstream is(argv[1], std::ios::binary);
	std::ofstream os(argv[2]);

	lzs_decompress(is, os);

	return EXIT_SUCCESS;
}
