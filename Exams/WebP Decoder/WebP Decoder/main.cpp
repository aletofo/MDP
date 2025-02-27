#include <vector>
#include <string>
#include <unordered_map>
#include <ranges>
#include <iterator>
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

struct frequency {
	std::unordered_map<uint64_t, uint64_t> freq_;
	void operator() (uint64_t len) {
		freq_[len]++;
	}
};

struct huffman {
	struct node {
		uint64_t sym_;
		uint64_t freq_;
		node* left_;
		node* right_;
	};
	struct nodeptr_less {
		bool operator() (node* a, node* b) {
			return a->freq_ > b->freq_;
		}
	};

	std::vector<std::unique_ptr<node>> nodes_;

	template<typename... Ts>
	node* make_node(Ts... args) {
		nodes_.emplace_back(std::make_unique<node>(std::forward<Ts>(args)...));
		return nodes_.back().get();
	}

	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> code_map; // sym -> {code, len}
	std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> canonical; // {len, sym, code}

	void generate_codes(const node* n, uint64_t code = 0, uint64_t len = 0) {
		if (n->left_ == 0) {
			code_map[n->sym_] = { code, len };
		}
		else {
			generate_codes(n->left_, (code << 1) | 0, len + 1);
			generate_codes(n->right_, (code << 1) | 1, len + 1);
		}
	}

	void make_canonical() {
		canonical.clear();
		for (const auto& [sym, x] : code_map) {
			auto&& [code, len] = x;
			canonical.emplace_back(len, sym, code);
		}
		std::ranges::sort(canonical);
		uint64_t curcode = 0, curlen = 0;
		for (auto& [len, sym, code] : canonical) {
			uint64_t addbits = len - curlen;
			curcode <<= addbits;
			code_map[sym].first = curcode;
			code = curcode;
			curlen = len;
			++curcode;
		}
	}

	huffman() {}

	huffman(frequency f) {
		std::vector<node*> v;
		for (auto& [len, f] : f.freq_) {
			node* n = make_node(len, f);
			v.emplace_back(n);
		}
		std::ranges::sort(v, nodeptr_less{});

		while (v.size() > 1) {
			node* n1 = v.back();
			v.pop_back();
			node* n2 = v.back();
			v.pop_back();

			node* n = make_node(0, n1->freq_ + n2->freq_, n1, n2);
			auto pos = std::ranges::lower_bound(v, n, nodeptr_less{});
			v.insert(pos, n);
		}
		node* root = v.back();
		v.pop_back();

		generate_codes(root, 0, 0);
		make_canonical();
	}

	
};

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

std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> make_canonical(std::vector<std::pair<uint64_t, uint64_t>> map) {
	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> canonical_codes;
	uint64_t cur_len = 0;
	uint64_t cur_code = 0;

	std::pair<uint64_t, uint64_t> tmp;
	for (std::pair<uint64_t, uint64_t> p : map) {
		auto& [len, pos] = p;
		if (len == 0) {
			continue;
		}
		uint64_t addbits = len - cur_len;
		cur_code <<= addbits;
		cur_len = len;
		tmp = std::make_pair(cur_len, pos);
		canonical_codes.emplace(cur_code, tmp);
		cur_code++;
	}
	return canonical_codes;
}

uint64_t find_lenghtcode(std::ifstream& is, bitreader& br, std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> codes) {
	uint64_t len = std::get<0>(codes[0]);
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

void normal_code(bitreader& br, std::ifstream& is, int prefixcode, std::vector<std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>>& RGBA_hufmaps) {
	uint64_t num_code_lengths = br(4) + 4;

	int kCodeLengthCodes = 19;
	std::vector<int> kCodeLengthOrder = { 17,18,0,1,2,3,4,5,16,6,7,8,9,10,11,12,13,14,15 };
	std::vector<uint64_t> code_length_code_lengths;
	code_length_code_lengths.resize(19);
	std::fill(code_length_code_lengths.begin(), code_length_code_lengths.end(), 0); //all zeros
	std::vector<std::pair<uint64_t, uint64_t>> code_lengths_map;
	std::pair<uint64_t, uint64_t> p;

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

		std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> code_lengths_huf = make_canonical(code_lengths_map);

		std::vector<uint64_t> lengths;
		uint64_t last_len = 0;
		int counter = 0;
		for (uint64_t i = 0; i < max_symbol;) {
			uint64_t code = find_lenghtcode(is, br, code_lengths_huf);
			uint64_t len = std::get<1>(code_lengths_huf[code]);
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
		std::vector<std::pair<uint64_t, uint64_t>> codelengths;
		std::pair<uint64_t, uint64_t> p;
		uint64_t pos = 0;
		for (uint64_t len : lengths) {
			p = std::make_pair(len, pos);
			codelengths.push_back(p);
			++pos;
		}
		std::ranges::sort(codelengths);
		std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> lengths_huf = make_canonical(codelengths);
		
		std::cout << "\n" << is.tellg() << " - posbit: " << br.n_ << "\n";

		RGBA_hufmaps.push_back(lengths_huf);
	}
	else {
		uint64_t length_nbits = 2 + 2 * br(3);
		uint64_t max_symbol = 2 + br(static_cast<int>(length_nbits));

		std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> code_lengths_huf = make_canonical(code_lengths_map);
	}
	
	
}

void simple_code(bitreader& br, std::ifstream& is, std::vector<std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>>& RGBA_hufmaps) {
	uint64_t num_symbols = br(1) + 1;
	uint64_t is_first_8bits = br(1);

	uint64_t symbol0 = br(1 + 7 * is_first_8bits);

	std::vector<std::pair<uint64_t, uint64_t>> code_lengths;
	std::pair<uint64_t, uint64_t> p = std::make_pair(symbol0, 1);
	code_lengths.push_back(p);

	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> alpha;

	if (num_symbols == 2) {
		uint64_t symbol1 = br(8);
		std::pair<uint64_t, uint64_t> p = std::make_pair(symbol1, 1);
		code_lengths.push_back(p);
		alpha.emplace(0, code_lengths[0]);
		alpha.emplace(1, code_lengths[1]);
		RGBA_hufmaps.push_back(alpha);
	}
	else {
		alpha.emplace(0, code_lengths[0]);
		RGBA_hufmaps.push_back(alpha);
	}
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

	std::vector<std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>> RGBA_hufmaps;

	for (int i = 1; i <= 5; ++i) { //5 prefix codes
		uint64_t encoding = br(1);
		if (encoding == 0) { //normal code length code
			normal_code(br, is, i, RGBA_hufmaps);
		}
		else { //simple code length code
			simple_code(br, is, RGBA_hufmaps);
		}
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