#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cmath>

struct rectangle {
	double x_, y_;
	double width_, height_;
	double r_, g_, b_;

	rectangle(double x, double y, double w, double h, double r, double g, double b) : 
		x_(x), y_(y), width_(w), height_(h), r_(r), g_(g), b_(b) {}
};

std::vector<rectangle> store_rectangles(std::ifstream& is) {
	double x, y;
	double w, h;
	double r, g, b;

	std::vector<rectangle> rectangles;

	while (!is.eof()) {
		is >> x;
		is >> y;
		is >> w;
		is >> h;
		is >> r;
		is >> g;
		is >> b;
		is.ignore(1);
		rectangle rect(x, y, w, h, r, g, b);
		rectangles.push_back(rect);
	}
	rectangles.pop_back();

	return rectangles;
}

void generatePDF(std::vector<rectangle> rectangles, std::ofstream& os) {	//header->body->cross-ref table->trailer


	//header
	os << "%PDF-1.3\n";
	//body -> list of all the objects: ctalog, outlines, pages, page and then the rectangles(?)
	//catalog
	os << "1 0 obj\n" << "<<\n" << "/Type /Catalog\n" << "/Pages 3 0 R\n" << "Outlines 2 0 R\n" << ">>\n" << "endobj\n\n";
	//outlines (no outline -> count == 0)
	os << "2 0 obj\n" << "<<\n" << "/Type /Outlines\n" << "/Count 0\n" << ">>\n" << "endobj\n\n"; 
	//Pages refers only to 1 Page
	os << "3 0 obj\n" << "<<\n" << "/Type /Pages\n" << "/Count 1\n" << "/Kids [4 0 R]\n" << ">>\n" << "endobj\n\n";
	//Page
	os << "4 0 obj\n" << "<<\n" << "/Type /Page\n" << "/MediaBox [0 0 595 842]\n" << "/Parent 3 0 R\n"
		<< "/Contents 5 0 R\n" << "/Resources << /ProcSet 6 0 R >>\n" << ">>\n" << "endobj\n\n";  
	//Page description
	os << "5 0 obj\n" << "<< /Length 598 >>\n" << "stream\n";

	/*example
	os << "0 0 0 rg\n" << "1 1 2 2 re\n" << "f\n" << "endstream\n" << "endobj\n\n";

	*/

	for (rectangle r : rectangles) {
		os << r.r_ << " " << r.g_ << " " << r.b_ << " " << "rg\n";
		r.x_ = (r.x_ / 2.54) * 72;
		r.y_ = (r.y_ / 2.54) * 72;
		r.width_ = (r.width_ / 2.54) * 72;
		r.height_ = (r.height_ / 2.54) * 72;
		os << r.x_ << " " << r.y_ << " " << r.width_ << " " << r.height_ << " " << "re\n" << "f\n\n";
	}

	os << "endstream\n" << "endobj\n\n";

	os << "6 0 obj\n" << "[\n/PDF\n]\n" << "endobj\n\n";

	os << "xref\n" << "0 7\n" << "0000000000 65535 f \n"; //first entry
	os << "0000000009 00000 n \n";
	os << "0000000074 00000 n \n";
	os << "0000000121 00000 n \n";
	os << "0000000179 00000 n \n";
	os << "0000000299 00000 n \n";
	os << "0000000372 00000 n \n";
	os << "trailer\n" << "<<\n" << "/Size 8\n" << "/Root 1 0 R\n" << ">>\n" << "startxref\n" << "1110\n" << "%%EOF";
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
	
	std::vector<rectangle> rectangles = store_rectangles(is);
	generatePDF(rectangles, os);

	return EXIT_SUCCESS;
}