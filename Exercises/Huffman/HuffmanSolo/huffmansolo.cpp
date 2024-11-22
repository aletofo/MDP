#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <bit>
#include <algorithm>
#include <unordered_map>
#include <ranges>

using namespace::std;

class bitwriter {
	uint8_t buffer_;
	size_t n_ = 0;
	std::ostream& os_;

	void writebit(uint64_t curbit) {
		buffer_ = (buffer_ << 1) | (curbit & 1);
		++n_;
		if (n_ == 8) {
			os_.put(buffer_);
			n_ = 0;
		}
	}
public:
	bitwriter(std::ostream& os) : os_(os) {}
	~bitwriter() {
		flush();
	}
	std::ostream& operator()(uint64_t x, uint64_t numbits) {
		while (numbits-- > 0) {
			writebit(x >> numbits);
		}
		return os_;
	}
	std::ostream& flush(int padbit = 0) {
		while (n_ > 0) {
			writebit(padbit);
		}
		return os_;
	}
};

class bitreader {
	uint8_t buffer_;
	size_t n_ = 0;
	std::istream& is_;

	uint64_t readbit() {
		if (n_ == 0) {
			buffer_ = is_.get();
			n_ = 8;
		}
		--n_;
		return (buffer_ >> n_) & 1;
	}
public:
	bitreader(std::istream& is) : is_(is) {}
	uint64_t operator()(uint64_t numbits) {
		uint64_t u = 0;
		while (numbits-- > 0) {
			u = (u << 1) | readbit();
		}
		return u;
	}
	bool fail() const {
		return is_.fail();
	}
	operator bool() const {
		return !fail();
	}
};

//template<typename T, typename CT = uint64_t>
unordered_map<uint8_t, uint64_t> frequencies(vector<uint8_t>& v) {
	unordered_map<uint8_t, uint64_t> freq;

	for (const auto& x : v) {
		//unsigned char u = static_cast<unsigned char>(x);
		freq[x] += 1;
	}

	//sort(freq.begin(), freq.end());
	return freq;
}

struct huffman {
	struct node {
		uint8_t sym_;
		uint64_t freq_;
		node* left_;
		node* right_;
	};
	struct nodeptr_less {
		bool operator()(const node* a, const node* b) {
			return a->freq_ > b->freq_;
		}
	};

	vector<unique_ptr<node>> nodes_;

	template<typename... Ts>
	node* new_node(Ts... args) {
		nodes_.emplace_back(std::make_unique<node>(std::forward<Ts>(args)...));
		return nodes_.back().get();
	}

	std::unordered_map<uint8_t, std::pair<uint64_t, uint64_t>> code_map; // sym -> {code, len}

	void generate_codes(const node* n, uint64_t code = 0, uint64_t len = 0) {
		if (n->left_ == 0) {
			code_map[n->sym_] = { code, len };
		}
		else {
			generate_codes(n->left_, (code << 1) | 0, len + 1);
			generate_codes(n->right_, (code << 1) | 1, len + 1);
		}
	}

	huffman() {};

	huffman(unordered_map<uint8_t, uint64_t> map){
		vector<node*> v;
		for (const auto& [s, f] : map) {
			v.emplace_back(new_node(s, f)); //'emplace' puts directly in the container the object without copies
		}
		
		ranges::sort(v, nodeptr_less{});

		while (v.size() > 1) {
			node* n1 = v.back();
			v.pop_back();
			node* n2 = v.back();
			v.pop_back();
			node* n = new_node(0, n1->freq_ + n2->freq_, n1, n2);

			auto pos = ranges::lower_bound(v, n, nodeptr_less{});
			v.insert(pos, n);
		}
		node* root = v.back();
		v.pop_back();

		generate_codes(root);
	}
};

void compress(const string& input_filename, const string& output_filename) {
	std::ifstream is(input_filename, std::ios::binary);
	if (!is) {
		cout << "ERROR: cannot open file for reading";
	}
	std::ofstream os(output_filename, std::ios::binary);
	if (!os) {
		cout << "ERROR: cannot open file for writing";
	}
	vector<uint8_t> v(istreambuf_iterator<char>{is}, istreambuf_iterator<char>() );

	unordered_map<uint8_t, uint64_t> freq_map = frequencies(v);
	huffman h(freq_map);

	bitwriter bw(os);

	os << "HUFFMAN1";
	os.put(static_cast<char>(freq_map.size()));
	for (const auto& [sym, x] : h.code_map) {
		auto&& [code, len] = x;
		bw(sym, 8);
		bw(len, 5);
		bw(code, len);
	}
	bw(v.size(), 32);
	for (const auto& x : v) {
		auto&& [code, len] = h.code_map[x];
		bw(code, len);
	}
}

void decompress(const string& input_filename, const string& output_filename) {
	std::ifstream is(input_filename, std::ios::binary);
	if (!is) {
		cout << "ERROR: cannot open file for reading";
	}
	std::ofstream os(output_filename, std::ios::binary);
	if (!os) {
		cout << "ERROR: cannot open file for writing";
	}

	bitreader br(is);

	std::string magic(8, ' ');
	is.read(magic.data(), 8);
	if (magic != "HUFFMAN1") {
		return;
	}
	size_t tblsize = is.get(); //next 8bits we will read represent the number of items in Huffman Table
	if (tblsize == 0) {
		tblsize = 256;
	}
	//building up the Huffman Table using a vector of tuples
	vector<tuple<uint64_t, uint8_t, uint64_t>> tbl;
	for (size_t i = 0; i < tblsize; i++) {
		uint8_t sym = br(8);
		uint64_t len = br(5);
		uint64_t code = br(len);
		tbl.emplace_back(len, sym, code);
	}
	size_t numsym = br(32);
	std::ranges::sort(tbl);
	//reading the compressed data
	uint64_t read_len;
	uint64_t buffer;
	for (size_t i = 0; i < numsym; i++) {
		read_len = 0;
		buffer = 0;
		for (size_t j = 0; j < tblsize; j++) {
			auto&& [len, sym, code] = tbl[j];
			buffer = (buffer << (len - read_len)) | br(len - read_len);
			read_len = len;
			if (buffer == code) {
				os.put(sym);
				break;
			}
		}
	}
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		cout << "ERROR: number of parameters insufficient.";
		exit(EXIT_FAILURE);
	}
	string command = argv[1];
	if (command == "c") {
		compress(argv[2], argv[3]);
	}
	else if (command == "d") {
		decompress(argv[2], argv[3]);
	}
	else {
		cout << "ERROR: first parameter is not 'c' or 'd'";
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}

