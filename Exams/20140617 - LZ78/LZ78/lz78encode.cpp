#include <cstdlib>
#include <vector>
#include <iostream>
#include <tuple>
#include <fstream>
//#include <print>
#include <string>
#include <map>
#include <cmath>
#include <cstdio>
#include <cstdint>

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

	std::ostream& operator()(char c, int numbits) {
		uint64_t x = static_cast<uint64_t>(c);
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

	void fillbuffer(uint8_t b, size_t n) {
		buffer_ = b;
		n_ = n;
	}

	uint8_t buffer() { return buffer_; }
	size_t n() { return n_; }
};

void write_couple(char ch, int index, bitwriter &bw, int bits) {
	if (bits < 1) {
		bw(ch, 8);
	}
	if (bits == 1) {
		bw(static_cast<char>(index), 1);
		bw(ch, 8);
	}
	if (bits > 1 && bits <= 3) {
		bw(static_cast<char>(index), 2);
		bw(ch, 8);
	}
	if (bits > 3 && bits <= 7) {
		bw(static_cast<char>(index), 3);
		bw(ch, 8);
	}
	if (bits > 7 && bits <= 15) {
		bw(static_cast<char>(index), 4);
		bw(ch, 8);
	}
	if (bits > 15 && bits <= 31) {
		bw(static_cast<char>(index), 5);
		bw(ch, 8);
	}
	if (bits > 31 && bits <= 63) {
		bw(static_cast<char>(index), 6);
		bw(ch, 8);
	}
	if (bits > 63 && bits <= 127) {
		bw(static_cast<char>(index), 7);
		bw(ch, 8);
	}
	if (bits > 127 && bits <= 255) {
		bw(static_cast<char>(index), 8);
		bw(ch, 8);
	}
	if (bits > 255 && bits <= 511) {
		bw(static_cast<char>(index), 9);
		bw(ch, 8);
	}
	if (bits > 511 && bits <= 1023) {
		bw(static_cast<char>(index), 10);
		bw(ch, 8);
	}
}

bool lz78encode(const std::string& input_filename, const std::string& output_filename, int maxbits) {
	std::ifstream is(input_filename, std::ios::binary);
	if (!is) {
		return false;
	}

	std::ofstream os(output_filename, std::ios::binary);
	if (!os) {
		return false;
	}

	os << "LZ78";

	bitwriter bw(os);

	bw(maxbits, 5); //set maxbits value: only 5 bits so I need to wait to put out the buffer(I can only put out 8 bits)

	char ch, nextch;
	int index = 0, maxindex = static_cast<int>(std::pow(2, maxbits)) - 1;
	std::string s;
	std::map<std::string, int> m;

	while (is.get(ch)) {
		if (index > maxindex) {
			m.clear();
			index = 0;
		}
		std::streampos curpos = is.tellg();

		if (is.get() == EOF) {
			write_couple(ch, 0, bw, index);
			break;
		}

		is.seekg(curpos);

		s.push_back(ch);

		if (m.contains(s)) {
			is.get(nextch);
			++index;
			s.push_back(nextch);
			if (!m.emplace(s, index).second) {
				while (!m.emplace(s, index).second) {
					std::streampos curpos = is.tellg();
					if (is.get() == EOF)
						break;
					is.seekg(curpos);
					is.get(nextch);
					s.push_back(nextch);
				}
				s.pop_back();
				write_couple(nextch, m[s], bw, index - 1);
			}
			else {
				s.clear();
				s.push_back(ch);
				write_couple(nextch, m[s], bw, index - 1);
			}
			s.clear();
		}
		else {
			write_couple(ch, 0, bw, index);
			++index;
			m.emplace(s, index);
			s.clear();
		}
		
	}

	return true;
}
