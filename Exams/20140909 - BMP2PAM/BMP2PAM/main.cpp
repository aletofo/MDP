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

	void filldata24bpp() {
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
	void filldata8bpp(std::vector<std::vector<uint8_t>> table) {
		char ch;
		uint8_t index, byte;
		size_t pad = width() * 8;
		while (pad % 32 != 0) {
			++pad;
		}
		size_t remainder = (pad - width() * 8) / 8;

		size_t output_c = 0;

		for (int r = static_cast<int>(height_) - 1; r >= 0; --r) {
			for (int c = 0; c < width(); ++c) {
				is_.get(ch);
				index = static_cast<uint8_t>(ch);
				for (int channel = 2; channel >= 0; --channel) {
					byte = table.at(index).at(channel);

					data_[r * width_ + output_c] = byte;

					++output_c;
				}
			}
			for (size_t i = 0; i < remainder; ++i) {
				is_.get();
			}
			output_c = 0;
		}
	}
	void filldata4bpp(std::vector<std::vector<uint8_t>> table) {
		char ch;
		uint8_t index, index1, index2, byte1, byte2;
		size_t pad = (width() / 2) * 8;
		while (pad % 32 != 0) {
			++pad;
		}
		size_t remainder = (pad - (width() / 2) * 8) / 8;

		size_t output_c = 0;

		for (int r = static_cast<int>(height_) - 1; r >= 0; --r) {
			for (int c = 0; c < width() / 2; ++c) {
				is_.get(ch);
				index = static_cast<uint8_t>(ch);
				index2 = index << 4;
				index2 = index2 >> 4;
				index1 = index >> 4;

				for (int channel = 2; channel >= 0; --channel) {
					byte1 = table.at(index1).at(channel);

					data_[r * width_ + output_c] = byte1;

					++output_c;
				}
				for (int channel = 2; channel >= 0; --channel) {
					byte2 = table.at(index2).at(channel);

					data_[r * width_ + output_c] = byte2;

					++output_c;
				}
			}
			for (size_t i = 0; i < remainder; ++i) {
				is_.get();
			}
			output_c = 0;
		}
	}
	void filldata1bpp(std::vector<std::vector<uint8_t>> table) {
		char ch;
		uint8_t index, i1, i2, i3, i4, i5, i6, i7, i8, b1, b2, b3, b4, b5, b6, b7, b8;
		double pad = std::round((static_cast<double>(width()) / 8));
		size_t roundedpad = static_cast<size_t>(pad) * 8;
		while (roundedpad % 32 != 0) {
			++roundedpad;
		}
		size_t remainder = (roundedpad - (static_cast<size_t>(pad)) * 8) / 8;

		size_t output_c = 0;
		size_t w = static_cast<int>(pad);

		for (int r = static_cast<int>(height_) - 1; r >= 0; --r) {
			for (int c = 0; c < w; ++c) {
				is_.get(ch);
				index = static_cast<uint8_t>(ch);
				i8 = index << 7;
				i8 = i8 >> 7;
				i7 = index << 6;
				i7 = i7 >> 7;
				i6 = index << 5;
				i6 = i6 >> 7;
				i5 = index << 4;
				i5 = i5 >> 7;
				i4 = index << 3;
				i4 = i4 >> 7;
				i3 = index << 2;
				i3 = i3 >> 7;
				i2 = index << 1;
				i2 = i2 >> 7;
				i1 = index >> 7;

				for (int channel = 2; channel >= 0; --channel) {
					b1 = table.at(i1).at(channel);

					data_[r * width_ + output_c] = b1;

					++output_c;
				}
				for (int channel = 2; channel >= 0; --channel) {
					b2 = table.at(i2).at(channel);

					data_[r * width_ + output_c] = b2;

					++output_c;
				}
				for (int channel = 2; channel >= 0; --channel) {
					b3 = table.at(i3).at(channel);

					data_[r * width_ + output_c] = b3;

					++output_c;
				}
				for (int channel = 2; channel >= 0; --channel) {
					b4 = table.at(i4).at(channel);

					data_[r * width_ + output_c] = b4;

					++output_c;
				}
				for (int channel = 2; channel >= 0; --channel) {
					b5 = table.at(i5).at(channel);

					data_[r * width_ + output_c] = b5;

					++output_c;
				}
				for (int channel = 2; channel >= 0; --channel) {
					b6 = table.at(i6).at(channel);

					data_[r * width_ + output_c] = b6;

					++output_c;
				}
				for (int channel = 2; channel >= 0; --channel) {
					b7 = table.at(i7).at(channel);

					data_[r * width_ + output_c] = b7;

					++output_c;
				}
				for (int channel = 2; channel >= 0; --channel) {
					b8 = table.at(i8).at(channel);

					data_[r * width_ + output_c] = b8;

					++output_c;
				}
			}
			for (size_t i = 0; i < remainder; ++i) {
				is_.get();
			}
			output_c = 0;
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

size_t bmp_fileheader(std::ifstream& is) {
	for (int i = 0; i < 10; ++i) {
		is.get();
	}
	char buffer[4];
	size_t x = 1;
	size_t offset = 0;
	is.read(buffer, 4);
	offset = str2ul(buffer, 3);

	return offset;
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

	PAMimg img(w + 1, h, is, os);
	img.setbpp(bpp);
	img.setnumcolors(num_colors);

	return img;
}

std::vector<std::vector<uint8_t>> create_colortable(std::ifstream& is, size_t offset) {
	std::vector<std::vector<uint8_t>> colortable;
	size_t pos = 0, index = 0;
	char ch;

	colortable.resize((offset - 54) / 4);

	while (pos != offset) {
		for (int i = 0; i < 3; ++i) {
			is.get(ch);
			colortable.at(index).push_back(static_cast<uint8_t>(ch));
		}
		is.get();

		auto curpos = is.tellg();
		pos = static_cast<size_t>(curpos);

		++index;
	}
	return colortable;
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

	size_t data_offset = bmp_fileheader(is);
	PAMimg img = bmp_infoheader(is, os);

	os << "P7\n";
	os << "WIDTH " << img.width() << "\n";
	os << "HEIGHT " << img.height() << "\n";
	os << "DEPTH 3\n";
	os << "MAXVAL 255\n";
	os << "TUPLTYPE RGB\n";
	os << "ENDHDR\n";

	/*img.filldata24bpp(); 24bpp*/

	//8bpp
	/*std::vector<std::vector<uint8_t>> colortable = create_colortable(is, data_offset);
	img.filldata8bpp(colortable);
	*/

	//4bpp
	std::vector<std::vector<uint8_t>> colortable = create_colortable(is, data_offset);
	img.filldata1bpp(colortable);

	os.write(img.rawdata(), img.rawsize());

	return EXIT_SUCCESS;
}