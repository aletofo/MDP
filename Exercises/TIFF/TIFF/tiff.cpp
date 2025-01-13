
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <format>

/*
template<typename T>
struct mat {
	int64_t rows_;
	int64_t cols_;
	int64_t depth_;
	int64_t size_;
	std::vector<T> data_;

	int64_t rows() const { return rows_; }
	int64_t cols() const { return cols_; }
	int64_t depth() const { return depth_; }
	int64_t size() const { return size_; }

	mat(int64_t rows = 0, int64_t cols = 0, int64_t depth = 0) :
		rows_(rows), cols_(cols), depth_(depth),
		data_(rows * cols * depth) {}

	auto rawsize() const { return size() * sizeof(T); }
	auto rawdata() {
		return reinterpret_cast<char*>(data_.data());
	}
	auto rawdata() const {
		return reinterpret_cast<const char*>(data_.data());
	}
};
*/

template<typename T>
std::istream& rawread(std::istream& is, T& val, size_t size = sizeof(T)) {
	return is.read(reinterpret_cast<char*>(&val), size);
}
std::istream& rawread(std::istream& is, std::string& val, size_t size) {
	return is.read(val.data(), size);
}

void check_tag(std::ifstream& is, uint16_t tag) {
	switch (tag) {
	case :
	}	
}

void read_IFD(std::ifstream& is) {

	uint16_t entries;
	rawread(is, entries, 2);

	uint16_t tag, type;
	uint32_t v_count, v_offset;


	for (size_t i = 0; i < entries; ++i) {
		rawread(is, tag, 2);
		check_tag(is, tag);

		rawread(is, type, 2);
		rawread(is, v_count, 4);
		rawread(is, v_offset, 4);
	}

	uint32_t new_pos;
	rawread(is, new_pos, 32);
	if (new_pos != 0) {
		is.seekg(new_pos);
		read_IFD(is);
	}
}

uint32_t read_header(std::ifstream& is) {

	std::string order(2, ' ');
	rawread(is, order, 2);
	if (order != "II")
		std::cout << "ERROR: not Little Endian.\n";
	uint16_t num;
	rawread(is, num, 2);
	if (num != 42) {
		std::cout << "ERROR: not 42.\n";
	}
	uint32_t pos;
	rawread(is, pos, 4);

	return pos;
}

int main(int argc, char* argv[]) {

	if (argc != 3) {
		std::cout << "ERROR: invalid number of parameters.\n";
		return EXIT_FAILURE;
	}
	std::ifstream is(argv[1], std::ios::binary);
	if (!is) {
		std::cout << "ERROR: impossible to open the input file.\n";
		return EXIT_FAILURE;
	}
	std::ofstream os(argv[2], std::ios::binary);
	if (!os) {
		std::cout << "ERROR: impossible to open the output file.\n";
		return EXIT_FAILURE;
	}

	uint32_t pos = read_header(is);

	is.seekg(pos);

	read_IFD(is);

	return EXIT_SUCCESS;
}



/*
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cstdint>
#include <format>
#include <print>

#define print(...) cout << std::format(__VA_ARGS__)
#define println(...) cout << std::format(__VA_ARGS__) << "\n"

void check(bool cond, std::string_view s) {
	if (!cond) {
		std::println("{}", s);
		exit(-1);
	}
}

template<typename T>
std::istream& rawread(std::istream& is, T& val, size_t size = sizeof(T)) {
	return is.read(reinterpret_cast<char*>(&val), size);
}
std::istream& rawread(std::istream& is, std::string& val, size_t size) {
	return is.read(val.data(), size);
}

std::unordered_map<uint16_t, std::string> tag_names = {
	{ 262, "PhotometricInterpretation" },
	{ 259, "Compression" },
	{ 257, "ImageLength" },
	{ 256, "ImageWidth" },
	{ 296, "ResolutionUnit" },
	{ 282, "XResolution" },
	{ 283, "YResolution" },
	{ 278, "RowsPerStrip" },
	{ 273, "StripOffsets" },
	{ 279, "StripByteCounts" },
	{ 258, "BitsPerSample" },
	{ 270, "ImageDescription" },
	{ 277, "SamplesPerPixel" },
	{ 284, "PlanarConfiguration" },
	{ 305, "Software" },
};

std::unordered_map<uint16_t, std::pair<std::string, uint16_t>> type_names = {
	{1, {"BYTE", 1} },
	{2, {"ASCII", 1} },
	{3, {"SHORT", 2} },
	{4, {"LONG", 4} },
	{5, {"RATIONAL", 8} },
};

int main(int argc, char* argv[])
{
	check(argc == 3, "Numero argomenti errato");

	std::ifstream is(argv[1], std::ios::binary);
	check(bool(is), "Impossibile aprire il file di input");

	// header
	{
		std::string order(2, ' ');
		rawread(is, order, 2);
		check(order == "II", "L'header non comincia con II");
		uint16_t num;
		rawread(is, num);
		check(num == 42, "L'header non contiene 42");
		uint32_t offset;
		rawread(is, offset);
		is.seekg(offset);
	}

	// IFD
	struct entry {
		uint16_t tag, type;
		uint32_t count;
		uint32_t offset;

		entry(std::istream& is) {
			rawread(is, tag);
			rawread(is, type);
			rawread(is, count);
			rawread(is, offset);
		}
	};
	std::vector<entry> IFD;
	{
		uint16_t num;
		rawread(is, num);
		for (uint16_t i = 0; i < num; ++i) {
			IFD.emplace_back(is);
		}
		uint32_t next;
		rawread(is, next);
		check(next == 0, "Sono presenti pi� IFD nel file");
	}

	size_t w, h, strip_offset;

	for (const auto& x : IFD) {
		{
			auto it = tag_names.find(x.tag);
			if (it != tag_names.end()) {
				std::print("{}: {}, ", x.tag, it->second);
			}
			else {
				std::print("{}: unknown, ", x.tag);
			}
		}
		{
			auto it = type_names.find(x.type);
			if (it != type_names.end()) {
				std::print("{}, ", it->second.first);
				std::print("count={}, ", x.count);
				if (x.count * it->second.second <= 4) {
					std::println("value={}", x.offset);
				}
				else {
					std::println("offset={}", x.offset);
				}
			}
			else {
				std::print("unknown type ({}), ", x.type);
				std::println("count={}, offset={}", x.count, x.offset);
			}
		}

		switch (x.tag)
		{
		break; case 256: w = x.offset;
		break; case 257: h = x.offset;
		break; case 258: check(x.offset == 8, "BitsPerSample != 8");
		break; case 259: check(x.offset == 1, "Compression != 1");
		break; case 262: check(x.offset == 1, "PhotometricInterpretation != 1");
		break; case 273: check(x.count == 1, "multiple StripOffsets");
					   strip_offset = x.offset;
		break; case 277: check(x.offset == 1, "SamplesPerPixel != 1");
		break; case 284: check(x.offset == 1, "PlanarConfiguration != 1");
		}
	}

	std::string data;
	data.resize(w * h);
	is.seekg(strip_offset);
	rawread(is, data, w * h);

	std::ofstream os(argv[2], std::ios::binary);
	check(bool(os), "Impossibile aprire il file di output");
	os << std::format(
		"P7\n"
		"WIDTH {}\n"
		"HEIGHT {}\n"
		"DEPTH 1\n"
		"MAXVAL 255\n"
		"TUPLTYPE GRAYSCALE\n"
		"ENDHDR\n", w, h);
	os.write(data.data(), w * h);

	return 0;
}
*/