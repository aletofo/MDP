/*

Write a command line program in C++ with this syntax:

read_int32 <filein.bin> <fileout.txt>

The first parameter is the name of a binary file containing 32-bit numbers 2’s complement, in little endian. 
The program must create a new file, with the name passed as the second parameter, 
with the same numbers saved in decimal text format separated by a new line.

*/

#include <fstream>

template <typename T>
std::istream& raw_read(std::istream& is, T& val, const size_t size = sizeof(T)) {
	return is.read(reinterpret_cast<char*>(&val), size);
}

int main(int argc, char** argv) {
	if (argc != 3) return EXIT_FAILURE;

	std::ifstream is(argv[1], std::ios::binary);
	if (!is) return EXIT_FAILURE;

	std::ofstream os(argv[2]);
	if (!os) return EXIT_FAILURE;

	int32_t tmp;

	while (raw_read(is, tmp)) { 
		os << tmp << std::endl;
	}

	is.close();
	os.close();

	return EXIT_SUCCESS;
}