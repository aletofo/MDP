#include <cstdint>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>

void print_group(std::vector<std::vector<int>> group, int index) {
	std::cout << index << ") ";
	size_t seg_i = 0, group_i = 0;

	for (std::vector<int> seg : group) {
		std::cout << "[";
		for (int x : seg) {
			if (seg_i == seg.size() - 1) {
				std::cout << x << "]";
				break;
			}
			std::cout << x << ",";

			++seg_i;
		}
		if (group_i == group.size() - 1) {
			std::cout << "\n";
		}
		else {
			std::cout << ", ";
		}
		seg_i = 0;

		++group_i;
	}
}

void decompress(std::string s) {
	std::unordered_map<char, uint8_t> base64;
	std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	for (uint8_t i = 0; i < alphabet.size(); ++i) {
		base64.emplace(alphabet[i], i);
	}

	char ch;
	uint8_t val, tmp;
	std::vector<std::vector<int>> group;
	std::vector<int> segment;

	int group_counter = 0;

	for (size_t i = 0; i < s.size(); ++i) {
		ch = s[i];
		if (ch == ';') {
			if (!group.empty() || !segment.empty()) {
				group.push_back(segment);
				segment.clear();
				print_group(group, group_counter);
				group.clear();
			}

			++group_counter;
			continue;
		}
		if (ch == ',') {
			group.push_back(segment);
			segment.clear();
			continue;
		}
		val = base64[ch];
		tmp = val;
		//sign check
		bool negative;
		tmp = val << 7;
		tmp = tmp >> 7;
		if (tmp == 0)
			negative = false;
		else 
			negative = true;
		val = val >> 1;
		if (!negative)
			segment.push_back(static_cast<int>(val));
		else {
			int neg_val = static_cast<int>(val);
			segment.push_back(-neg_val);
		}
	}
	group.push_back(segment);
	print_group(group, group_counter);
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		return EXIT_FAILURE;
	}
	std::string s = argv[1];

	decompress(s);

	return EXIT_SUCCESS;
}