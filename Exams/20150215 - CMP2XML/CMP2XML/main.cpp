#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <ranges>

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

	return EXIT_SUCCESS;
}