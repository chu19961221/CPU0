#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <cstdlib>
#include <sstream>

using namespace std;

void read_op_txt(string op_table[], uint8_t op_code[], int op_type[], int* length);
void config_out(string out);
string to_hex_string(const unsigned int i);

int main() {
	string op_table[100];
	uint8_t op_code[100] = { 0 };
	int op_type[100] = { 0 };
	int op_length = 0;
	read_op_txt(op_table, op_code, op_type, &op_length);
	string out;
	for (int i = 0; i < op_length; i++) {
		out += "`define " + op_table[i] + " 8'h" + to_hex_string((unsigned int)op_code[i]) + "\n";
	}
	config_out(out);
	
	return 0;
}

void read_op_txt(string op_table[], uint8_t op_code[], int op_type[], int* length) {
	int state = 0;
	int count = 0;
	char buff;
	string op_string[1000];

	fstream file;
	file.open("./config/op.txt", ios::in);
	if (!file) cout << "op_tableÀÉ®×¶}±Ò¥¢±Ñ\n";
	else {
		while (file.eof() != 1) {
			file.get(buff);
			if (buff != ' ' && buff != ',' && buff != '\n' && buff != '\t' && buff != ';' && buff != ':' && buff != '\0') {
				op_string[count] += buff;
				state = 1;
			}
			else {
				if (state == 1) {
					count++;
				}
				state = 0;
			}
		}
		file.close();
		if (state == 1) {
			count++;
		}
		for (int i = 0; i < count; i++) {
			if (i % 3 == 0) {
				op_table[i / 3] = op_string[i];
				cout << op_table[i / 3] << "=";
			}
			else if (i % 3 == 1) {
				int x;
				stringstream ss;
				ss << hex << op_string[i];
				ss >> x;
				op_code[i / 3] = (uint8_t)x;
				cout << (int)op_code[i / 3] << " ";
			}
			else {
				int x;
				stringstream ss;
				ss << dec << op_string[i];
				ss >> x;
				op_type[i / 3] = x;
				cout << op_type[i / 3] << " type, ";
			}
		}
		*length = count / 3;
		cout << "\nAll " << *length << " op\nImport op.txt finish\n";
	}
}

void config_out(string out) {
	char f_out[] = "./output/config.v";
	ofstream fout(f_out, std::ios::out);
	//ofstream fout("led.bin", std::ios::binary);
	fout.write(out.c_str(), out.length());
	fout.close();
}

string to_hex_string(const unsigned int i)
{
	string out;
	stringstream s;
	s << hex << i;
	out += s.str();
	return out;
}