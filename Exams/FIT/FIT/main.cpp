#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cstdint>
#include <cassert>
#include <cstdlib>

using namespace std;

void FitCRC_Get16(uint16_t& crc, uint8_t byte)
{
	static const uint16_t crc_table[16] =
	{
		0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
		0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
	};
	uint16_t tmp;
	// compute checksum of lower four bits of byte
	tmp = crc_table[crc & 0xF];
	crc = (crc >> 4) & 0x0FFF;
	crc = crc ^ tmp ^ crc_table[byte & 0xF];
	// now compute checksum of upper four bits of byte
	tmp = crc_table[crc & 0xF];
	crc = (crc >> 4) & 0x0FFF;
	crc = crc ^ tmp ^ crc_table[(byte >> 4) & 0xF];
}

int readhdr(ifstream& is) {
	char ch;
	uint8_t byte;
	uint16_t crc = 0;
	int i = 0;

	while (i < 12) {
		is.get(ch);
		byte = static_cast<uint8_t>(ch);
		FitCRC_Get16(crc, byte);
		++i;
	}

	uint16_t true_crc;
	is.read(reinterpret_cast<char*>(&true_crc), sizeof(true_crc));

	if (true_crc != crc)
		return 1;

	return 0;
}

uint8_t message_type(ifstream& is) {
	char ch;
	uint8_t recordhdr;

	is.get(ch);
	recordhdr = static_cast<uint8_t>(ch);

	return recordhdr;
}

int fitdump(ifstream& is) {
	char ch;
	uint8_t temp;
	uint32_t filesize;

	//header CRC check
	int filehdr = readhdr(is);
	if (filehdr == 1) {
		//cout << "Header CRC is wrong.\n";
		return 1;
	}
	else {
		cout << "Header CRC ok\n";
	}
	//save the size of the file
	is.seekg(0);
	is.ignore(4 * sizeof(char));
	is.read(reinterpret_cast<char*>(&filesize), sizeof(filesize));
	is.ignore(6 * sizeof(char));

	auto curpos = is.tellg();
	vector<uint8_t> crc_buffer;
	crc_buffer.resize(filesize);
	is.read(reinterpret_cast<char*>(crc_buffer.data()), filesize * sizeof(char));
	is.seekg(curpos);

	//first message shoukd be a definition message (header with the first 4b its = 4)
	is.get(ch);
	uint8_t recordhdr = static_cast<uint8_t>(ch);
	temp = recordhdr >> 4; //checking the first 4 bits
	if (temp != 4) {
		cout << "Not a definition message - ERROR.\n";
		return 1;
	}
	is.ignore(2 * sizeof(char));
	//Global Message Number (should be = 0)
	uint16_t gmn;
	is.read(reinterpret_cast<char*>(&gmn), sizeof(gmn));
	if (gmn != 0) {
		cout << "Global Message Number not 0 - ERROR.\n";
		return 1;
	}
	//numfield and storing all the field definitions
	is.get(ch);
	uint8_t num_field = static_cast<uint8_t>(ch);

	uint8_t num, size, base;
	vector<tuple<uint8_t, uint8_t, uint8_t>> fields;
	tuple<uint8_t, uint8_t, uint8_t> t;

	for (size_t i = 0; i < num_field; ++i) {
		is.get(ch);
		num = static_cast<uint8_t>(ch);
		is.get(ch);
		size = static_cast<uint8_t>(ch);
		is.get(ch);
		base = static_cast<uint8_t>(ch);

		t = make_tuple(num, size, base);
		fields.push_back(t);
	}
	//reading the first data message (gmn = 0). First check if its a data message (hdr = 0)
	is.get(ch);
	recordhdr = static_cast<uint8_t>(ch);
	temp = recordhdr >> 4; //checking the first 4 bits
	if (temp != 0) {
		cout << "Not a data message - ERROR.\n";
		return 1;
	}
	uint32_t time_created;
	for (size_t i = 0; i < num_field; ++i) {
		num = get<0>(fields[i]);
		size = get<1>(fields[i]);

		if (num == 4) {
			is.read(reinterpret_cast<char*>(&time_created), sizeof(time_created));
			cout << "time_created = " << time_created << "\n";
		}
		else {
			is.ignore(size * sizeof(char));
		}
	}

	//now I read all the file correctly
	curpos = is.tellg();
	uint32_t index = static_cast<uint32_t>(curpos);
	uint8_t lmt;

	fields.clear();

	map<uint8_t, vector<tuple<uint16_t, uint8_t, uint8_t, uint16_t>>> fields_map; //num_fields, num, size, gnm
	vector<tuple<uint16_t, uint8_t, uint8_t, uint16_t>> map_v;
	tuple<uint16_t, uint8_t, uint8_t, uint16_t> map_t;

	while (index < filesize) {
		recordhdr = message_type(is);
		lmt = recordhdr << 4;
		lmt = lmt >> 4;
		recordhdr = recordhdr >> 4;
		if (recordhdr == 0) {
			vector<tuple<uint16_t, uint8_t, uint8_t, uint16_t>> tmp_v = fields_map[lmt];
			num_field = static_cast<uint8_t>(get<0>(tmp_v[0]));
			gmn = get<3>(tmp_v[0]);
			if (gmn == 19) {
				uint16_t avg_speed;
				for (size_t i = 0; i < num_field; ++i) {
					num = get<1>(tmp_v[i]);
					size = get<2>(tmp_v[i]);

					if (num == 13) {
						is.read(reinterpret_cast<char*>(&avg_speed), sizeof(avg_speed));
						double speed = avg_speed * 0.0036;
						cout << "avg_speed = " << speed << "km/h" << "\n";
					}
					else {
						is.ignore(size * sizeof(char));
					}
				}
			}
			else {
				tuple<uint16_t, uint8_t, uint8_t> tmp_t;

				for (size_t i = 0; i < num_field; ++i) {
					num = get<1>(tmp_v[i]);
					size = get<2>(tmp_v[i]);

					is.ignore(size * sizeof(char));
				}
			}
		}
		else if (recordhdr == 4) {
			is.ignore(2 * sizeof(char));
			is.read(reinterpret_cast<char*>(&gmn), sizeof(gmn));
			is.read(reinterpret_cast<char*>(&num_field), sizeof(num_field));
			
			for (size_t i = 0; i < num_field; ++i) {
				is.get(ch);
				num = static_cast<uint8_t>(ch);
				is.get(ch);
				size = static_cast<uint8_t>(ch);
				is.get(ch);
				base = static_cast<uint8_t>(ch);

				map_t = make_tuple(num_field, num, size, gmn);
				map_v.push_back(map_t);
			}
			fields_map.emplace(lmt, map_v);
			map_v.clear();
		}
		else {
			cout << "ERROR in reading - not a record header\n" << index;
			return 1;
		}

		index = static_cast<uint32_t>(is.tellg());
	}

	uint16_t crc = 0, true_crc;
	is.read(reinterpret_cast<char*>(&true_crc), sizeof(true_crc));
	
	for (uint8_t byte : crc_buffer) {
		FitCRC_Get16(crc, byte);
	}
	if (crc == true_crc) {
		cout << "File CRC ok";
	}
	else
		return 1;

	return 0;
}


int main(int argc, char** argv)
{
	if (argc != 2) {
		return EXIT_FAILURE;
	}
	ifstream is(argv[1], ios::binary);

	return fitdump(is);
}