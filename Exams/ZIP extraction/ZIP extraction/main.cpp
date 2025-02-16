#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdint>

void extract_file(std::ifstream& is, uint32_t hdr_offset) {
	is.seekg(hdr_offset);

	uint32_t signature;
	is.read(reinterpret_cast<char*>(&signature), sizeof(signature));
	if (signature != 67324752) {
		std::cout << "ERROR in the local file header position.\n";
		return;
	}
	is.ignore(14 * sizeof(char));

	uint32_t compr_size, uncompr_size;
	is.read(reinterpret_cast<char*>(&compr_size), sizeof(compr_size));
	is.read(reinterpret_cast<char*>(&uncompr_size), sizeof(uncompr_size));

	uint16_t filename_size, extraf_size;
	is.read(reinterpret_cast<char*>(&filename_size), sizeof(filename_size));
	is.read(reinterpret_cast<char*>(&extraf_size), sizeof(extraf_size));

	std::string filename;
	char ch;
	for (int i = 0; i < filename_size; ++i) {
		is.get(ch);
		filename.push_back(ch);
	}

	std::ofstream os(filename, std::ios::binary);
	for (uint32_t i = 0; i < uncompr_size; ++i) {
		is.get(ch);
		os.put(ch);
	}
}

void filehdr(std::ifstream& is) {
	char ch;

	is.ignore(4 * sizeof(char)); //signature
	is.ignore(6 * sizeof(char));

	uint16_t compression;
	is.read(reinterpret_cast<char*>(&compression), sizeof(compression));

	is.ignore(8 * sizeof(char));

	uint32_t compr_size, uncompr_size;
	is.read(reinterpret_cast<char*>(&compr_size), sizeof(compr_size));
	is.read(reinterpret_cast<char*>(&uncompr_size), sizeof(uncompr_size));

	uint16_t filename_size, extraf_size, comment_size;
	is.read(reinterpret_cast<char*>(&filename_size), sizeof(filename_size));
	is.read(reinterpret_cast<char*>(&extraf_size), sizeof(extraf_size));
	is.read(reinterpret_cast<char*>(&comment_size), sizeof(comment_size));

	is.ignore(8 * sizeof(char));

	uint32_t localfhdr_offset;
	is.read(reinterpret_cast<char*>(&localfhdr_offset), sizeof(localfhdr_offset));

	std::cout << "FILE: ";
	for (int i = 0; i < filename_size; ++i) {
		is.get(ch);
		std::cout << ch;
	}
	std::cout << "\n";

	is.ignore((extraf_size + comment_size) * sizeof(char));

	auto curpos = is.tellg();

	if (compression == 0) {
		extract_file(is, localfhdr_offset);
	}

	is.seekg(curpos);
}

void decompress(std::ifstream& is) {
	char ch;
	is.seekg(0, std::ios::end);
	auto endfile = is.tellg();
	uint32_t EOCD = 0;
	int off = 0;

	while (EOCD != 101010256) {
		is.seekg(static_cast<int>(endfile) - 22 - off);
		is.read(reinterpret_cast<char*>(&EOCD), sizeof(EOCD));
		++off;
	}
	is.ignore(8 * sizeof(char));
	uint32_t CDsize, startCD_offset;
	is.read(reinterpret_cast<char*>(&CDsize), sizeof(CDsize));
	is.read(reinterpret_cast<char*>(&startCD_offset), sizeof(startCD_offset));

	uint16_t comment_len;
	is.read(reinterpret_cast<char*>(&comment_len), sizeof(comment_len));
	std::cout << "COMMENT: ";
	for (int i = 0; i < comment_len; ++i) {
		is.get(ch);
		std::cout << ch;
	}
	std::cout << "\n";
	
	is.seekg(startCD_offset);

	while (is.tellg() != startCD_offset + CDsize) {
		filehdr(is);
	}
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		return EXIT_FAILURE;
	}
	std::ifstream is(argv[1], std::ios::binary);
	if (!is) {
		return EXIT_FAILURE;
	}

	decompress(is);

	return EXIT_SUCCESS;
}