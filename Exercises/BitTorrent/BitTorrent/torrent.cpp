/*
#include <ios>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

struct torrent_reader {
	std::ifstream& is_;
	std::ofstream& os_;
	char ch_ = 0;

	torrent_reader(std::ifstream& is, std::ofstream& os) : is_(is), os_(os) {}

	void length(std::ifstream& is, size_t l) {

	}

	void read_dict(std::ofstream& os, std::ifstream& is, uint8_t b, uint8_t last_b, size_t pos) {
		is_.get(ch_);
		b = static_cast<uint8_t>(ch_);
		pos = is.tellg();

		if (b == 'd') {
			read_dict(os, is, b, b);
		}
		if (b == 'l') {
			read_list(os, is, b, b);
		}
		if (b == 'i') {
			read_int(os, is, b, b);
		}
		if (b == ':') {
			read_string(os, is, b, b);
		}

		read_dict(os, is, b, b, pos);
	}
	void read_list(std::ofstream& os, std::ifstream& is, uint8_t b, uint8_t last_b) {
		is_.get(ch_);
		b = static_cast<uint8_t>(ch_);

		if (b == 'e') {

		}
		if (b == 'd') {
			read_dict(os, is, b, b);
		}
		if (b == 'l') {
			read_list(os, is, b, b);
		}
		if (b == 'i') {
			read_int(os, is, b, b);
		}
		if (b == ':') {
			read_string(os, is, b, b);
		}
	}
	void read_int(std::ofstream& os, std::ifstream& is, uint8_t b, uint8_t last_b) {
		is_.get(ch_);
		b = static_cast<uint8_t>(ch_);

		if (b == 'e') {

		}
		if (b == 'd') {
			read_dict(os, is, b, b);
		}
		if (b == 'l') {
			read_list(os, is, b, b);
		}
		if (b == 'i') {
			read_int(os, is, b, b);
		}
		if (b == ':') {
			read_string(os, is, b, b);
		}
	}
	void read_string(std::ofstream& os, std::ifstream& is, uint8_t b, size_t l) {
		
		//os.put('"');
		length(l);
		for (size_t i = 0; i < length; ++i) {
			is_.get(ch_);
			b = static_cast<uint8_t>(ch_);
			os.put(ch_);
		}
		//os.put('"');
		return;
		
		if (b == 'e') {

			return;
		}
		if (b == 'd') {
			read_dict(os, is, b, b);
		}
		if (b == 'l') {
			read_list(os, is, b, b);
		}
		if (b == 'i') {
			read_int(os, is, b, b);
		}
		if (b == ':') {
			read_string(os, is, b, b);
		}
		
	}

	void recursive_read(std::ofstream& os_, std::ifstream& is_, uint8_t b, uint8_t last_b, size_t pos) {
		is_.get(ch_);
		b = static_cast<uint8_t>(ch_);
		pos = is_.tellg();

		//recursive_read(os_, is_, b, b, pos);

		if (b == 'd') {
			read_dict(os_, is_, b, b);
			is_.seekg(pos);
		}
		if (b == 'l') {
			is_.seekg(pos);
			read_list(os_, is_, b, b);
		}
		if (b == 'i') {
			is_.seekg(pos);
			read_int(os_, is_, b, b);
		}
		if (b == ':') {
			is_.seekg(pos);
			read_string(os_, is_, b, b);
		}
	}
};

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cout << "ERROR: invalid number of parameters.\n";
		return -1;
	}
	std::ifstream is(argv[1], std::ios::binary);
	if (!is) {
		std::cout << "ERROR: impossible to open .torrent file.\n";
		return -1;
	}

	torrent_reader tr(is);

	tr.recursive_read(is, 0, 0);

	return EXIT_SUCCESS;
}
*/

#include <iostream>
#include <fstream>
#include <variant>
#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include <exception>

namespace bencode {

	struct parse_error : public std::runtime_error {
		parse_error() : std::runtime_error("Parse error") {}
	};

	struct elem {
		char type_;
		std::string s_;
		int64_t i_;
		std::vector<elem> l_;
		std::map<std::string, elem> d_;

		elem(std::istream& is) {
			type_ = is.peek();
			if (type_ == EOF) {
				throw parse_error();
			}
			if (type_ == 'i') {
				is.get(); // remove 'i'
				is >> i_;
				if (is.get() != 'e') {
					throw parse_error();
				}
			}
			else if (type_ == 'l') {
				is.get(); // remove 'l' 
				while (is.peek() != 'e') {
					l_.emplace_back(is);
				}
				is.get(); // remove 'e'
			}
			else if (type_ == 'd') {
				is.get(); // remove 'd'
				while (is.peek() != 'e') {
					elem key(is);
					if (key.type_ != 's') {
						throw parse_error();
					}
					d_.emplace(key.s_, is);
				}
				is.get(); // remove 'e'
			}
			else {
				type_ = 's';
				size_t len;
				is >> len;
				if (is.get() != ':') {
					throw parse_error();
				}
				s_.resize(len);
				is.read(s_.data(), len);
			}
		}

		void print(int tabs = 0, const std::string& key = "") const {
			if (key == "pieces") {
				for (size_t i = 0; i < s_.size(); ++i) {
					if (i % 20 == 0) {
						std::cout << '\n' << std::string(tabs + 1, '\t');
					}
					std::cout << std::format("{:02x}",
						static_cast<unsigned char>(s_[i]));
				}
				std::cout << '\n';
			}
			else if (type_ == 'i') {
				std::cout << i_ << '\n';
			}
			else if (type_ == 'l') {
				std::cout << "[\n";
				for (const auto& x : l_) {
					std::cout << std::string(tabs + 1, '\t');
					x.print(tabs + 1);
				}
				std::cout << std::string(tabs, '\t') << "]\n";
			}
			else if (type_ == 'd') {
				std::cout << "{\n";
				for (const auto& [key, value] : d_) {
					std::cout << std::string(tabs + 1, '\t');
					std::cout << '"' << key << "\" => ";
					value.print(tabs + 1, key);
				}
				std::cout << std::string(tabs, '\t') << "}\n";
			}
			else {
				std::cout << '"';
				for (auto ch : s_) {
					if (ch < 32 || ch > 126) {
						std::cout << '.';
					}
					else {
						std::cout << ch;
					}
				}
				std::cout << "\"\n";
			}
		}
	};

}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		std::cout << "SYNTAX\ntorrent_dump <file.torrent>";
		return EXIT_FAILURE;
	}
	std::ifstream is(argv[1], std::ios::binary);
	if (!is) {
		std::cout << std::format("Cannot open {} for reading", argv[1]);
		return EXIT_FAILURE;
	}

	bencode::elem torrent(is);
	torrent.print();

	return EXIT_SUCCESS;
}