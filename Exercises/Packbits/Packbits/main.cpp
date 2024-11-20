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

	void length(char ch) {	
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
				if (ch == nextch) {
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
				copyvect_.push_back(ch);
				writecopy(copyvect_);
				copyvect_.clear();
			}
			
		}
		
	}

public:
	pbcompresser(std::istream& is, std::ostream& os) : is_(is), os_(os) {}
	~pbcompresser() {

	}

	std::ostream& operator()(char ch) {
		copyvect_.push_back(lastbyte_);
		if (ch == lastbyte_) {
			++run_;
		}
		else {
			++copy_;
			length(ch);
		}
		lastbyte_ = ch;
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

	std::ostream& decompress_length(size_t x) {
		L_ = x;
		return os_;
	}
	std::ostream& decompress_char(char ch) {
		if (L_ < 128) {
			os_.write(&ch, 1);
		}
		else if (L_ > 128) {
			for (size_t i = 0; i < 257 - L_; i++) {
				os_.write(&ch, 1);
			}
		}
		else if (L_ == 128) {
			return os_;
		}
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
		if (ch == '\r' || ch == '\n') {
			os << std::endl;
			continue;
		}
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

	size_t x;
	char ch;
	pbdecompresser pb(is, os);
	std::string token;

	while (std::getline(is, token, '|')) {
		// Controlla se il token è numerico
		bool is_number = !token.empty() && std::all_of(token.begin(), token.end(), ::isdigit);

		if (is_number) {
			x = std::stoi(token);
			pb.decompress_length(x);
		}
		else {
			ch = token[0];
			pb.decompress_char(ch);
		}
		
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