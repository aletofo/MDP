#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <cmath>
#include <vector>

class PPMimg {
	size_t width_;
	size_t height_;
	std::vector<uint8_t> data_;
	std::ifstream& is_;
	std::ofstream& os_;

	size_t size() { return width_ * height_; }

public:
	PPMimg(size_t w, size_t h, std::ifstream& is, std::ofstream& os) : width_(w), height_(h), is_(is), os_(os), data_(w * h) {}
	~PPMimg() {}

	void setbg(std::string r, std::string g, std::string b) {
		uint8_t R, G, B;
		R = static_cast<uint8_t>(std::stoul(r, nullptr, 16));
		G = static_cast<uint8_t>(std::stoul(g, nullptr, 16));
		B = static_cast<uint8_t>(std::stoul(b, nullptr, 16));

		for (size_t i = 0; i < height_; ++i) {
			for (size_t j = 0; j < width_; ++j) {
				for (size_t channel = 0; channel < 3; ++channel) {
					if (channel == 0) {
						data_[i * width_ + j] = R;
						++j;
					}
					if (channel == 1) {
						data_[i * width_ + j] = G;
						++j;
					}
					if (channel == 2) {
						data_[i * width_ + j] = B;
					}
				}
			}
		}
	}
	void fill_area(this auto&& self, size_t start_index) {
		std::vector<uint8_t> buffer(378);
		self.is_.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

		if (!self.is_) {
			throw std::runtime_error("Errore: lettura dell'input stream fallita.");
		}

		std::copy(buffer.begin(), buffer.end(), self.data_.begin() + start_index);
	}


	void fillarea(this auto&& self, size_t x, size_t y, size_t w, size_t h) {
		
		std::vector<uint8_t> v(w * 3 * h);
		self.is_.read(reinterpret_cast<char*>(v.data()), v.size());
		int i = 0;

		for (size_t r = y; r < y + h; ++r) {
			for (size_t c = x * 3; c < (x * 3) + (w * 3); ++c) {
				if (i == w * 3 * h)
					break;
				self.data_[r * self.width_ + c] = v[i];
				++i;
			}
			if (i == w * 3 * h)
				break;
		}

		return;
	}

	auto rawsize() { return size() * sizeof(uint8_t); }
	auto rawdata() {
		return reinterpret_cast<char*>(data_.data());
	}
};

void buildheader(size_t w, size_t h, size_t maxval, std::ofstream& os) {
	os << "P6\n";
	os << w << " " << h << "\n";
	os << maxval << "\n";
}

PPMimg canvas(std::ifstream& is, std::ofstream& os) {
	char ch;
	unsigned char uch = 0;
	std::string info, val, hexval, r, g, b;
	size_t img_w = 0, img_h = 0;

	while (is.get(ch)) {
		if (ch == '}')
			break;
		if (ch != 'i' && ch != 'I') {
			while (is.get(ch)) {
				if (ch == 'U') {
					is.get(ch);
					uch = static_cast<unsigned int>(ch);
					break;
				}
				if (!std::isalpha(ch))
					break;
				info.push_back(ch);
			}
			if(info.back() == 'i' || info.back() == 'I')
				info.pop_back();
		}
		if (info == "width") {
			while (1) {
				if (ch == 'i' || ch == 'I' || ch == 'U')
					break;
				if (uch != 0) {
					val.push_back(uch);
					uch = 0;
				}
				else {
					val.push_back(ch);
				}
				is.get(ch);
			}
			for (unsigned char c : val) {
				char buffer[3];
				std::snprintf(buffer, sizeof(buffer), "%02X", c);
				hexval += buffer;
			}
			img_w = std::stoul(hexval, nullptr, 16);
			val.clear();
			hexval.clear();
			info.clear();
		}
		if (info == "height") {
			while (1) {
				if (ch == 'i' || ch == 'I' || ch == 'U')
					break;
				if (uch != 0) {
					val.push_back(uch);
					uch = 0;
				}
				else {
					val.push_back(ch);
				}
				is.get(ch);
			}
			for (unsigned char c : val) {
				char buffer[3];
				std::snprintf(buffer, sizeof(buffer), "%02X", c);
				hexval += buffer;
			}
			img_h = std::stoul(hexval, nullptr, 16);
			val.clear();
			hexval.clear();
			info.clear();
		}
		if (info == "background") {
			while (is.get(ch)) {
				if (ch == 'i' || ch == 'I' || ch == 'U') {
					is.get();
					is.get(ch);
					val.push_back(ch);
					is.get(ch);
					val.push_back(ch);
					is.get(ch);
					val.push_back(ch);
					break;
				}
			}
			for (int i = 0; i < 3; ++i) {
				char buffer[3];
				std::snprintf(buffer, sizeof(buffer), "%02X", static_cast<unsigned char>(val[i]));
				if (i == 0) {
					r += buffer;
				}
				if (i == 1) {
					g += buffer;
				}
				if (i == 2) {
					b += buffer;
				}
			}

		}
	}
	PPMimg bg_img(img_w * 3, img_h, is, os);
	bg_img.setbg(r,g,b);
	buildheader(img_w, img_h, 255, os);

	return bg_img;
}

void items_print(std::ifstream& is, std::ofstream& os) {
	char ch;
	unsigned char uch = 0;
	std::string info;
	char bin[10];
	//is.get(ch); // read '{'

	while (is.get(ch)) {
		if (ch == '}')
			break;
		if (ch == '$') {
			is.read(bin, 7 * sizeof(char));
			continue;
		}
		if (ch != 'i' && ch != 'I' && ch != 'U') {
			while (is.get(ch)) {
				if (ch == '-') {
					info.push_back(ch);
					continue;
				}
				if (ch == 'U') {
					is.get(ch);
					uch = static_cast<unsigned int>(ch);
					break;
				}
				if (!std::isalpha(ch))
					break;
				info.push_back(ch);
			}
			if (info.size() > 0) {
				if (info.back() == 'i' || info.back() == 'I')
					info.pop_back();
			}
			std::cout << info << ",";
			info.clear();
		}
	}
	std::cout << "\n";
}

void elements(std::ifstream& is, std::ofstream& os, PPMimg& img) {
	char ch;
	unsigned char uch = 0;
	std::string item, info, val, hexval;
	size_t w = 0, h = 0, x = 0, y = 0;

	while (is.get(ch)) {
		if (ch == '}')
			continue;
		if (ch != 'i' && ch != 'I' && ch != 'U') {
			while (is.get(ch)) {
				if (ch == 'U') {
					is.get(ch);
					uch = static_cast<unsigned int>(ch);
					break;
				}
				if (!std::isalpha(ch))
					break;
				item.push_back(ch);
			}
			if (item.size() > 0) {
				if (item.back() == 'i' || item.back() == 'I')
					item.pop_back();
			}
		}
		if (item == "image") {
			while (is.get(ch)) {
				if (ch == '}')
					break;
				if (ch != 'i' && ch != 'I' && ch != 'U') {
					while (is.get(ch)) {
						if (ch == 'U') {
							is.get(ch);
							uch = static_cast<unsigned int>(ch);
							break;
						}
						if (!std::isalpha(ch))
							break;
						info.push_back(ch);
					}
					if (info.back() == 'i' || info.back() == 'I' || info.back() == 'U')
						info.pop_back();
				}
				if (info == "x") {
					while (1) {
						if (ch == 'i' || ch == 'I' || ch == 'U')
							break;
						if (uch != 0) {
							val.push_back(uch);
							uch = 0;
						}
						else {
							val.push_back(ch);
						}
						is.get(ch);
					}
					for (unsigned char c : val) {
						char buffer[3];
						std::snprintf(buffer, sizeof(buffer), "%02X", c);
						hexval += buffer;
					}
					x = std::stoul(hexval, nullptr, 16);
					val.clear();
					hexval.clear();
					info.clear();
				}
				if (info == "y") {
					while (1) {
						if (ch == 'i' || ch == 'I' || ch == 'U')
							break;
						if (uch != 0) {
							val.push_back(uch);
							uch = 0;
						}
						else {
							val.push_back(ch);
						}
						is.get(ch);
					}
					for (unsigned char c : val) {
						char buffer[3];
						std::snprintf(buffer, sizeof(buffer), "%02X", c);
						hexval += buffer;
					}
					y = std::stoul(hexval, nullptr, 16);
					val.clear();
					hexval.clear();
					info.clear();
				}
				if (info == "width") {
					while (1) {
						if (ch == 'i' || ch == 'I' || ch == 'U')
							break;
						if (uch != 0) {
							val.push_back(uch);
							uch = 0;
						}
						else {
							val.push_back(ch);
						}
						is.get(ch);
					}
					for (unsigned char c : val) {
						char buffer[3];
						std::snprintf(buffer, sizeof(buffer), "%02X", c);
						hexval += buffer;
					}
					w = std::stoul(hexval, nullptr, 16);
					val.clear();
					hexval.clear();
					info.clear();
				}
				if (info == "height") {
					while (1) {
						if (ch == 'i' || ch == 'I' || ch == 'U')
							break;
						if (uch != 0) {
							val.push_back(uch);
							uch = 0;
						}
						else {
							val.push_back(ch);
						}
						is.get(ch);
					}
					for (unsigned char c : val) {
						char buffer[3];
						std::snprintf(buffer, sizeof(buffer), "%02X", c);
						hexval += buffer;
					}
					h = std::stoul(hexval, nullptr, 16);
					val.clear();
					hexval.clear();
					info.clear();
				}
				if (info == "data") {
					char bin[7];
					is.read(bin, 6 * sizeof(char));
					img.fillarea(x, y, w, h);
					item.clear();
					break;
				}
			}
		}
		else {
			if (item.size() == 0)
				continue;
			std::cout << item << " : ";
			items_print(is, os);
			item.clear();
		}
	}

	os.write(img.rawdata(), img.rawsize());
}

void decompress(std::ifstream& is, std::ofstream& os) {
	char ch;
	std::string label;
	is.get(); //read {

	//canvas
	while (is.get(ch)) {
		if (ch != 'i' && !std::isspace(ch)) {
			while (is.get(ch)) {
				if (!std::isalpha(ch))
					break;
				label.push_back(ch);
			}
		}
		if (label == "canvas") {
			label.clear();
			break;
		}
	}
	PPMimg bg_img = canvas(is, os);

	//elements
	while (is.get(ch)) {
		if (ch != 'i' && !std::isspace(ch)) {
			while (is.get(ch)) {
				if (!std::isalpha(ch))
					break;
				label.push_back(ch);
			}
		}
		if (label == "elements") {
			elements(is, os, bg_img);
			label.clear();
			break;
		}
	}
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

	decompress(is, os);

	return EXIT_SUCCESS;
}