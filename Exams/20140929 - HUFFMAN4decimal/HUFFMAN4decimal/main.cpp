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

template<typename T>
struct huffman_tree {
	struct node {
		T sym_;
		int num_;
		size_t freq_;
		node* left_;
		node* right_;
	};

	template<typename... Ts>
	node* make_node(Ts... args) {
		node* n;
	}

	void operator() (std::unordered_map<int, size_t> f) {
		for (auto [key, value] : f) {
			node* n = make_node(key, value);
		}
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

	return EXIT_SUCCESS;
}