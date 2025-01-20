#include <fstream>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>

class PAMimg {
	size_t width_, height_, bpp_, num_colors_;
	std::vector<uint8_t> data_;
	std::ifstream& is_;
	std::ofstream& os_;

public:
	PAMimg(size_t w, size_t h, std::ifstream& is, std::ofstream& os) : width_(w * 3), height_(h), is_(is), os_(os), data_((w * 3) * h) {}

	void setbpp(size_t x) { bpp_ = x; }
	void setnumcolors(size_t x) { num_colors_ = x; }

	size_t size() const { return width_ * height_; }
	size_t width() const { return width_ / 3; }
	size_t height() const { return height_; }

	void filldata() {
		char ch;
		uint8_t byte;
		size_t pad = width_ * 8;
		while (pad % 32 != 0) {
			++pad;
		}
		size_t remainder = (pad - width_ * 8) / 8;

		for (int r = static_cast<int>(height_) - 1; r >= 0; --r) {
			for (int c = 0; c < width_; c += 3) {
				for (int channel = 2; channel >= 0; --channel) {
					is_.get(ch);
					byte = static_cast<uint8_t>(ch);
					
					data_[r * width_ + c + channel] = byte;
				}
			}
			for (size_t i = 0; i < remainder; ++i) {
				is_.get();
			}
		}
	}

	auto rawsize() const { return size() * sizeof(uint8_t); }
	auto rawdata() {
		return reinterpret_cast<char*>(data_.data());
	}
};

size_t str2ul(char buffer[], int range) {
	std::string s;
	size_t x;

	for (int i = range; i >= 0; --i) {
		char b[3];
		std::snprintf(b, sizeof(buffer), "%02X", buffer[i]);
		s += b;
	}
	x = std::stoul(s, nullptr, 16);
	return x;
}

void bmp_fileheader(std::ifstream& is) {
	for (int i = 0; i < 10; ++i) {
		is.get();
	}
	char buffer[4];
	size_t x = 1;
	size_t offset = 0;
	is.read(buffer, 4);
	offset = str2ul(buffer, 3);
}

PAMimg bmp_infoheader(std::ifstream& is, std::ofstream& os) {
	size_t w, h, bpp, num_colors;
	char buffer4[4], buffer2[2];

	is.read(buffer4, 4); //header size in bytes (40)
	is.read(buffer4, 4);
	w = str2ul(buffer4, 3);
	is.read(buffer4, 4);
	h = str2ul(buffer4, 3);
	is.read(buffer2, 2);
	is.read(buffer2, 2);
	bpp = str2ul(buffer2, 1);
	is.read(buffer4, 4);
	is.read(buffer4, 4);
	is.read(buffer4, 4);
	is.read(buffer4, 4);
	is.read(buffer4, 4);
	num_colors = str2ul(buffer4, 3);
	is.read(buffer4, 4);

	PAMimg img(w, h, is, os);
	img.setbpp(bpp);
	img.setnumcolors(num_colors);

	return img;
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

	bmp_fileheader(is);
	PAMimg img = bmp_infoheader(is, os);

	img.filldata();
	os << "P7\n";
	os << "WIDTH " << img.width() << "\n";
	os << "HEIGHT " << img.height() << "\n";
	os << "DEPTH 3\n";
	os << "MAXVAL 255\n";
	os << "TUPLTYPE RGB\n";
	os << "ENDHDR\n";

	os.write(img.rawdata(), img.rawsize());

	return EXIT_SUCCESS;
}