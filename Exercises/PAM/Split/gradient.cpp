#include <cstdlib>
#include <iostream>
#include <fstream>
#include <print>

using namespace std;

bool create_pam_image(const string& output_filename) {

	ofstream os(output_filename, ios::binary);
	if (!os) {
		return EXIT_FAILURE;
	}

	print(os,
		"P7\n"
		"WIDTH 256\n"
		"HEIGHT 256\n"
		"DEPTH 1\n"
		"MAXVAL 255\n"
		"TUPLTYPE GRAYSCALE\n"
		"ENDHDR\n");

	uint8_t sample = 0;

	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			os.put(sample);
		}
		sample++;
	}

	return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {

	if (argc != 2) {
		return EXIT_FAILURE;
	}

	create_pam_image(argv[1]);

	return EXIT_SUCCESS;
}