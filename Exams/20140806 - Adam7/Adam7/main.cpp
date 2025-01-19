#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <cstdint>

class PGM {
	size_t width_, height_;
	std::vector<uint8_t> data_;
	std::ifstream& is_;
	std::ofstream& os_;
	std::vector<std::vector<uint8_t>> levels_;
	std::vector<uint8_t> pattern_ = { 1,6,4,6,2,6,4,6,
									7,7,7,7,7,7,7,7,
									5,6,5,6,5,6,5,6,
									7,7,7,7,7,7,7,7,
									3,6,4,6,3,6,4,6,
									7,7,7,7,7,7,7,7,
									5,6,5,6,5,6,5,6,
									7,7,7,7,7,7,7,7 };


public:
	PGM(size_t w, size_t h, std::ifstream& is, std::ofstream& os) : width_(w), height_(h), is_(is), os_(os), data_(w * h) {
		levels_.resize(8);
	}

	size_t width() { return width_; }
	size_t height() { return height_; }
	uint8_t getpixel(size_t r, size_t c) {
		return data_[r * width_ + c];
	}

	void filldata() {
		uint8_t byte;
		char ch;

		for (size_t r = 0; r < height_; ++r) {
			for (size_t c = 0; c < width_; ++c) {
				is_.get(ch);
				byte = static_cast<uint8_t>(ch);

				data_[r * width_ + c] = byte;
			}
		}
	}

	void notfullblock() {
		uint8_t curpattern;
		uint8_t byte;

		for (size_t r = 0; r < height_; ++r) {
			for (size_t c = 0; c < width_; ++c) {
				byte = getpixel(r, c);
				curpattern = pattern_[r * 8 + c];

				levels_.at(curpattern).push_back(byte);
			}
		}
	}

	void readblock() {
		uint8_t curpattern;
		uint8_t byte;
		size_t i = 0, j = 0;
		//char ch;

		
			for (size_t r = 0; r < height_; ++r) {
				if (j == 8) {
					j = 0;
				}
				for (size_t c = 0; c < width_; ++c) {
					if (i == 8) {
						i = 0;
					}
					byte = getpixel(r, c);
					curpattern = pattern_[j * 8 + i];

					levels_.at(curpattern).push_back(byte);
					++i;
				}
				++j;
			}
	}

	void writeblock() {

	}

	void readlevels() {

	}

	void writelevels() {
		uint8_t x;

		for (size_t i = 1; i < 8; ++i) {
			for (size_t j = 0; j < levels_[i].size(); ++j) {
				x = levels_[i][j];
				os_.put(x);
			}
		}
	}
};

PGM header(std::ifstream& is, std::ofstream& os) {

	os << "MULTIRES";

	// Leggi il magic number
	std::string magic_number;
	is >> magic_number;

	// Salta i commenti e leggi width e height
	std::string line;
	while (is.peek() == '#') { // Salta le righe di commento
		std::getline(is, line);
	}

	// Leggi width e height
	size_t width, height;
	is >> width >> height;

	// Scrivi width e height in formato little-endian nel file di output
	uint32_t width_le = static_cast<uint32_t>(width);
	uint32_t height_le = static_cast<uint32_t>(height);

	os.write(reinterpret_cast<const char*>(&width_le), sizeof(width_le));
	os.write(reinterpret_cast<const char*>(&height_le), sizeof(height_le));

	PGM pgm(width, height, is, os);

	std::string maxval;
	is >> maxval;
	is.get();

	return pgm;
}



void decompress(std::ifstream& is, std::string s) {

	size_t w = 0, h = 0;
	char ch;
	size_t x = 1;
	std::vector<std::vector<uint8_t>> levels;
	std::vector<uint8_t> pattern = { 1,6,4,6,2,6,4,6,
									7,7,7,7,7,7,7,7,
									5,6,5,6,5,6,5,6,
									7,7,7,7,7,7,7,7,
									3,6,4,6,3,6,4,6,
									7,7,7,7,7,7,7,7,
									5,6,5,6,5,6,5,6,
									7,7,7,7,7,7,7,7 };

	for (int i = 0; i < 8; ++i) {
		is.get();
	}
	for (int i = 0; i < 4; ++i) {
		is.get(ch);
		w += x * static_cast<uint8_t>(ch);
		x *= 10;
	}
	x = 1;
	for (int i = 0; i < 4; ++i) {
		is.get(ch);
		h += x * static_cast<uint8_t>(ch);
		x *= 10;
	}
	uint8_t value, pat_value, i = 0, j = 0;
	for (size_t r = 0; r < h; ++r) {
		if (i == 8) {
			j = 0;
		}
		for (size_t c = 0; c < w; ++c) {
			if (i == 8) {
				i = 0;
			}
			is.get(ch);
			value = static_cast<uint8_t>(ch);
			pat_value = pattern[j * 8 + i];
			levels.at(pat_value).push_back(value);
			++i;
		}
		++j;
	}

	for (char i = 0; i < 8; ++i) {
		s.push_back('_');
		s.push_back(i);
		s += ".pgm";
		std::ofstream os(s, std::ios::binary);
		
		PGM pgm(w, h, is, os);
	}

}
void compress(std::ifstream& is, std::ofstream& os) {

	PGM pgm = header(is, os);
	pgm.filldata();

	if (pgm.width() * pgm.height() >= 64) {
		pgm.readblock();
	}
	else {
		pgm.notfullblock();
	}
	pgm.writelevels();
}

int main(int argc, char* argv[]) {

	if (argc != 4) {
		return EXIT_FAILURE;
	}

	char ch = *argv[1];
	if (ch == 'c') {
		std::ifstream is(argv[2], std::ios::binary);
		if (!is) {
			return EXIT_FAILURE;
		}
		std::ofstream os(argv[3], std::ios::binary);
		if (!os) {
			return EXIT_FAILURE;
		}
		compress(is, os);
	}
	if (ch == 'd') {
		std::ifstream is(argv[2], std::ios::binary);
		if (!is) {
			return EXIT_FAILURE;
		}
		std::string s = argv[3];
		decompress(is, s);
	}

	return EXIT_SUCCESS;
}