/*
Write a command-line C++ program that accepts the following syntax:

frequencies <input file> <output file>

The program takes a binary file as input and for each byte (interpreted as an unsigned 8-bit integer) it
counts its occurrences. The output is a text file consisting of one line for each different byte found in the
input file with the following format:

<byte><tab><occurrences><new line>

The byte is represented with its two-digit hexadecimal value, occurrences in base ten. The rows are sorted
by byte value, from the smallest to the largest.
*/

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <iomanip>
#include <chrono>
#include <iterator>
#include <ranges>

void error(const char* message) {
	std::cout << message << '\n';
	exit(EXIT_FAILURE);
}

void syntax() {
	error("SYNTAX:\n"
		"frequencies <input file> <output file>\n"
	);
}

int main(int argc, char* argv[]) {

	using namespace std::chrono;
	using namespace std;

	auto start = steady_clock::now();

	if (argc != 3) {
		syntax();
	}

	//char ch;
	//unsigned char uch;

	std::vector<int> stats(256, 0);

	std::ifstream is(argv[1]/*, std::ios::binary*/);
	if (!is) {
		return EXIT_FAILURE;
	}

	is.seekg(0, ios::end);
	auto filesize = is.tellg(); //Returns input position indicator of the current associated streambuf object.
	is.seekg(0, ios::beg);
	vector<char> v(filesize);
	is.read(v.data(), filesize);
	//vector<char> v{ istreambuf_iterator<char>(is), istreambuf_iterator<char>() }; //istreambuf_iterator legge byte per byte

	/*
	while (is.get(ch)) {
		uch = static_cast<unsigned char>(ch);
		v[uch] += 1;
	}
	*/

	for (const auto& x : v) {
		unsigned char u = static_cast<unsigned char>(x);
		stats[u] += 1;
	}

	std::ofstream os(argv[2]/*, std::ios::binary*/);
	if (!os) {
		return EXIT_FAILURE;
	}

	int i = 0;

	for (const auto& x : stats) {
		if (x == 0) {
			i++;
			continue;
		}
		os << std::hex << i << ' ';
		os << std::dec << x << '\n';

		i++;
	}

	auto stop = steady_clock::now();
	auto elapsed = stop - start;
	duration<double, std::milli> elapsed_ms = elapsed;

	std::cout << "Elapsed: " << elapsed_ms << "\n";

	return EXIT_SUCCESS;
}