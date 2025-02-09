#include <algorithm>
#include <array>
#include <bit>
#include <format>
#include <fstream>
#include <iterator>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <random>

struct frequency {
	std::unordered_map<uint8_t, size_t> freq_;
	void operator() (uint8_t sym) {
		freq_[sym]++;
	}
};

template<typename T>
struct huffman {
	struct node {
		T sym_;
		size_t freq_;

		node* left_;
		node* right_;
	};
	std::vector<std::unique_ptr<node>> nodes_;

	struct nodeptr_less {
		bool operator() (node* a, node* b) {
			return a->freq_ > b->freq_;
		}
	};

	template<typename... Ts>
	node* make_node(T sym, size_t freq, node* l, node* r) {
		nodes_.emplace_back(std::make_unique<node>(sym, freq, l, r));
		return nodes_.back().get();
	}

	std::unordered_map<T, std::pair<size_t, size_t>> codes_map;
	void generate_codes(node* n, size_t code, size_t len = 0) {
		if (n->left_ == nullptr) {
			codes_map.emplace(n->sym_, std::make_pair(code, len));
		}
		else {
			generate_codes(n->left_, (code << 1) | 0, len + 1);
			generate_codes(n->right_, (code << 1) | 1, len + 1);
		}
	}

	std::vector<std::tuple<size_t, T, size_t>> canonical;
	std::unordered_map<T, std::pair<size_t, size_t>> canonical_codes_map;
	void make_canonical() {
		for (auto& [sym, pair] : codes_map) {
			auto [code, len] = pair;
			canonical.emplace_back(len, sym, code);
		}
		std::ranges::sort(canonical);

		size_t curr_code = 0, curr_len = 0;
		for (auto& [len, sym, code] : canonical) {
			size_t addbits = len - curr_len;
			curr_code <<= addbits;
			canonical_codes_map.emplace(sym, std::make_pair(curr_code, len));
			code = curr_code;
			curr_len = len;
			++curr_code;
		}
	}

	huffman() {}

	//template<std::ranges::range R>
	huffman(frequency f) {
		std::vector<node*> v;
		for (auto& [sym, counter] : f.freq_) {
			node* n = make_node(sym, counter, nullptr, nullptr);
			v.emplace_back(n);
		}
		std::ranges::sort(v, nodeptr_less{});

		while (v.size() > 1) {
			node* n1 = v.back();
			v.pop_back();
			node* n2 = v.back();
			v.pop_back();

			node *n = make_node(0, n1->freq_ + n2->freq_, n1, n2);
			auto pos = std::ranges::lower_bound(v, n, nodeptr_less{});
			v.insert(pos, n);
		}
		node* root = v.back();
		v.pop_back();

		std::cout << "HUFFMAN CODES:\n";
		generate_codes(root, 0, 0);
		for (auto& [sym, pair] : codes_map) {
			auto [code, len] = pair;
			std::cout << "<<" << sym << ">> -> code: " << code << " len: " << len << "\n";
		}
		std::cout << "CANONICAL HUFFMAN CODES:\n";
		make_canonical();
		for (auto& [sym, pair] : canonical_codes_map) {
			auto [code, len] = pair;
			std::cout << "<<" << sym << ">> -> code: " << code << " len: " << len << "\n";
		}
	}
};

void decompress(std::string input_file, std::string output_file) {

}

void compress(std::string input_file, std::string output_file) {
	std::ifstream is(input_file, std::ios::binary);
	std::vector<uint8_t> v{std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>()};

	auto f = std::ranges::for_each(v, frequency{}).fun;
	huffman<uint8_t> h(f);

}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		return EXIT_FAILURE;
	}

	std::string command = argv[1];
	if (command == "c") {
		compress(argv[2], argv[3]);
	}
	else if (command == "d") {
		decompress(argv[2], argv[3]);
	}
	else {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}