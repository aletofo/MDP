#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <tuple>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <cstdint>

class PDBheader {
	std::string PDBname_, cdate_, type_, creator_, records_;
	std::ifstream& is_;
	std::ofstream& os_;

	void setname(std::string s) { PDBname_ = s; }
	void setdate(std::string s) { cdate_ = s; }
	void settype(std::string s) { type_ = s; }
	void setcreator(std::string s) { creator_ = s; }
	void setrecords(std::string s) { records_ = s; }

public:

	PDBheader(std::ifstream& is, std::ofstream& os) : is_(is), os_(os) {}

	void readheader() {
		char ch;
		std::string s;
		int counter = 0;

		while (is_.get(ch)) {

			s.push_back(ch);

			if (counter == 31) {
				setname(s);
				s.clear();
			}
			if (counter == 35) {
				s.clear();
			}
			if (counter == 39) {
				setdate(s);
				s.clear();
			}
			if (counter == 59) {
				s.clear();
			}
			if (counter == 63) {
				settype(s);
				s.clear();
			}
			if (counter == 63) {
				s.clear();
			}
			if (counter == 67) {
				setcreator(s);
				s.clear();
			}
			if (counter == 75) {
				s.clear();
			}
			if (counter == 77) {
				setrecords(s);
				s.clear();
				break;
			}
			
			++counter;
		}
	}

	void writeheader() {
		os_.put(0xEF);
		os_.put(0xBB);
		os_.put(0xBF);
		os_ << "\n";
		//char BOM[] = { 0xEF, 0xBB, 0xBF };
		//os_.write(BOM, sizeof(BOM));
		//os_ << 0xEF << 0xBB << 0xBF << std::hex << "\n"; //BOM bytes

		os_ << "PDB name : " << PDBname_ << "\n";
		os_ << "Creation date (s) : " << cdate_ << "\n";
		os_ << "Type : " << type_ << "\n";
		os_ << "Creator : " << creator_ << "\n";
		os_ << "Records : " << records_ << "\n" << "\n";
	}

	size_t getrecords() {
		std::string hexstring;
		for (char ch : records_) {
			hexstring += (ch < 16 ? "0" : ""); 
			if (ch < 10) {
				hexstring += std::to_string(ch);
			}
			else {
				if (ch == 10) {
					hexstring += "A";
				}
				if (ch == 11) {
					hexstring += "B";
				}
				if (ch == 12) {
					hexstring += "C";
				}
				if (ch == 13) {
					hexstring += "D";
				}
				if (ch == 14) {
					hexstring += "E";
				}
				if (ch == 15) {
					hexstring += "F";
				}
			}
		}
		size_t n = std::stoul(hexstring, nullptr, 16);
		return n;
	}
};

class RIE {
	std::vector<std::string> offset_, ID_;
	size_t recnum_;
	std::ifstream& is_;
	std::ofstream& os_;

public:

	RIE(std::ifstream& is, std::ofstream& os, size_t num) : is_(is), os_(os), recnum_(num) {}

	std::vector<std::string> getoffsets() { return offset_; }
	std::vector<std::string> getids() { return ID_; }

	void readRIE() {
		char ch;
		std::string offset, id;
		size_t counter = 0;

		while (is_.get(ch)) {

			if (counter == recnum_) {
				break;
			}
			for (int i = 0; i < 4; i++) {
				offset.push_back(ch);
				is_.get(ch);
			}
			for (int i = 0; i < 3; i++) {
				is_.get(ch);
				id.push_back(ch);
			}
			offset_.push_back(offset);
			ID_.push_back(id);
			offset.clear();
			id.clear();
			++counter;
		}
	}

	void writeRIE() {
		for (size_t i = 0; i < recnum_; ++i) {
			os_ << i << " - offset: ";
			os_ << offset_[i] << " - id: ";
			os_ << ID_[i] << "\n";
		}
	}

};

class MOBI {
	RIE& entries_;
	std::ifstream& is_;
	std::ofstream& os_;

public:
	MOBI(std::ifstream& is, std::ofstream& os, RIE& e) : is_(is), os_(os), entries_(e) {}

	std::string getoffset(size_t index) {
		std::vector<std::string> offsets = entries_.getoffsets();
		return offsets[index];
	}
	std::string getID(size_t index) {
		std::vector<std::string> ids = entries_.getids();
		return ids[index];
	}

	void firstrecord() {
		std::vector<std::string> offsets = entries_.getoffsets();
		std::string offset = offsets[0];

		std::string hexstring;
		for (unsigned char ch : offset) {
			char buffer[3];
			std::snprintf(buffer, sizeof(buffer), "%02X", ch);
			hexstring += buffer;
		}

		size_t ofst = std::stoul(hexstring, nullptr, 16);
		is_.seekg(std::streampos(ofst));

		os_ << "Compression: ";
		os_.put(is_.get());
		os_.put(is_.get());
		is_.get();
		is_.get();
		os_ << "\n" << "TextLength: ";
		os_.put(is_.get());
		os_.put(is_.get());
		os_.put(is_.get());
		os_.put(is_.get());
		os_ << "\n" << "RecordCount: ";
		os_.put(is_.get());
		os_.put(is_.get());
		os_ << "\n" << "RecordSize: ";
		os_.put(is_.get());
		os_.put(is_.get());
		os_ << "\n" << "EncryptionType: ";
		os_.put(is_.get());
		os_.put(is_.get());
	}

	void readrecord(size_t index) {
		char ch, nextch;
		uint8_t x;
		std::string hexstring;
		std::vector<std::string> offsets = entries_.getoffsets();
		std::string offset = offsets[index];

		for (unsigned char ch : offset) {
			char buffer[3];
			std::snprintf(buffer, sizeof(buffer), "%02X", ch);
			hexstring += buffer;
		}
		size_t ofst = std::stoul(hexstring, nullptr, 16);
		is_.seekg(std::streampos(ofst));
		hexstring.clear();

		while (is_.get(ch)) {

			x = static_cast<uint8_t>(ch);
			if (x == 0x00) {
				break;
			}
			else if (x >= 0x01 && x <= 0x08) {
				for (int i = 0; i < 8; ++i) {
					is_.get(ch);
					os_.put(ch);
				}
			}
			else if (x >= 0x09 && x <= 0x7F) {
				os_.put(ch);
			}
			else if (x >= 0x80 && x <= 0xBF) {
				is_.get(nextch);
				
				char buffer[3];
				std::snprintf(buffer, sizeof(buffer), "%02X", ch);
				hexstring += buffer;
				std::snprintf(buffer, sizeof(buffer), "%02X", nextch);
				hexstring += buffer;
				
				uint16_t seq = static_cast<uint16_t>(std::stoul(hexstring, nullptr, 16));

				seq = seq << 2;
				uint16_t dist = 0; 
				uint8_t length = 0;
				for (int i = 15; i >= 2; --i) {
					if (i < 5 && i >= 2) {
						length = (length << 1) | ((seq >> i) & 1);
					}
					else { dist = (dist << 1) | ((seq >> i) & 1); }
				}
			}
			else if (x >= 0xC0 && x <= 0xFF) {

			}
		}
	}

	void writerecord() {

	}
};

void MOBIdecode(std::ifstream& is, std::ofstream& os) {
	PDBheader pdb(is, os);
	pdb.readheader();
	pdb.writeheader();

	size_t records = pdb.getrecords();
	//std::cout << records;

	RIE entries(is, os, records);
	entries.readRIE();
	entries.writeRIE();

	MOBI mobi(is, os, entries);
	mobi.firstrecord();
	for (size_t i = 1; i < records; ++i) {
		mobi.readrecord(i);
	}
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

	MOBIdecode(is, os);

	return EXIT_SUCCESS;
}