#include <algorithm>
#include <bit>
#include <fstream>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <ranges>
#include <array>
#include <string>
#include <vector>

void syntax() {
	std::cout <<
		"SYNTAX:\n"
		"elias [c|d] <filein> <fileout>\n";
	exit(EXIT_FAILURE);
}

void error(std::string_view s) {
	std::cout << "ERROR: " << s << "\n";
	exit(EXIT_FAILURE);
}

class pbcompresser {
	char lastbyte_ = 0;
	size_t L_ = 0;
	size_t run_ = 1;
	size_t copy_ = 0;
	std::vector<char> copyvect_;
	std::istream& is_;
	std::ostream& os_;

	void writerun(char ch) {
		os_ << "|" << L_ << "|" << ch;
	}

	void writecopy(std::vector<char> v) {
		os_ << "|" << L_ << "|";
		for (int i = 0; i < v.size(); i++) {
			if (i < v.size() - 1)
				os_ << v.at(i) << "|";
			else
				os_ << v.at(i);
		}
	}

	void length(char x) {	
		if (run_ > 1) {
			L_ = 257 - run_;
			writerun(lastbyte_);
			copyvect_.clear();
			run_ = 1;
			copy_ = 1;
			L_ = 0;
		}
		if (copy_ > 1) {
			std::streampos curpos = is_.tellg();
			is_.seekg(0, std::ios::end);
			std::streampos filesize = is_.tellg();
			is_.seekg(curpos);

			char nextch;
			if (curpos < filesize) {
				is_.get(nextch);
				is_.seekg(curpos);
				if (x == nextch) {
					--copy_;
					L_ = copy_;
					writecopy(copyvect_);
					copyvect_.clear();
					run_ = 1;
					copy_ = 1;
					L_ = 0;
				}
			}
			else {
				is_.seekg(curpos);
				L_ = copy_;
				copyvect_.push_back(x);
				writecopy(copyvect_);
				copyvect_.clear();
			}
			
		}
		
	}

public:
	pbcompresser(std::istream& is, std::ostream& os) : is_(is), os_(os) {}
	~pbcompresser() {

	}

	std::ostream& operator()(char x) {
		copyvect_.push_back(lastbyte_);
		if (x == lastbyte_) {
			++run_;
		}
		else {
			++copy_;
			length(x);
		}
		lastbyte_ = x;
		return os_;
	}
};

class pbdecompresser {
	uint8_t lastbyte_ = 0;
	size_t L_ = 0;
	size_t run_ = 1;
	size_t copy_ = 0;
	std::vector<uint8_t> copyvect_;
	std::istream& is_;
	std::ostream& os_;

	

public:
	pbdecompresser(std::istream& is, std::ostream& os) : is_(is), os_(os) {}
	~pbdecompresser() {

	}

	std::ostream& operator()(uint8_t x) {
		return os_;
	}
};

void compress(const std::string& input_filename, const std::string& output_filename) {
	std::ifstream is(input_filename, std::ios::binary);
	if (!is) {
		error("Cannot open file " + input_filename + " for reading");
	}
	std::ofstream os(output_filename/*, std::ios::binary*/);
	if (!os) {
		error("Cannot open file " + output_filename + " for writing");
	}
	is.seekg(0, std::ios::end);
	auto filesize = is.tellg();
	is.seekg(0, std::ios::beg);

	char ch;
	pbcompresser pb(is, os);
	for (int i = 0; i < filesize; ++i) {
		is.get(ch);
		pb(ch);
	}
	os << "|" << 128 << "|";
}

void decompress(const std::string& input_filename, const std::string& output_filename) {
	std::ifstream is(input_filename/*, std::ios::binary*/);
	if (!is) {
		error("Cannot open file " + input_filename + " for reading");
	}
	std::ofstream os(output_filename, std::ios::binary);
	if (!os) {
		error("Cannot open file " + output_filename + " for writing");
	}
	is.seekg(0, std::ios::end);
	auto filesize = is.tellg();
	is.seekg(0, std::ios::beg);

	char ch;
	pbdecompresser pb(is, os);
	for (int i = 0; i < filesize; ++i) {
		is.get(ch);
		uint8_t x = static_cast<uint8_t>(ch);
		pb(x);
	}
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		syntax();
	}

	std::string command = argv[1];
	if (command == "c") {
		compress(argv[2], argv[3]);
	}
	else if (command == "d") {
		decompress(argv[2], argv[3]);
	}
	else {
		syntax();
	}

	return EXIT_SUCCESS;
}