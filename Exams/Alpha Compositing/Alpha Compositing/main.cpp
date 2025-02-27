#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>

struct PAMimg {
	size_t width_, height_, depth_;
	std::vector<uint8_t> data_;

	std::vector<uint8_t> blend_pixel(std::vector<uint8_t> pix_o, std::vector<uint8_t> pix_u) {
		double alpha_o = pix_o[3] / 255, alpha_u = pix_u[3] / 255;
		double alpha0 = alpha_o + alpha_u * (1 - alpha_o);
		double C0;
		std::vector<double> rgba_d;
		std::vector<uint8_t> rgba;

		for (int i = 0; i < 3; ++i) {
			C0 = (pix_o[i] * alpha_o + pix_u[i] * alpha_u * (1 - alpha_o)) / alpha0;
			rgba_d.push_back(C0);
		}
		for (int i = 0; i < 3; ++i) {
			uint8_t byte = static_cast<uint8_t>(rgba_d[i]);
			rgba.push_back(byte);
		}
		rgba.push_back(static_cast<uint8_t>(alpha0 * 255));
		return rgba;
	}

public:

	PAMimg(size_t w, size_t h, size_t d) : width_(w), height_(h), depth_(d), data_(w * h * d) {}

	uint8_t at (size_t c, size_t r) {
		return data_[r * (width_ * depth_) + c];
	}
	uint8_t at(size_t i) {
		return data_[i];
	}

	size_t const width() { return width_; }
	size_t const height() { return height_; }
	size_t const depth() { return depth_; }

	size_t size() { return data_.size(); }

	void zeros() {
		std::fill(data_.begin(), data_.end(), 0);
	}

	void fill_data(size_t r, size_t c, uint8_t byte) {
		data_[r * (width_ * depth_) + c] = byte;
	}

	void add_image(size_t x, size_t y, PAMimg& over_img) {
		std::vector<uint8_t> over_data;
		if (over_img.depth() == 3) {		
			for (size_t i = 0; i < over_img.size(); ++i) {
				if (i % 3 == 0 && i != 0) {
					over_data.push_back(255);
				}
				over_data.push_back(over_img.at(i));
			}
		}
		else {
			over_data = over_img.get_data();
		}
		
		int i = 0;
		uint8_t byte_o = 0;
		std::vector<uint8_t> rgba_over, rgba_under;
		for (size_t r = y; r < height_; ++r) {
			if (r >= over_img.height() + y) {
				break;
			}
			for (size_t c = x * depth_; c < width_ * depth_; ++c) {
				if (c >= over_img.width() * depth_ + x * depth_) {
					break;
				}
				for (int channel = 0; channel < 4; ++channel) {
					if (i < over_data.size()) {
						byte_o = over_data[i];
						++i;
					}
					else {
						byte_o = 0;
					}
					uint8_t byte_u = data_[r * (width_ * depth_) + c];
					++c;
					rgba_over.push_back(byte_o);
					rgba_under.push_back(byte_u);
				}
				rgba_over = blend_pixel(rgba_over, rgba_under);
				c -= 4;
				for (int channel = 0; channel < 4; ++channel) {
					uint8_t byte = rgba_over[channel];
					data_[r * (width_ * depth_) + c] = byte;
					++c;
				}
				rgba_over.clear();
				rgba_under.clear();
				--c;
				if (i >= over_data.size()) {
					break;
				}
			}
			if (i >= over_data.size()) {
				break;
			}
		}
	}

	std::vector<uint8_t> get_data() {
		return data_;
	}

	PAMimg copyPAMimg(this auto&& self) {
		return self;
	}
};

PAMimg PAMhdr(std::ifstream& is) {
	size_t width, height, depth;
	std::string token;

	is >> token; //must be P7
	is >> token >> token;
	width = std::stoi(token);
	is >> token >> token;
	height = std::stoi(token);
	is >> token >> token;
	depth = std::stoi(token);

	while (token != "ENDHDR") {
		is >> token;
	}

	is.get(); //ignore the last 0A of the header

	PAMimg img(width, height, depth);

	return img;
}

PAMimg p_compose(std::ifstream& is, size_t x, size_t y, PAMimg& out) {
	PAMimg input_img = PAMhdr(is);

	char ch;
	uint8_t byte;
	for (size_t r = 0; r < input_img.height(); ++r) {
		for (size_t c = 0; c < input_img.width() * input_img.depth(); ++c) {
			is.get(ch);
			byte = static_cast<uint8_t>(ch);

			input_img.fill_data(r, c, byte);
		}
	}
	size_t out_w = input_img.width() + x, out_h = input_img.height() + y;
	if (out.width() > out_w) {
		out_w = out.width();
	}
	if (out.height() > out_h) {
		out_h = out.height();
	}

	PAMimg output_img(out_w, out_h, 4);
	output_img.zeros();
	if (out.width() != 0) {
		output_img.add_image(0,0, out);
	}
	output_img.add_image(x, y, input_img);

	return output_img;
}

int main(int argc, char* argv[]) {
	std::string input_filename, output_filename, ext = ".pam";
	output_filename = argv[1] + ext;
	std::ofstream os(output_filename, std::ios::binary);
	PAMimg img(0, 0, 0);

	size_t final_w = 0, final_h = 0;
	std::vector<uint8_t> final_data;

	for (int i = 2; i < argc; ++i) {
		std::string argstr = argv[i];
		if (argstr == "-p") {
			++i;
			size_t x, y;
			x = std::stoul(argv[i]);
			++i;
			y = std::stoul(argv[i]);
			++i;
			input_filename = argv[i] + ext;
			std::ifstream is(input_filename, std::ios::binary);
			img = p_compose(is, x, y, img);
			final_data = img.get_data();
			final_w = img.width();
			final_h = img.height();
		}
		else {
			input_filename = argv[i] + ext;
			std::ifstream is(input_filename, std::ios::binary);
			img = p_compose(is, 0, 0, img);
			final_data = img.get_data();
			final_w = img.width();
			final_h = img.height();
		}
	}

	os << "P7\n" << "WIDTH " << final_w << "\n" << "HEIGHT " << final_h << "\n" << "DEPTH 4\n" << "MAXVAL 255\n" << "TUPLTYPE RGB_ALPHA\n" << "ENDHDR\n";
	os.write(reinterpret_cast<char*>(final_data.data()), final_data.size());

	return EXIT_SUCCESS;
}