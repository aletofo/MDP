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
			if (!std::isalpha(ch))
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

bool check(uint8_t x, std::vector<node>& v) {
	
	for (node n : v) {
		if (n.sym_ == x)
			return true;
	}
	return false;
}

void generate_codes(std::vector<node>& v) {
	uint8_t buffer = 0;
	bool found = true;
	//(v[i].sym_ >> (v[i - 1].length_ - 1))
	

	for (int i = 1; i < v.size(); ++i) {
		if (v[i].length_ > v[i - 1].length_) {
			while (found) {
				buffer = (v[i].sym_ >> 1);
				found = check(buffer, v);
				if(found)
					v[i].sym_ += 1;
			}
		}
		else {
			v[i].sym_ = v[i - 1].sym_ + 1;
		}
	}
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

	return EXIT_SUCCESS;
}