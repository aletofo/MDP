#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

size_t preamble(std::ifstream& is) {
	size_t streamsize;
	char ch;
	unsigned char byte, firstbit, tmp;
	std::vector<unsigned char> buf;

	is.get(ch);
	byte = static_cast<unsigned char>(ch);
	firstbit = byte >> 7;

	if (firstbit == 0) {
		streamsize = byte;
	}
	else {
		while (firstbit == 1) {
			tmp = byte << 1;
			byte = tmp >> 1;
			buf.push_back(byte);

			is.get(ch);
			byte = static_cast<unsigned char>(ch);
			firstbit = byte >> 7;
		}
		buf.push_back(byte);
	}
	std::string sizestr;
	for (int i = buf.size() - 1; i >= 0; --i) {
		char buffer[3];
		std::snprintf(buffer, sizeof(buffer), "%02X", buf[i]);
		sizestr += buffer;
	}
	streamsize = std::stoul(sizestr, nullptr, 16);

	return streamsize;
}

uint8_t tag(uint8_t x) {
	uint8_t tmp = x << 6;
	x = tmp >> 6;

	return x;
}

void literal(std::ifstream& is, std::vector<char>& v, uint8_t value) {
	if (value < 60) {
		uint8_t length = value + 1;
		char ch;
		for (size_t i = 0; i < length; ++i) {
			is.get(ch);
			v.push_back(ch);
		}
	}
	else {
		char ch;
		uint32_t length, tmp;
		if (value == 60) {
			is.read(reinterpret_cast<char*>(&length), sizeof(uint8_t));
			tmp = length << 24;
			length = tmp >> 24;
			for (size_t i = 0; i < length + 1; ++i) {
				is.get(ch);
				v.push_back(ch);
			}
		}
		if (value == 61) {
			is.read(reinterpret_cast<char*>(&length), 2 * sizeof(uint8_t));
			tmp = length << 16;
			length = tmp >> 16;
			for (size_t i = 0; i < length + 1; ++i) {
				is.get(ch);
				v.push_back(ch);
			}
		}
		if (value == 62) {
			is.read(reinterpret_cast<char*>(&length), 3 * sizeof(uint8_t));
			tmp = length << 8;
			length = tmp >> 8;
			for (size_t i = 0; i < length + 1; ++i) {
				is.get(ch);
				v.push_back(ch);
			}
		}
		if (value == 63) {
			is.read(reinterpret_cast<char*>(&length), 4 * sizeof(uint8_t));
			for (size_t i = 0; i < length + 1; ++i) {
				is.get(ch);
				v.push_back(ch);
			}
		}
	}
}

void write_copy(std::ifstream& is, std::vector<char>& v, size_t length, size_t offset, uint8_t offset_bytes) {
	
	size_t curpos = v.size() - offset;

	if (offset >= length) {
		for (size_t i = curpos; i < curpos + length; ++i) {
			v.push_back(v[i]);
		}
	}
	else {
		size_t j = curpos;
		for (size_t i = 0; i < length; ++i) {
			if (i != 0) {
				if (i % offset == 0) {
					j = curpos;
				}
			}
			v.push_back(v[j]);
			++j;
		}
	}
}

void copy(std::ifstream& is, std::vector<char>& bytes, uint8_t offset_bytes, uint8_t value) {
	char ch;
	std::string offset_str;

	if (offset_bytes == 1) {
		uint8_t length, tmp = value << 5;
		uint16_t offset;

		length = tmp >> 5;
		length += 4;
		offset = value >> 3;

		offset_str.push_back(static_cast<char>(offset));
		is.get(ch);
		offset_str.push_back(ch);

		std::string s;
		for (unsigned char c : offset_str) {
			char buffer[3];
			std::snprintf(buffer, sizeof(buffer), "%02X", c);
			s += buffer;
		}
		offset = static_cast<uint16_t>(std::stoul(s, nullptr, 16));

		write_copy(is, bytes, length, offset, offset_bytes + 1);
	}
	else if (offset_bytes == 2) {
		uint8_t length = value + 1;
		uint16_t offset;

		is.read(reinterpret_cast<char*>(&offset), sizeof(offset));

		write_copy(is, bytes, length, offset, offset_bytes + 1);
	}
	else if (offset_bytes == 4) {
		uint8_t length = value + 1;
		uint32_t offset;

		is.read(reinterpret_cast<char*>(&offset), sizeof(offset));

		write_copy(is, bytes, length, offset, offset_bytes + 1);
	}
}

void decode(std::ifstream& is, std::ofstream& os) {
	char ch;
	uint8_t byte, type;
	std::string s;
	std::vector<char> bytes;

	is.get(ch);

	while (!is.eof()) {		
		auto curpos = is.tellg();
		byte = static_cast<uint8_t>(ch);
		type = tag(byte);

		if (type == 0) {
			literal(is, bytes, byte >> 2);
		}
		else if (type == 1) {
			copy(is, bytes, 1, byte >> 2);
		}
		else if (type == 2) {
			copy(is, bytes, 2, byte >> 2);
		}
		else if (type == 3) {
			copy(is, bytes, 4, byte >> 2);
		}
		
		is.get(ch);
	}

	os.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
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

	size_t size = preamble(is);
	decode(is, os);

	return EXIT_SUCCESS;
}