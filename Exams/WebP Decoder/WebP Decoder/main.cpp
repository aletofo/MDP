#include <vector>
#include <string>
#include <unordered_map>
#include <ranges>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>

struct bitreader {
	uint8_t buffer_;
	int n_ = 0;
	char cur_ch;
	std::ifstream& is_;

	uint64_t readbit() { //least significant bit first
		if (n_ == 0) {
			is_.get(cur_ch);
			buffer_ = static_cast<uint8_t>(cur_ch);
			n_ = 8;
		}
		--n_;
		uint64_t tmp = buffer_ << n_;
		tmp = tmp >> 7;
		return tmp & 1;
	}

public:
	bitreader(std::ifstream& is) : is_(is){}

	uint64_t operator() (int numbits) {
		int nbits = numbits;
		uint64_t value = 0;
		while (--numbits >= 0) {
			value = (value >> 1) | (readbit() << 63); //shift 63 bits to encounter the exact bit I'm inserting from the right in 'value'
		}
		value >>= (64 - nbits);
		return value;
	}
	uint64_t read_code (int numbits) {
		uint64_t value = 0;
		while (--numbits >= 0) {
			value = (value << 1) | readbit(); //shift 63 bits to encounter the exact bit I'm inserting from the right in 'value'
		}
		return value;
	}

	void reset(std::streampos startpos, int start_n) {
		char ch;

		n_ = start_n;
		is_.seekg(static_cast<int>(startpos) - 1);
		is_.get(ch);
		buffer_ = static_cast<uint8_t>(ch);
	}
};

struct PAM {
	size_t width_, height_;
	std::vector<uint8_t> data_;
	std::ifstream& is_;

	PAM(std::ifstream& is, size_t w, size_t h) : is_(is), width_(w), height_(h), data_(w * h){}


};
/*
struct huffman {
	struct node {
		uint64_t sym_;
		uint64_t freq_;
		node* left_;
		node* right_;
	};
	struct nodeptr_less {
		bool operator() (node* a, node* b) {
			return a->freq_ < b->freq_;
		}
	};

	void generate_codes(node* n, uint64_t code, uint64_t len) {

	}

	std::unordered_map<std::pair<uint64_t, int>, uint64_t> canonical_codes_;
	void make_canonical(std::vector<std::pair<uint64_t, int>> map) {
		uint64_t cur_len = 0;
		uint64_t cur_code = 0;
		std::pair<uint64_t, int> tmp;
		for (std::pair<uint64_t, int> p : map) {
			auto& [len, pos] = p;
			uint64_t addbits = len - cur_len;
			cur_code <<= addbits;
			cur_len = len;
			tmp = std::make_pair(cur_len, pos);
			canonical_codes_.emplace(tmp, cur_code);
			cur_code++;
		}
	}

	huffman() {}

	
};
*/
bool webPhdr(std::ifstream& is) {
	std::string token;
	char ch;

	for (int i = 0; i < 4; ++i) {
		is.get(ch);
		token.push_back(ch);
	}
	if (token != "RIFF")
		return false;
	token.clear();

	uint32_t chunk_size;
	is.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));

	for (int i = 0; i < 4; ++i) {
		is.get(ch);
		token.push_back(ch);
	}
	if (token != "WEBP")
		return false;
	token.clear();
	for (int i = 0; i < 4; ++i) {
		is.get(ch);
		token.push_back(ch);
	}
	if (token != "VP8L")
		return false;
	token.clear();

	uint32_t stream_bytes;
	is.read(reinterpret_cast<char*>(&stream_bytes), sizeof(stream_bytes));

	uint8_t signature;
	is.read(reinterpret_cast<char*>(&signature), sizeof(signature));
	if (signature != 0x2F)
		return false;

	return true;
}

std::unordered_map<uint64_t, std::pair<uint64_t, int>> make_canonical(std::vector<std::pair<uint64_t, int>> map) {
	std::unordered_map<uint64_t, std::pair<uint64_t, int>> canonical_codes;
	uint64_t cur_len = 0;
	uint64_t cur_code = 0;

	std::pair<uint64_t, int> tmp;
	for (std::pair<uint64_t, int> p : map) {
		auto& [len, pos] = p;
		uint64_t addbits = len - cur_len;
		cur_code <<= addbits;
		cur_len = len;
		tmp = std::make_pair(cur_len, pos);
		canonical_codes.emplace(cur_code, tmp);
		cur_code++;
	}
	return canonical_codes;
}

uint64_t find_lenghtcode(std::ifstream& is, bitreader& br, std::unordered_map<uint64_t, std::pair<uint64_t, int>> codes) {
	int len = static_cast<int>(std::get<0>(codes[0]));
	auto startpos = is.tellg();
	int start_n = br.n_;
	uint64_t code = br.read_code(len);

	while (1) {
		if (codes.contains(code)) {
			if (std::get<0>(codes[code]) == len) {
				break;
			}
		}
		++len;
		br.reset(startpos, start_n);
		code = br.read_code(len);
	}

	return code;
}

void normal_code(bitreader& br, std::ifstream& is, int prefixcode) {
	uint64_t num_code_lengths = br(4) + 4;

	int kCodeLengthCodes = 19;
	std::vector<int> kCodeLengthOrder = { 17,18,0,1,2,3,4,5,16,6,7,8,9,10,11,12,13,14,15 };
	std::vector<uint64_t> code_length_code_lengths;
	code_length_code_lengths.resize(19);
	std::fill(code_length_code_lengths.begin(), code_length_code_lengths.end(), 0); //all zeros
	std::vector<std::pair<uint64_t, int>> code_lengths_map;
	std::pair<uint64_t, int> p;

	for (size_t i = 0; i < num_code_lengths; ++i) {
		code_length_code_lengths[kCodeLengthOrder[i]] = br(3);
		if (code_length_code_lengths[kCodeLengthOrder[i]] != 0) {
			p = std::make_pair(code_length_code_lengths[kCodeLengthOrder[i]], kCodeLengthOrder[i]);
			code_lengths_map.push_back(p);
		}
	}
	std::ranges::sort(code_lengths_map);

	uint64_t check_maxsym = br(1);
	if (check_maxsym == 0) {
		uint64_t max_symbol;
		if (prefixcode == 1)
			max_symbol = 256 + 24 + 0;
		else if (prefixcode == 5)
			max_symbol = 40;
		else
			max_symbol = 256;

		std::unordered_map<uint64_t, std::pair<uint64_t, int>> code_lengths_huf = make_canonical(code_lengths_map);

		std::vector<int> lengths;
		int last_len = 0;
		int counter = 0;
		for (uint64_t i = 0; i < max_symbol;) {
			uint64_t code = find_lenghtcode(is, br, code_lengths_huf);
			int len = std::get<1>(code_lengths_huf[code]);
			if (len <= 15) {
				lengths.push_back(len);
				last_len = len;
				std::cout << last_len << "-";
				++i;
			}
			if (len == 16) {
				uint64_t copy = 3 + br(2);
				for (uint64_t j = 0; j < copy; ++j) {
					lengths.push_back(last_len);
					std::cout << last_len << "-";
					++i;
				}
			}
			if (len == 17) {
				uint64_t copy = 3 + br(3);
				for (uint64_t j = 0; j < copy; ++j) {
					lengths.push_back(0);
					std::cout << 0 << "-";
					++i;
				}
			}
			if (len == 18) {
				uint64_t copy = 11 + br(7);
				for (uint64_t j = 0; j < copy; ++j) {
					lengths.push_back(0);
					std::cout << 0 << "-";
					++i;
				}
			}
		}
	}
	else {
		uint64_t length_nbits = 2 + 2 * br(3);
		uint64_t max_symbol = 2 + br(static_cast<int>(length_nbits));

		std::unordered_map<uint64_t, std::pair<uint64_t, int>> code_lengths_huf = make_canonical(code_lengths_map);
	}
	
	
}

void simple_code(bitreader& br, std::ifstream& is) {
	uint64_t num_symbols = br(1) + 1;
	uint64_t is_first_8bits = br(1);

	uint64_t symbol0 = br(1 + 7 * is_first_8bits);
}

void decompress(std::ifstream& is, std::ofstream& os) {
	if (!webPhdr(is)) {
		return;
	}
	bitreader br(is);

	uint64_t width = br(14) + 1, height = br(14) + 1;
	uint64_t alpha_used = br(1);
	uint64_t version = br(3);
	if (alpha_used != 0 && version != 0) {
		return;
	}
	uint64_t check_bits = br(3);
	if (check_bits != 0) {
		return;
	}
	uint64_t encoding = br(1);
	if (encoding == 0) { //normal code length code
		normal_code(br, is, 1);
	}
	else { //simple code length code
		simple_code(br, is);
	}


}

int main(int argc, char* argv[]) {
	if (argc != 3)
		return EXIT_FAILURE;
	std::ifstream is(argv[1], std::ios::binary);
	if (!is)
		return EXIT_FAILURE;
	std::ofstream os(argv[2], std::ios::binary);
	if (!os)
		return EXIT_FAILURE;

	decompress(is, os);

	return EXIT_SUCCESS;
}