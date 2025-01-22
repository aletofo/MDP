#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <iterator>
#include <ranges>
#include <algorithm>

struct huffman {
	struct node {
		int num_;
		size_t freq_;
		node* left_;
		node* right_;
		uint8_t sym_;
		uint8_t symlen_;
	};

	struct nodeptr_less {
		bool operator()(node* a, node* b) {
			return a->freq_ > b->freq_;
		}
	};

	std::vector<std::unique_ptr<node>> nodes_;

	template<typename... Ts>
	node* make_node(Ts... args) {
		nodes_.emplace_back(std::make_unique<node>(std::forward<Ts>(args)...));
		return nodes_.back().get();
	}


	void generate_codes(node* n, uint8_t buffer, uint8_t len) {
		if (n->left_ != nullptr && n->right_ != nullptr) {
			generate_codes(n->left_, (buffer << 1), ++len);
			generate_codes(n->right_, (buffer << 1) | 1, ++len);
		}
		else {
			n->sym_ = buffer;
			n->symlen_ = len;
		}
	}

	std::vector<std::tuple<uint8_t, uint64_t, uint8_t>> getnodes() { 
		std::vector<std::tuple<uint8_t, uint64_t, uint8_t>> v;

		for (int i = 0; i < nodes_.size(); ++i) {
			if (nodes_[i]->num_ == 0) {
				continue;
			}
			auto t = std::make_tuple(nodes_[i]->symlen_, nodes_[i]->num_, nodes_[i]->sym_);
			v.push_back(t);
		}
		std::sort(v.begin(), v.end(), [](const auto& t1, const auto& t2) {
			return std::get<0>(t1) < std::get<0>(t2); // Confronta il primo valore
			});
		
		return v; 
	}

	huffman(std::unordered_map<int, size_t> f) {
		std::vector<node*> v;

		for (auto [key, freq] : f) {
			node* n = make_node(key, freq);
			v.push_back(n);
		}
		std::ranges::sort(v.begin(), v.end(), nodeptr_less{});

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
		generate_codes(root, 0, 0);
	}

};

int make_int(std::string s) {
	int value = 0;
	double n = 0, e, res;
	char ch;
	std::string num, exp;

	for (int i = 0; i < s.size(); ++i) {
		ch = s[i];
		if (ch == ',')
			break;
		if (ch == 'E' || ch == 'e') {
			n = std::stod(num);
			++i;
			for (int j = i; j < s.size(); ++j) {
				ch = s[j];
				exp.push_back(ch);
				++i;
			}
			
		}
		else {
			num.push_back(ch);
		}
	}
	e = std::stod(exp);
	
	res = n * (std::pow(10, e));
	res *= 16777215;
	value = static_cast<int>(std::floor(res));

	return value;
}

std::vector<int> read_inputfile(std::ifstream& is) {
	std::string s;
	std::vector<int> v;
	int value;

	while (!is.eof()) {
		is >> s;
		value = make_int(s);
		v.push_back(value);
	}

	return v;
}

std::unordered_map<int, size_t> frequencies(std::vector<int> v) {
	std::unordered_map<int, size_t> f;

	for (int x : v) {
		++f[x];
	}

	return f;
}

int main(int argc, char* argv[]) {
	if (argc != 3)
		return EXIT_FAILURE;
	std::ifstream is(argv[1]);
	if (!is)
		return EXIT_FAILURE;
	std::ofstream os(argv[2], std::ios::binary);
	if (!os)
		return EXIT_FAILURE;

	std::vector<int> v = read_inputfile(is);
	std::unordered_map<int, size_t> f = frequencies(v);
	huffman h(f);
	std::vector<std::tuple<uint8_t, uint64_t, uint8_t>> nodes = h.getnodes();

	for (std::tuple<uint8_t, uint64_t, uint8_t> t : nodes) {
		os << std::get<0>(t);
		os << std::get<1>(t);
	}

	return EXIT_SUCCESS;
}