#include <cstdint>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

std::vector<int16_t> get_samples(std::ifstream& is) {
	std::vector<int16_t> samples;
	int16_t sample;

	while (is.read(reinterpret_cast<char*>(&sample), sizeof(sample))) {
		samples.push_back(sample);
	}
	is.close();

	return samples;
}

double get_entropy(std::vector<int16_t> v) {
	double E = 0;
	double p;

	for (int i = 0; i < v.size(); ++i) {
		double val = static_cast<double>(v[i]);
		p = val / INT16_MAX;
		E += p * log2(p);
	}

	return -E;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cout << "ERROR - invalid number of arguments.\n";
		return EXIT_FAILURE;
	}
	std::ifstream is(argv[1], std::ios::binary);
	if (!is) {
		std::cout << "ERROR - can't open the input file\n.";
		return EXIT_FAILURE;
	}
	//save all the samples and close the file
	std::vector<int16_t> samples = get_samples(is);
	auto original_samples = samples; //saving a copy of the original vector
	//compute the entropy of the samples
	double entropy = get_entropy(samples);

	return EXIT_SUCCESS;
}