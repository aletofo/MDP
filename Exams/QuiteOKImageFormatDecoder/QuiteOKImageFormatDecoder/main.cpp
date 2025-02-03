#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <string>

template<typename T>
struct mat {
    size_t rows_, cols_, size_;
    std::vector<T> data_;

    mat(size_t rows, size_t cols) : rows_(rows), cols_(cols), size_(rows * cols) {}

    const T& operator[](size_t i) const { return data_[i]; }
    T& operator[](size_t i) { return data_[i]; }

    size_t size() const { return rows_ * cols_; }
    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }

    void push_pixel(std::vector<uint8_t> rgba) {
        if (data_.size() > size())
            return;
        for (int i = 0; i < 4; ++i) {
            data_.push_back(rgba[i]);
        }
    }

    const char* rawdata() const {
        return reinterpret_cast<const char*>(data_.data());
    }
    size_t rawsize() const { return size() * sizeof(T); }
};

uint32_t swap_endian(uint32_t value) {
    return ((value & 0x000000FF) << 24) |
        ((value & 0x0000FF00) << 8) |
        ((value & 0x00FF0000) >> 8) |
        ((value & 0xFF000000) >> 24);
}

mat<uint8_t> readhdr(std::ifstream& is) {
    char buffer[4];

    is.read(buffer, 4 * sizeof(char));
    std::string magic = buffer;
    magic.resize(4);
    if (magic != "qoif") {
        std::cout << "ERROR - not a .qoi file\n";
        mat<uint8_t> ignore(0, 0);
        return ignore;
    }

    uint32_t width, height;
    is.read(reinterpret_cast<char*>(&width), sizeof(width));
    is.read(reinterpret_cast<char*>(&height), sizeof(height));

    uint8_t channels, colorspace;
    is.read(reinterpret_cast<char*>(&channels), sizeof(channels));
    is.read(reinterpret_cast<char*>(&colorspace), sizeof(colorspace));
    width = swap_endian(width);
    height = swap_endian(height);

    mat<uint8_t> img(height, width * 4);
    return img;
}

size_t find_index(std::vector<uint8_t> rgba) {
    return (rgba[0] * 3 + rgba[1] * 5 + rgba[2] * 7 + rgba[3] * 11) % 64;
}

bool end_marker(std::ifstream& is) {
    char buffer[8];

    auto curpos = is.tellg();
    for (int i = 0; i < 8; ++i) {
        is.get(buffer[i]);
    }
    for (int i = 0; i < 8; ++i) {
        if (i < 7) {
            if (buffer[i] != 0) {
                is.seekg(curpos);
                return false;
            }
        }
        else if (i == 7) {
            if (buffer[i] != 1) {
                is.seekg(curpos);
                return false;
            }
        }
    }
    is.seekg(curpos);

    return true;
}

void bit8chunk(std::ifstream& is, uint8_t tag, std::vector<uint8_t>& prev_pixel, mat<uint8_t>& img) {
    uint8_t r, g, b, a;
    char ch;
    
    is.get(ch);
    r = static_cast<uint8_t>(ch);
    is.get(ch);
    g = static_cast<uint8_t>(ch);
    is.get(ch);
    b = static_cast<uint8_t>(ch);

    prev_pixel[0] = r;
    prev_pixel[1] = g;
    prev_pixel[2] = b;
    if (tag == 254) {
        img.push_pixel(prev_pixel);
    }
    else if (tag == 255) {
        is.get(ch);
        a = static_cast<uint8_t>(ch);
        prev_pixel[3] = a;

        img.push_pixel(prev_pixel);
    }
}

void diff(uint8_t d, int i, std::vector<uint8_t>& pix) {

    if (d == 0) {
        if (pix[i] == 0) {
            pix[i] = 254;
        }
        else if (pix[i] == 1) {
            pix[i] = 255;
        }
        else {
            pix[i] -= 2;
        }
    }
    if (d == 1) {
        if (pix[i] == 0) {
            pix[i] = 255;
        }
        else {
            pix[i] -= 1;
        }
    }
    if (d == 2) {
        return;
    }
    if (d == 3) {
        if (pix[i] == 255) {
            pix[i] = 0;
        }
        else {
            pix[i] += 1;
        }
    }
}

void luma(uint8_t diff_g, uint8_t dr_dg, uint8_t db_dg, std::vector<uint8_t>& pix) {
    //r and b of new pixel are calculated as r = (dr - dg) - diff_green and b = (db - dg) - diff_green
    std::vector<int8_t> range_g;
    std::vector<int8_t> range_rb;
    int8_t val = -32;
    for (int8_t i = 0; i < 64; ++i) {
        range_g.push_back(val);
        ++val;
    }
    val = -8;
    for (int8_t i = 0; i < 16; ++i) {
        range_rb.push_back(val);
        ++val;
    }
    int16_t r, g, b;

    g = pix[1] + range_g[diff_g];
    r = range_rb[dr_dg] + range_g[diff_g];
    b = range_rb[db_dg] + range_g[diff_g];

    if (r > 255) {
        r -= 255;
    }
    if (g > 255) {
        g -= 255;
    }
    if (b > 255) {
        b -= 255;
    }
    if (r < 0) {
        r = 256 + r;
    }
    if (g < 0) {
        g = 256 + g;
    }
    if (b < 0) {
        b = 256 + b;
    }

    pix[0] = static_cast<uint8_t>(r);
    pix[1] = static_cast<uint8_t>(g);
    pix[2] = static_cast<uint8_t>(b);
}

void bit2chunk(std::ifstream& is, std::vector<std::vector<uint8_t>> pixels, uint8_t tag, uint8_t info, std::vector<uint8_t>& prev_pixel, mat<uint8_t>& img) {


    if (tag == 0) {
        prev_pixel = pixels[info];
        img.push_pixel(prev_pixel);
    }
    else if (tag == 1) {
        uint8_t dr, dg, db;

        dr = info >> 4;
        dg = info << 4;
        dg = dg >> 6;
        db = info << 6;
        db = db >> 6;

        for (int i = 0; i < 3; ++i) {
            if (i == 0) {
                diff(dr, i, prev_pixel);
            }
            if (i == 1) {
                diff(dg, i, prev_pixel);
            }
            if (i == 2) {
                diff(db, i, prev_pixel);
            }
        }
        img.push_pixel(prev_pixel);
    }
    else if (tag == 2) {
        std::vector<uint8_t> cur_pixel;
        cur_pixel.resize(4);
        cur_pixel[3] = prev_pixel[3];
        uint8_t diff_green = info;
        char ch;
        is.get(ch);
        uint8_t buf = static_cast<uint8_t>(ch);
        uint8_t dr_dg = buf >> 4;
        uint8_t db_dg = buf << 4;
        db_dg = buf >> 4;
        //r and b of new pixel are calculated as r = (dr - dg) - diff_green and b = (db - dg) - diff_green
        luma(diff_green, dr_dg, db_dg, prev_pixel);
        img.push_pixel(prev_pixel);
    }
    else if (tag == 3) {
        uint8_t run = info + 1;
        for (uint8_t i = 0; i < run; ++i) {
            img.push_pixel(prev_pixel);
        }
    }
}

void decode(std::ifstream& is, mat<uint8_t>& img) {
    std::vector<std::vector<uint8_t>> pixel_array;
    pixel_array.resize(64);
    for (int i = 0; i < 64; ++i) {
        pixel_array[i].resize(4);
        std::fill(pixel_array[i].begin(), pixel_array[i].end(), 0);
    }
    std::vector<uint8_t> prev_pixel = { 0,0,0,255 };

    char ch;
    uint8_t info = 0, tag = 0;
    size_t index_position;

    while (!end_marker(is)) {
        auto curpos = is.tellg(); //save the position at each iteration

        is.get(ch);
        tag = static_cast<uint8_t>(ch);
        if (tag == 254 || tag == 255) {
            bit8chunk(is, tag, prev_pixel, img);
            index_position = find_index(prev_pixel);
            pixel_array[index_position] = prev_pixel;
        }
        else {
            info = tag << 2;
            info = info >> 2;
            tag = tag >> 6;
            bit2chunk(is, pixel_array, tag, info, prev_pixel, img);
            index_position = find_index(prev_pixel);
            pixel_array[index_position] = prev_pixel;
        }
    }
}

int main(int argc, char* argv[])
{
    // TODO: Manage the command line  
    if (argc != 3)
        return EXIT_FAILURE;
    std::ifstream is(argv[1], std::ios::binary);
    if (!is)
        return EXIT_FAILURE;

    // TODO: Lettura dell'header e delle dimensioni dell'immagine 
    mat<uint8_t> img = readhdr(is); // TODO: Dovete mettere le dimensioni corrette!

    // TODO: decodificare il file QOI in input e inserire i dati nell'immagine di output
    decode(is, img);

    // Questo è il formato di output PAM. È già pronto così, ma potete modificarlo se volete
    std::ofstream os(argv[2], std::ios::binary); // Questo utilizza il secondo parametro della linea di comando!
    os << "P7\nWIDTH " << img.cols() / 4 << "\nHEIGHT " << img.rows() <<
        "\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n";
    os.write(img.rawdata(), img.rawsize());

    return EXIT_SUCCESS;
}