#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <ranges>

struct node {
	uint8_t sym_ = 0;
	std::string label_;
	uint8_t length_;
};


class bitreader {
	char curr_ch;
	uint16_t buffer_ = 0;
	uint16_t n_attr = 0;
	int n_ = 0;
	uint8_t nbits_ = 0;
	bool read_ = false;
	std::string label_;
	std::istream& is_;
	std::ostream& os_;
	std::vector<node>& nodes_;

	size_t check_code(uint16_t x, std::vector<node> v, uint8_t nbits) {

		size_t pos = v.size() + 1;

		for (size_t i = 0; i < v.size(); ++i) {
			if (v[i].sym_ == x) {
				if (v[i].length_ == nbits) {
					pos = i;
					break;
				}
			}
		}
		return pos;
	}

	void readbit(uint64_t curbit) {
		buffer_ = (buffer_ << 1) | (curbit & 1);
		++n_;
		if (n_ == 8) {
			n_ = 0;
			is_.get(curr_ch);
			read_ = true;
		}
	}

public:
	bitreader(std::istream& is, std::ostream& os, std::vector<node>& v) : is_(is), os_(os), nodes_(v) {}

	~bitreader() {
		//os_ << buffer_ << std::dec << std::endl;
	}

	void set_notread() { read_ = false; }

	std::string readcode() {
		if (!read_) {
			is_.get(curr_ch);
			read_ = true;
		}
		uint64_t x = static_cast<uint64_t>(curr_ch);
		for (int bitnum = 7 - n_; bitnum >= 0; --bitnum) {
			readbit(x >> bitnum);
			++nbits_;
			if (nbits_ >= nodes_[0].length_) {
				size_t pos = check_code(buffer_, nodes_, nbits_);
				if (pos > nodes_.size()) {
					continue;
				}
				else {
					label_ = nodes_[pos].label_;
					os_ << nodes_[pos].label_;
					buffer_ = 0;
					nbits_ = 0;
					return label_;
				}
			}
		}
		if (n_ == 0) {
			this->readcode();
		}
		return label_;
	}

	uint16_t operator()(int numbits) {
		if (!read_) {
			is_.get(curr_ch);
			read_ = true;
		}
		uint64_t x = static_cast<uint64_t>(curr_ch);
		for (int bitnum = 7 - n_; bitnum >= 0; --bitnum) {
			readbit(x >> bitnum);
			++nbits_;
			if (nbits_ == numbits) {
				n_attr = buffer_;
				nbits_ = 0;
				buffer_ = 0;
				return n_attr;
			}
		}
		if (n_ == 0) {
			this->operator()(numbits);
		}
		return n_attr;
	}

};

uint16_t swap_endian(uint16_t value) {
	return (value >> 8) | (value << 8);
}

std::unordered_map<std::string, uint8_t> string_table(std::ifstream& is) {

	uint16_t N; //string number
	std::string s; //string
	uint8_t length; //huffman code length
	
	is.read(reinterpret_cast<char*>(&N), sizeof(N));
	N = swap_endian(N);

	std::unordered_map<std::string, uint8_t> table;
	char ch;

	for (uint16_t i = 0; i < N; ++i) {
		while (is.get(ch)) {
			if (ch == 0)
				break;
			s.push_back(ch);
		}
		is.get(ch);
		length = static_cast<uint8_t>(ch);

		table.emplace(s, length);
		s.clear();
	}
	return table;
}

std::vector<node> sortmap(std::unordered_map<std::string, uint8_t> m) {
	std::vector<node> nodes;
	node n;

	for (auto [label, len] : m) {
		n.label_ = label;
		n.length_ = len;
		nodes.push_back(n);
	}

	std::stable_sort(nodes.begin(), nodes.end(), [](node a, node b) { return a.length_ < b.length_; });

	return nodes;
}

bool check_code(uint8_t x, std::vector<node>& v) {
	
	for (node n : v) {
		if (n.sym_ == x)
			return true;
	}
	return false;
}

void generate_codes(std::vector<node>& v) {
	uint8_t buffer = 0, last_sym = 0;
	bool found = true;
	uint8_t offset = 1;
	
	for (int i = 1; i < v.size(); ++i) {
		v[i].sym_ = v[i - 1].sym_;
		if (v[i].length_ > v[i - 1].length_) {
			offset = v[i].length_ - v[i - 1].length_;
			while (found) {
				buffer = (v[i].sym_ >> offset);
				if (buffer > last_sym) {
					found = check_code(buffer, v);
				}
				if (found) {
					v[i].sym_ += 1;
				}
			}
			found = true;
			last_sym = v[i].sym_;
		}
		else {
			v[i].sym_ = v[i - 1].sym_ + 1;
			last_sym = v[i].sym_;
		}
	}
}

void decompress(std::ifstream& is, std::ofstream& os, std::vector<node>& nodes, bitreader& br, bool first_row) {
	std::string label1, label2;

	os << "<";
	label1 = br.readcode(); //read the first label in the xml file 

	uint16_t n_attr = br(4); //read 'n_attr' (4 bits)
	if (n_attr == 0) {
		if (first_row)
			os << ">\n";
		else
			os << ">";
	}
	else {
		os << " ";
	}
	
	for (uint8_t i = 0; i < n_attr; ++i) {
		br.readcode();
		os << "=\"";
		br.readcode();
		os << "\" ";
	}
	if(n_attr != 0)
		os << ">\n";

	uint16_t children = br(1); //children?

	if (children == 1) {
		os << "\n";
		uint16_t n_children = br(10);
		for (uint16_t i = 0; i < n_children; ++i) {
			decompress(is, os, nodes, br, false);
		}
	}
	else {
		uint16_t n_byte = br(10);
		char ch;
		std::streampos curpos = is.tellg();
		is.seekg(static_cast<int>(curpos) - 1);
		std::string stream;
		stream.resize(n_byte);
		is.read(stream.data(), n_byte * sizeof(char));
		os.write(stream.data(), n_byte * sizeof(char));
		br.set_notread();
	}
	os << "<" << label1 << "/>\n";
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		return EXIT_FAILURE;
	}
	std::ifstream is(argv[1], std::ios::binary);
	if (!is) {
		return EXIT_FAILURE;
	}
	std::ofstream os(argv[2], std::ios::binary);
	if (!os) {
		return EXIT_FAILURE;
	}
	std::unordered_map<std::string, uint8_t> table = string_table(is);
	std::vector<node> nodes = sortmap(table);
	generate_codes(nodes);

	os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	bitreader br(is, os, nodes);
	decompress(is, os, nodes, br, true);

	return EXIT_SUCCESS;
}