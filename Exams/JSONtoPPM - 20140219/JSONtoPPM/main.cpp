#include <cstdlib>
#include <vector>
#include <iostream>
#include <tuple>
#include <fstream>
#include <print>
#include <string>

template <typename T>
struct ppmimg {
	size_t rows_;
	size_t cols_;
	std::string color_;
	std::vector<T> data_;

	ppmimg(size_t rows = 0, size_t cols = 0, std::string color = "") :
		rows_(rows), cols_(cols), data_(rows* cols), color_(color) {}

	size_t rows() const { return rows_; }
	size_t cols() const { return cols_; }
	size_t size() const { return rows_ * cols_; }

	std::tuple<uint8_t, uint8_t, uint8_t> color() const { //split color string in 3 bytes for r,g,b values
		std::string r, g, b;
		for (size_t i = 0; i < 6; i++) {
			if (i < 2) {
				r.push_back(color_[i]);
			}
			if (i >= 2 && i < 4) {
				g.push_back(color_[i]);
			}
			if (i >= 4 && i < 6) {
				b.push_back(color_[i]);
			}
		}
		uint8_t r_ = static_cast<uint8_t>(std::stoul(r, nullptr, 16));
		uint8_t g_ = static_cast<uint8_t>(std::stoul(g, nullptr, 16));
		uint8_t b_ = static_cast<uint8_t>(std::stoul(b, nullptr, 16));

		return std::tuple<uint8_t, uint8_t, uint8_t>{r_, g_, b_};
	}

	void setrows(this auto&& self, size_t num) { self.rows_ = num; }
	void setcols(this auto&& self, size_t num) { self.cols_ = num; }
	void setdata(this auto&& self, size_t r, size_t c) { self.data_ = r * c; }
	void setcolor(this auto&& self, std::string color) { self.color_ = color; }

	void operator() (this auto&& self, size_t r, size_t c, uint8_t byte) {
		self.data_[r * self.cols_ + c] = byte;
	}
	uint8_t at(this auto&& self, size_t r, size_t c) {
		return self.data_[r * self.cols_ + c];
	}

	void fill_background(this auto&& self) {
		std::tuple<uint8_t, uint8_t, uint8_t> rgb = self.color();
		uint8_t R = std::get<0>(rgb);
		uint8_t G = std::get<1>(rgb);
		uint8_t B = std::get<2>(rgb);

		for (size_t r = 0; r < self.rows_; ++r) {
			for (size_t c = 0; c < self.cols_; ++c) {
				for (size_t channel = 0; channel < 3; ++channel) {
					if (channel == 0) {
						self(r, c, R);
						++c;
					}
					if (channel == 1) {
						self(r, c, G);
						++c;
					}
					if (channel == 2) {
						self(r, c, B);
					}
				}
			}
		}
	}
	void fill_area(this auto&& self, std::string color, size_t x, size_t y, size_t w, size_t h) {
		self.setcolor(color);
		std::tuple<uint8_t, uint8_t, uint8_t> rgb = self.color();
		uint8_t R = std::get<0>(rgb);
		uint8_t G = std::get<1>(rgb);
		uint8_t B = std::get<2>(rgb);

		for (size_t r = y; r < y + h; ++r) {
			for (size_t c = x * 3; c < (x * 3) + (w * 3); ++c) {
				for (size_t channel = 0; channel < 3; ++channel) {
					if (channel == 0) {
						self(r, c, R);
						++c;
					}
					if (channel == 1) {
						self(r, c, G);
						++c;
					}
					if (channel == 2) {
						self(r, c, B);
					}
				}
			}
		}
	}

	auto rawsize() const { return size() * sizeof(T); }
	auto rawdata() {
		return reinterpret_cast<char*>(data_.data());
	}
	auto rawdata() const {
		return reinterpret_cast<const char*>(data_.data());
	}
};

//template <typename E>
struct JSONelement {
	size_t width_;
	size_t height_;
	size_t x_;
	size_t y_;
	std::string color_;
	//std::vector<E> elements_;

	JSONelement(size_t width, size_t heigth, size_t x, size_t y, std::string color) : 
		width_(width), height_(heigth), x_(x), y_(y), color_(color) {}

	size_t w() const { return width_; }
	size_t h() const { return height_; }
	size_t x() const { return x_; }
	size_t y() const { return y_; }
	std::tuple<size_t, size_t, size_t> color() const {
		std::string r, g, b;
		for (size_t i = 0; i < 6; i++) {
			if (i < 2) {
				r.push_back(color_[i]);
			}
			if (i >= 2 && i < 4) {
				g.push_back(color_[i]);
			}
			if (i >= 4 && i < 6) {
				b.push_back(color_[i]);
			}
		}
		size_t r_ = std::stoi(r);
		size_t g_ = std::stoi(g);
		size_t b_ = std::stoi(b);

		return std::tuple<size_t, size_t, size_t>{r_, g_, b_};
	}

	void setw(this auto&& self, size_t num) { self.width_ = num; }
	void seth(this auto&& self, size_t num) { self.height_ = num; }
	void setx(this auto&& self, size_t num) { self.x_ = num; }
	void sety(this auto&& self, size_t num) { self.y_ = num; }
	void setcolor(this auto&& self, std::string c) { self.color_ = c; }
};

std::string readlabel(std::ifstream& is) {

	std::string l;
	char ch;

	while (is.get(ch)) {
		if (ch == '"')
			break;

		l.push_back(ch);
	}
	return l;
}

size_t readvalue(std::ifstream& is) {

	std::string value;
	char ch;

	while (is.get(ch)) {
		if (ch == ',')
			break;
		if (std::isspace(static_cast<unsigned char>(ch))) {
			continue;
		}
		value.push_back(ch);
	}
	size_t v = std::stoi(value);

	return v;
}

void buildheader(size_t w, size_t h, size_t maxval, std::ofstream &os) {
	os << "P6\n";
	os << w << " " << h << "\n";
	os << maxval << "\n";
}

ppmimg<uint8_t> img_background(std::ifstream &is, std::ofstream &os) {

	char ch;
	std::string label, color;
	size_t w, h;

	while (is.get(ch)) {
		if (ch == '"') {
			label = readlabel(is);
		}
		if (label == "width") {
			while (is.get(ch)) {
				if (ch == ':') {
					w = readvalue(is);
					label.clear();
					break;
				}
			}
		}
		if (label == "height") {
			while (is.get(ch)) {
				if (ch == ':') {
					h = readvalue(is);
					label.clear();
					break;
				}
			}
		}
		if (label == "background") {
			while (is.get(ch)) {
				if (ch == ':') {
					while (is.get(ch)) {
						if (ch == '"')
							break;
						if (std::isspace(static_cast<unsigned char>(ch)))
							continue;
					}
					color = readlabel(is);
					label.clear();
					break;
				}
			}
		}
		if (label == "elements") {
			label.clear();
			break;
		}
	}

	buildheader(w, h, 255, os);
	ppmimg<uint8_t> img(h, w * 3, color);
	img.fill_background();
	//os.write(img.rawdata(), img.rawsize());

	return img;
}

void img_elements(std::ifstream &is, std::ofstream &os, ppmimg<uint8_t> &img) {

	char ch;
	std::string label, color;
	size_t w, h, x, y;

	while (is.get(ch)) {
		if (ch == ']') {
			os.write(img.rawdata(), img.rawsize());
			break;
		}
		if (ch == '{') {
			while (is.get(ch)) {
				if (ch == '}') {
					img.fill_area(color, x, y, w, h);
					//os.write(img.rawdata(), img.rawsize());
					break;
				}
				if (ch == '"') {
					label = readlabel(is);
				}
				if (label == "width") {
					while (is.get(ch)) {
						if (ch == ':') {
							w = readvalue(is);
							label.clear();
							break;
						}
					}
				}
				if (label == "height") {
					while (is.get(ch)) {
						if (ch == ':') {
							h = readvalue(is);
							label.clear();
							break;
						}
					}
				}
				if (label == "x") {
					while (is.get(ch)) {
						if (ch == ':') {
							x = readvalue(is);
							label.clear();
							break;
						}
					}
				}
				if (label == "y") {
					while (is.get(ch)) {
						if (ch == ':') {
							y = readvalue(is);
							label.clear();
							break;
						}
					}
				}
				if (label == "color") {
					while (is.get(ch)) {
						if (ch == ':') {
							while (is.get(ch)) {
								if (ch == '"')
									break;
								if (std::isspace(static_cast<unsigned char>(ch)))
									continue;
							}
							color = readlabel(is);
							label.clear();
							break;
						}
					}
				}
			}
		}
	}
}

int main(int argc, char* argv[]) {

	if (argc != 3) {
		std::cout << "Invalid number of arguments.\n";
		return EXIT_FAILURE;
	}
	std::ifstream is(argv[1]);
	if (!is) {
		std::cout << "ERROR: couldn't open input file.\n";
		return EXIT_FAILURE;
	}
	std::ofstream os(argv[2], std::ios::binary);
	if (!os) {
		std::cout << "ERROR: couldn't open output file.\n";
		return EXIT_FAILURE;
	}

	ppmimg<uint8_t> img = img_background(is, os);
	img_elements(is, os, img);

	return EXIT_SUCCESS;
}