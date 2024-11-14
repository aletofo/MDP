/*

Write a command line program in C++ with this syntax:

write_int32 <filein.txt> <fileout.bin>

The first parameter is the name of a text file that contains signed base 10 integers from -1000 to 1000 separated by whitespace.
The program must create a new file, with the name passed as the second parameter, with the same numbers saved
as 32-bit binary little endian numbers in 2's complement.

*/

#include <vector>
#include <fstream>

template <typename T>
std::ostream& raw_write(std::ostream& os, const T& val, size_t size = sizeof(T)) {
	return os.write(reinterpret_cast<const char*>(&val), size);
}

int main(int argc, char** argv) {
	if (argc != 3) return EXIT_FAILURE;

	std::ifstream is(argv[1]);
	if (!is) return EXIT_FAILURE;

	std::ofstream os(argv[2], std::ofstream::binary);
	if (!os) return EXIT_FAILURE;

	int tmp;
	while (is >> tmp) {
		if (tmp < -1000 || tmp > 1000) break;
		raw_write(os, tmp);
	}

	is.close();
	os.close();

	return EXIT_SUCCESS;
}