#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdint>

struct bitreader {
	uint8_t buffer_ = 0;
	int n_ = 0;
	std::ifstream& is_;

	uint8_t readbit() {
		if (n_ == 0) {
			char ch;
			is_.get(ch);
			buffer_ = static_cast<uint8_t>(ch);
			n_ = 8;
		}
		--n_;
		return (buffer_ >> n_) & 1;
	}

	bitreader(std::ifstream& is) : is_(is) {}

	uint8_t operator() (int numbits) {
		uint8_t x = 0;
		while (--numbits >= 0) {
			x = (x << 1) | readbit();
		}
		return x;
	}

	void reset(int start_n, std::streampos curpos) {
		n_ = start_n;

		char ch;
		is_.seekg(static_cast<int>(curpos) - 1);
		is_.get(ch);
		buffer_ = static_cast<uint8_t>(ch);
	}
};

struct bitwriter {
	uint8_t buffer_ = 0;
	int n_ = 0;
	std::ofstream& os_;

	void writebit(uint8_t curbit) {
		buffer_ = (buffer_ << 1) | (curbit & 1);
		++n_;
		if (n_ == 8) {
			os_.put(buffer_);
			buffer_ = 0;
			n_ = 0;
		}
	}

	bitwriter(std::ofstream& os) : os_(os) {}

	void operator() (int numbits, uint8_t byte) {
		while (--numbits >= 0) {
			writebit(byte >> numbits);
		}
	}

	void flush(uint8_t padbit) {
		while (n_ != 0) {
			writebit(padbit);
		}
	}

	~bitwriter() {
		flush(0);
	}
};

bool decompress(std::string input_filename, std::string output_filename) {
	std::ifstream is(input_filename, std::ios::binary);
	if (!is) {
		return false;
	}
	std::ofstream os(output_filename);
	if (!os) {
		return false;
	}

	bitreader br(is);
	uint8_t endmarker = 0;

	while (endmarker != 64) {
		uint8_t option = br(1);
		if (option == 0) {
			uint8_t count = br(6) + 1;
			for (int i = 0; i < count; ++i) {
				uint8_t byte = br(8);
				os << byte;
			}
		}
		else {
			uint8_t count = br(6) + 1;
			uint8_t byte = br(8);
			for (int i = 0; i < count; ++i) {
				os << byte;
			}
		}
		auto curpos = is.tellg();
		int start_n = br.n_;
		endmarker = br(7);
		br.reset(start_n, curpos);
	}

	return true;
}

bool compress(std::string input_filename, std::string output_filename) {
	std::ifstream is(input_filename);
	if (!is) {
		return false;
	}
	std::ofstream os(output_filename, std::ios::binary);
	if (!os) {
		return false;
	}

	char cur_ch, next_ch;
	bitwriter bw(os);

	while (is.get(cur_ch)) {
		auto curpos = is.tellg();
		is.get(next_ch);
		if (cur_ch == next_ch) {
			bw(1, 1);
			int copies = 1;
			while (cur_ch == next_ch) {
				is.get(next_ch);
				++copies;
			}
			auto newpos = is.tellg();
			if (newpos == -1) {

			}
			else {
				is.seekg(static_cast<int>(newpos) - 1);
			}
			bw(6, copies - 1);
			bw(8, cur_ch);
		}
		else {
			bw(1, 0);
			std::string copy;
			int copies = 0;
			while (cur_ch != next_ch) {
				copy.push_back(cur_ch);
				cur_ch = next_ch;
				is.get(next_ch);
				++copies;
			}
			auto newpos = is.tellg();
			if (newpos == -1) {
				copy.push_back(cur_ch);
				++copies;
			}
			else {
				is.seekg(static_cast<int>(newpos) - 2);
			}
			bw(6, copies - 1);
			for (int i = 0; i < copies; ++i) {
				bw(8, copy[i]);
			}
		}
	}
	bw(7, 64);

	return true;
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		return EXIT_FAILURE;
	}
	std::string procedure = argv[1];
	std::string input = argv[2], output = argv[3];
	if (procedure == "c") {
		compress(input, output);
	}
	else {
		decompress(input, output);
	}
	
	return EXIT_SUCCESS;
}