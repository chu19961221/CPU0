#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <cstdlib>
#include <sstream>

#define start_pc		0
#define ISR_num			16
#define ISR_VEC_size	ISR_num*4
#define boot_len		2	//init SP, JMP main
#define main_pc			ISR_VEC_size + boot_len
#define reg_length 16
#define ram_size 0x4000

using namespace std;

string r[reg_length] = { "R0","R1","R2","R3","R4","R5","R6","R7","R8","R9","R10","R11","SW","SP","LR","PC" };
string ISR[ISR_num];
char pre_code[] = "JMP ISR_BOOT JMP ISR_GPIO JMP ISR_UART JMP ISR_TIM1 ORG 76 ISR_BOOT: LDI SP,R0,16380 JMP main\n";

struct asm_list {
	string code;
	asm_list *next;
};
struct equ_list {
	string code;
	string txt;
	equ_list *next;
};
struct flg_list {
	string code;
	uint32_t addr;
	flg_list *next;
};
struct bin_list {
	string code;
	uint32_t addr;
	bin_list *next;
};
struct add_list {
	string code;
	add_list *next;
};
struct link_list {
	add_list *head;
	link_list *next;
};

void read_op_txt(string op_table[], uint8_t op_code[], int op_type[], int* length);
void add_link(asm_list *asm_head);
asm_list* read_asm(char* src, int *asm_length);
void equ_replace(asm_list* asm_head);
flg_list* cr_flg_table(asm_list *asm_head, int *flg_length, string op_table[], int op_length);
void add_ISR_label(asm_list* asm_head, flg_list* flg_head);
asm_list* add_pre_code(asm_list *asm_head);
bin_list* cr_bin(asm_list *asm_head, flg_list *flg_head, string op_table[], uint8_t op_code[], int op_type[], int op_length);
int out_bin(bin_list *bin_head, char* des);

uint32_t str2uint(string in);
string to_hex_string(const unsigned int i);
uint32_t calculator(int le, int ri, const char *s);
string rpls(string t, flg_list *flg_head);

int main(int argc, char* argv[])
{
	ISR[0] = "ISR_BOOT";
	ISR[1] = "ISR_GPIO";
	ISR[2] = "ISR_UART";
	ISR[3] = "ISR_TIM1";

	//import op.txt
	string op_table[100];
	uint8_t op_code[100] = { 0 };
	int op_type[100] = { 0 };
	int op_length = 0;
	read_op_txt(op_table, op_code, op_type, &op_length);

	//read asm code
	int asm_length = 0;
	asm_list *asm_head = read_asm(argv[1], &asm_length);

	//import link table
	add_link(asm_head);
	asm_list *asm_c = asm_head;
	while (asm_c != NULL) {
		cout << asm_c->code << endl;
		asm_c = asm_c->next;
	}

	//EQU replace
	equ_replace(asm_head);

	//add boot code
	asm_head = add_pre_code(asm_head);

	//create flags table
	int flg_length = 0;
	flg_list *flg_head = cr_flg_table(asm_head, &flg_length, op_table, op_length);

	//add ISR label
	add_ISR_label(asm_head, flg_head);

	//create flags table
	flg_head = cr_flg_table(asm_head, &flg_length, op_table, op_length);

	//convert to bin
	int bin_length = 0;
	bin_list *bin_head = cr_bin(asm_head, flg_head, op_table, op_code, op_type, op_length);

	//create bin file
	int out_state = 0;
	//char f_name[] = "v2_sample";
	out_state = out_bin(bin_head, argv[1]);
	if (out_state < 0) {
		cout << "Output bin file error!\n";
	}

	//system("pause");
	return 0;
}

void read_op_txt(string op_table[], uint8_t op_code[], int op_type[], int* length) {
	int state = 0;
	int count = 0;
	char buff;
	string op_string[1000];

	fstream file;
	file.open("./config/op.txt", ios::in);
	if (!file) cout << "op_table檔案開啟失敗\n";
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

void add_link(asm_list *asm_head)
{
	add_list *add_head = new add_list;
	add_list *add_current = add_head;
	add_current->next = NULL;

	link_list *ll_head = new link_list;
	link_list *ll_current = ll_head;
	ll_current->head = add_head;
	ll_current->next = NULL;

	int state = 0;
	int count = 0;
	char buff;
	fstream file;
	file.open("./config/agenda.txt", ios::in);
	if (!file) cout << "asm檔案開啟失敗\n";
	else {
		while (file.eof() != 1) {
			file.get(buff);
			//cout << buff;
			if (buff != ' ' && buff != ',' && buff != '\n' && buff != '\t' && buff != ';' && buff != ':' && buff != '\0' && buff != '/') {
				if (state == 0) {
					if (add_current->code != "") {
						add_current->next = new add_list;
						add_current = add_current->next;
						add_current->next = NULL;
					}
					add_current->code += buff;
					state = 1;
				}
				else if (state == 1) {
					add_current->code += buff;
					state = 1;
				}
			}
			else if (buff == '/') {
				if (add_current != add_head) {
					add_head = new add_list;
					add_current = add_head;
					add_current->next = NULL;
					ll_current->next = new link_list;
					ll_current = ll_current->next;
					ll_current->head = add_head;
					ll_current->next = NULL;
				}
			}
			else if (buff == ':') {
				if (state == 0 || state == 1) {
					if (state == 1) {
						count++;
					}
					add_current->next = new add_list;
					add_current = add_current->next;
					add_current->next = NULL;
					add_current->code += buff;
					count++;
					state = 0;
				}
			}
			else if (buff == ';') {
				if (state == 1) {
					count++;
				}
				state = 2;
			}
			else if (buff == '\n') {
				if (state == 1) {
					count++;
				}
				state = 0;
			}
			else {
				if (state == 1 || state == 0) {
					if (state == 1) {
						count++;
					}
					state = 0;
				}
			}
		}
		//cout << endl;
		file.close();
		if (state == 1) {
			count++;
		}
	}
	cout << "link table create finish\n";
	
	asm_list* asm_current = asm_head;
	asm_list* asm_last = asm_current;
	while (asm_current != NULL) {
		ll_current = ll_head;
		cout << "1";
		if (asm_current->code == ":") {
			while (ll_current != NULL) {
				add_current = ll_current->head;
				cout << "2";
				if (asm_last->code == add_current->code) {
					add_current = add_current->next;
					if (add_current->code == ":") {
						add_current = add_current->next;
						while (add_current != NULL) {
							cout << "3";
							asm_list *temp = new asm_list;
							temp->code = add_current->code;
							temp->next = asm_current->next;
							asm_current->next = temp;
							asm_current = asm_current->next;
							add_current = add_current->next;
						}
					}
				}
				ll_current = ll_current->next;
			}
		}
		asm_last = asm_current;
		asm_current = asm_current->next;
	}
}

asm_list* read_asm(char* src, int *asm_length) {
	string f_in = src;
	f_in = "./asm/" + f_in + ".s";
	int n = f_in.length();
	char *f_name = new char[n+1];
	strcpy_s(f_name, n+1, f_in.c_str());

	asm_list *head = new asm_list;
	asm_list *current = head;
	current->next = NULL;
	int state = 0;
	int count = 0;
	char buff;

	fstream file;
	//file.open("led.txt", ios::in);
	file.open(f_name, ios::in);
	if (!file) cout << "asm檔案開啟失敗\n";
	else {
		while (file.eof() != 1) {
			file.get(buff);
			cout << buff;
			if (buff != ' ' && buff != ',' && buff != '\n' && buff != '\t' && buff != ';' && buff != ':' && buff != '\0') {
				if (state == 0) {
					if (count != 0) {
						asm_list *temp = new asm_list;
						current->next = temp;
						current = temp;
						current->next = NULL;
					}
					current->code += buff;
					state = 1;
				}
				else if (state == 1) {
					current->code += buff;
					state = 1;
				}
			}
			else if (buff == ':') {
				if (state == 0 || state == 1) {
					if (state == 1) {
						count++;
					}
					asm_list *temp = new asm_list;
					current->next = temp;
					current = temp;
					current->next = NULL;
					current->code += buff;
					count++;
					state = 0;
				}
			}
			else if (buff == ';') {
				if (state == 1) {
					count++;
				}
				state = 2;
			}
			else if (buff == '\n') {
				if (state == 1) {
					count++;
				}
				state = 0;
			}
			else {
				if (state == 1 || state == 0) {
					if (state == 1) {
						count++;
					}
					state = 0;
				}
			}
		}
		cout << endl;
		file.close();
		if (state == 1) {
			count++;
		}
		*asm_length = count;
		/*current = head;
		int end = 0;
		while(!end){
			cout<<current->code<<endl;
			if(current->next == NULL){
				end = 1;
			}
			else{
				current = current->next;
			}
		}
		cout<<*asm_length<<endl;*/
	}
	return head;
}

void equ_replace(asm_list* asm_head) {
	asm_list *asm_cur = asm_head;
	asm_list *asm_last = asm_cur;
	equ_list *equ_head = new equ_list;
	equ_list *equ_cur = equ_head;
	equ_cur->next = NULL;
	int count = 0;
	while (asm_cur != NULL) {
		if (asm_cur->code == "EQU") {
			if (count != 0) {
				equ_list *temp = new equ_list;
				equ_cur->next = temp;
				equ_cur = temp;
				equ_cur->next = NULL;
			}
			equ_cur->code = asm_last->code;
			int f = asm_cur->next->code.find("<");
			if (f >= 0) {
				equ_cur->txt = asm_cur->next->code.substr(1, asm_cur->next->code.length() -2);
			}
			else{
				equ_cur->txt = asm_cur->next->code;
			}
			cout << "----------------------------\nReplace txt:\n" << equ_cur->code << "\t" << equ_cur->txt << endl;
		}
		asm_last = asm_cur;
		asm_cur = asm_cur->next;
	}
	asm_cur = asm_head;
	asm_last = asm_cur;
	while (asm_cur != NULL) {
		equ_cur = equ_head;
		while (equ_cur != NULL) {
			if (asm_cur->code == equ_cur->code) {
				asm_cur->code = equ_cur->txt;
			}
			equ_cur = equ_cur->next;
		}
		asm_last = asm_cur;
		asm_cur = asm_cur->next;
	}
}

asm_list* add_pre_code(asm_list *asm_head) {
	asm_list* new_head = new asm_list;
	asm_list* asm_cur = new_head;
	asm_cur->next = NULL;
	string buf;
	uint8_t	state = 0;
	int count = 0;

	char buff;
	string bootcode[1000];

	fstream file;
	file.open("./config/boot.txt", ios::in);
	if (!file) cout << "boot.txt檔案開啟失敗\n";
	else {
		while (file.eof() != 1) {
			file.get(buff);
			
			if (buff != ' ' && buff != ',' && buff != '\n' && buff != '\t' && buff != ';' && buff != ':' && buff != '\0') {
				if (state == 0) {
					if (count != 0) {
						asm_list *temp = new asm_list;
						asm_cur->next = temp;
						asm_cur = temp;
						asm_cur->next = NULL;
					}
					asm_cur->code += buff;
					state = 1;
				}
				else if (state == 1) {
					asm_cur->code += buff;
					state = 1;
				}
			}
			else if (buff == ':') {
				if (state == 0 || state == 1) {
					if (state == 1) {
						count++;
					}
					asm_list *temp = new asm_list;
					asm_cur->next = temp;
					asm_cur = temp;
					asm_cur->next = NULL;
					asm_cur->code += buff;
					count++;
					state = 0;
				}
			}
			else if (buff == ';') {
				if (state == 1) {
					count++;
				}
				state = 2;
			}
			else if (buff == '\n') {
				if (state == 1) {
					count++;
				}
				state = 0;
			}
			else {
				if (state == 1 || state == 0) {
					if (state == 1) {
						count++;
					}
					state = 0;
				}
			}
		}
		file.close();
	}

	asm_cur->next = asm_head;
	return new_head;
}

flg_list* cr_flg_table(asm_list *asm_head, int *flg_length, string op_table[], int op_length) {
	uint32_t pc = main_pc;
	asm_list *asm_cur = asm_head;
	asm_list *asm_last = asm_cur;
	flg_list *flg_head = new flg_list;
	flg_list *flg_cur = flg_head;
	flg_cur->next = NULL;
	int count = 0;
	while (asm_cur != NULL) {
		if (asm_cur->code == ":") {
			if (count != 0) {
				flg_list *temp = new flg_list;
				flg_cur->next = temp;
				flg_cur = temp;
				flg_cur->next = NULL;
			}
			flg_cur->code = asm_last->code;
			flg_cur->addr = pc;
			count++;
		}
		else if (asm_cur->code == "ORG" || asm_cur->code == "org") {
			pc = str2uint(asm_cur->next->code);
		}
		else if(asm_cur->code == "WORD" || asm_cur->code == "word") {
			pc += 4;
		} 
		else if (asm_cur->code == "RESW" || asm_cur->code == "resw") {
			pc += (str2uint(asm_cur->next->code) * 4);
		}
		else {
			for (int j = 0; j < op_length; j++) {
				if (asm_cur->code == op_table[j]) {
					pc += 4;
				}
			}
		}
		asm_last = asm_cur;
		asm_cur = asm_cur->next;
	}

	*flg_length = count;
	cout << "------------------------------------\n";
	cout << "Flag table:\n";
	flg_cur = flg_head;
	while (flg_cur != NULL) {
		cout << flg_cur->code << "\t0x" << hex << flg_cur->addr << endl;
		flg_cur = flg_cur->next;
	}
	cout << dec;
	if (*flg_length == 0) {
		return NULL;
	}
	else {
		return flg_head;
	}
}

void add_ISR_label(asm_list* asm_head, flg_list* flg_head) {
	asm_list* asm_cur = asm_head;
	flg_list* flg_cur = flg_head;
	int ISR_check[ISR_num] = { 0 };
	while (flg_cur != NULL) {
		for (int i = 0; i < ISR_num; i++) {
			if (flg_cur->code == ISR[i]) {
				ISR_check[i] = 1;
			}
		}
		flg_cur = flg_cur->next;
	}
	while (asm_cur->next != NULL) {
		asm_cur = asm_cur->next;
	}
	for (int i = 0; i < ISR_num; i++) {
		if (ISR_check[i] == 0 && ISR[i] != "") {
			asm_list* tmp1 = new asm_list;
			asm_cur->next = tmp1;
			asm_cur = tmp1;
			asm_cur->code = ISR[i];
			asm_list* tmp2 = new asm_list;
			asm_cur->next = tmp2;
			asm_cur = tmp2;
			asm_cur->code = ":";
			asm_list* tmp3 = new asm_list;
			asm_cur->next = tmp3;
			asm_cur = tmp3;
			asm_cur->code = "IRET";
			asm_cur->next = NULL;
		}
	}
}

bin_list* cr_bin(asm_list *asm_head, flg_list *flg_head, string op_table[], uint8_t op_code[], int op_type[], int op_length) {
	asm_list *asm_cur = asm_head;
	asm_list *asm_last = asm_head;
	flg_list *flg_cur = flg_head;
	bin_list *bin_head = new bin_list;
	bin_list *bin_cur = bin_head;
	bin_cur->next = NULL;
	uint32_t pc = 0;
	uint32_t bin_buff = 0;
	int state = 0;
	int type = 0;
	int count = 0;
	int end = 0;
	
	while (!end) {
		if (state == 0) {
			if (asm_cur->code == "ORG" || asm_cur->code == "org") {
				pc = str2uint(asm_cur->next->code);
			}
			else if (asm_cur->code == "WORD" || asm_cur->code == "word") {
				if (count != 0) {
					bin_list *temp = new bin_list;
					bin_cur->next = temp;
					bin_cur = temp;
					bin_cur->next = NULL;
				}
				//bin_buff = str2uint(asm_cur->next->code);
				string tt = rpls(asm_cur->next->code, flg_head);
				bin_buff = (calculator(0, tt.size() - 1, tt.c_str()) & 0xFFFFFFFF);
				bin_cur->code = to_hex_string(bin_buff);
				bin_cur->addr = pc;
				bin_buff = 0;
				pc += 4;
				count++;
			}
			else if (asm_cur->code == "RESW" || asm_cur->code == "resw") {
				uint32_t buff_len = str2uint(asm_cur->next->code);
				for (uint32_t i = 0; i < buff_len; i++) {
					if (count != 0) {
						bin_list *temp = new bin_list;
						bin_cur->next = temp;
						bin_cur = temp;
						bin_cur->next = NULL;
					}
					bin_buff = 0;
					bin_cur->code = to_hex_string(bin_buff);
					bin_cur->addr = pc;
					pc += 4;
					count++;
				}
			}
			else {
				for (int i = 0; i < op_length; i++) {
					if (asm_cur->code == op_table[i]) {
						if (count != 0) {
							bin_list *temp = new bin_list;
							bin_cur->next = temp;
							bin_cur = temp;
							bin_cur->next = NULL;
						}
						type = op_type[i];
						bin_buff = (uint32_t)op_code[i] << 24;
						state = 1;
					}
				}
				if (state == 1) {
					if (type == 1) {
						bin_cur->code = to_hex_string(bin_buff);
						bin_cur->addr = pc;
						state = 0;
					}

					else if (type == 2) {
						int flag = 0;
						int end_1 = 0;
						while (!end_1) {
							if (asm_cur->next->code == flg_cur->code) {
								bin_buff += (((flg_cur->addr - pc) - 4) & 0xFFFFFF);
								flag = 1;
							}
							if (flg_cur->next == NULL) {
								end_1 = 1;
							}
							else {
								flg_cur = flg_cur->next;
							}
						}
						/*if (!flag) {
							//bin_buff += (str2uint(asm_cur->next->code) & 0xFFFFFF);
							bin_buff += (calculator(0, asm_cur->next->code.size() - 1, asm_cur->next->code.c_str()) & 0xFFFFFF);
						}*/
						bin_cur->code = to_hex_string(bin_buff);
						bin_cur->addr = pc;
						flg_cur = flg_head;
						state = 0;
					}

					else if (type == 3) {
						int flag = 0;
						for (int i = 0; i < reg_length; i++) {
							if (asm_cur->next->code == r[i]) {
								bin_buff += ((uint32_t)i << 20);
								flag = 1;
							}
						}
						if (!flag) {
							cout << "asm error: " << asm_cur->code << endl;
							state = 2;
							end = 1;
						}
						else {
							bin_cur->code = to_hex_string(bin_buff);
							bin_cur->addr = pc;
							state = 0;
						}
					}

					else if (type == 4) {
						int flag = 0;
						for (int i = 0; i < reg_length; i++) {
							if (asm_cur->next->code == r[i]) {
								bin_buff += ((uint32_t)i << 20);
								flag = 1;
							}
						}
						if (!flag) {
							cout << "asm error: " << asm_cur->code << endl;
							state = 2;
							end = 1;
						}
						else {
							flag = 0;
							for (int i = 0; i < reg_length; i++) {
								if (asm_cur->next->next->code == r[i]) {
									bin_buff += ((uint32_t)i << 16);
									flag = 1;
								}
							}
							if (!flag) {
								cout << "asm error: " << asm_cur->code << endl;
								state = 2;
								end = 1;
							}
							else {
								bin_cur->code = to_hex_string(bin_buff);
								bin_cur->addr = pc;
								state = 0;
							}
						}
					}

					else if (type == 5) {
						int flag = 0;
						for (int i = 0; i < reg_length; i++) {
							if (asm_cur->next->code == r[i]) {
								bin_buff += ((uint32_t)i << 20);
								flag = 1;
							}
						}
						if (!flag) {
							cout << "asm error: " << asm_cur->code << endl;
							state = 2;
							end = 1;
						}
						else {
							flag = 0;
							for (int i = 0; i < reg_length; i++) {
								if (asm_cur->next->next->code == r[i]) {
									bin_buff += ((uint32_t)i << 16);
									flag = 1;
								}
							}
							if (!flag) {
								cout << "asm error: " << asm_cur->code << endl;
								state = 2;
								end = 1;
							}
							else {
								flag = 0;
								int end_1 = 0;
								while (flg_cur != NULL) {
									if (asm_cur->next->next->next->code == flg_cur->code) {
										bin_buff += (flg_cur->addr & 0xFFFF);
										flag = 1;
									}
									flg_cur = flg_cur->next;
								}
								if (!flag) {
									//bin_buff += (str2uint(asm_cur->next->next->next->code) & 0xFFFF);
									string tt = rpls(asm_cur->next->next->next->code, flg_head);
									bin_buff += (calculator(0, tt.size() - 1, tt.c_str()) & 0xFFFF);
									//bin_buff += (calculator(0, asm_cur->next->next->next->code.size() - 1, asm_cur->next->next->next->code.c_str()) & 0xFFFF);
								}
								bin_cur->code = to_hex_string(bin_buff);
								bin_cur->addr = pc;
								flg_cur = flg_head;
								state = 0;
							}
						}
					}

					else if (type == 6) {
						int flag = 0;
						for (int i = 0; i < reg_length; i++) {
							if (asm_cur->next->code == r[i]) {
								bin_buff += ((uint32_t)i << 20);
								flag = 1;
							}
						}
						if (!flag) {
							cout << "asm error: " << asm_cur->code << endl;
							state = 2;
							end = 1;
						}
						else {
							flag = 0;
							for (int i = 0; i < reg_length; i++) {
								if (asm_cur->next->next->code == r[i]) {
									bin_buff += ((uint32_t)i << 16);
									flag = 1;
								}
							}
							if (!flag) {
								cout << "asm error: " << asm_cur->code << endl;
								state = 2;
								end = 1;
							}
							else {
								flag = 0;
								for (int i = 0; i < reg_length; i++) {
									if (asm_cur->next->next->next->code == r[i]) {
										bin_buff += ((uint32_t)i << 12);
										flag = 1;
									}
								}
								if (!flag) {
									cout << "asm error: " << asm_cur->code << endl;
									state = 2;
									end = 1;
								}
								else {
									bin_cur->code = to_hex_string(bin_buff);
									bin_cur->addr = pc;
									state = 0;
								}
							}
						}
					}

					else if (type == 7) {
						int flag = 0;
						for (int i = 0; i < reg_length; i++) {
							if (asm_cur->next->code == r[i]) {
								bin_buff += ((uint32_t)i << 20);
								flag = 1;
							}
						}
						if (!flag) {
							cout << "asm error: " << asm_cur->code << endl;
							state = 2;
							end = 1;
						}
						else {
							flag = 0;
							for (int i = 0; i < reg_length; i++) {
								if (asm_cur->next->next->code == r[i]) {
									bin_buff += ((uint32_t)i << 16);
									flag = 1;
								}
							}
							if (!flag) {
								cout << "asm error: " << asm_cur->code << endl;
								state = 2;
								end = 1;
							}
							else {
								string tt = rpls(asm_cur->next->next->next->code, flg_head);
								bin_buff += (calculator(0, tt.size() - 1, tt.c_str()) & 0x1F);
								//bin_buff += (calculator(0, asm_cur->next->next->next->code.size() - 1, asm_cur->next->next->next->code.c_str()) & 0x1F);
								bin_cur->code = to_hex_string(bin_buff);
								bin_cur->addr = pc;
								state = 0;
							}
						}
					}

					bin_buff = 0;
					pc += 4;
					count++;
				}
			}
		}
		if (asm_cur->next == NULL) {
			end = 1;
		}
		else {
			asm_cur = asm_cur->next;
		}
	}
	if (state != 0) {
		cout << "Compile error !!\nEnd program\n";
		return NULL;
	}
	else {
		cout << "------------------------------------\n";
		cout << "bin code:\n";
		end = 0;
		bin_cur = bin_head;
		while (!end) {
			cout << "0x" << hex << bin_cur->addr << "\t" << bin_cur->code << endl;
			if (bin_cur->next == NULL) {
				end = 1;
			}
			else {
				bin_cur = bin_cur->next;
			}
		}
		return bin_head;
	}
}

int out_bin(bin_list *bin_head, char* des) {
	int state = 0;
	uint32_t pc = 0;
	bin_list *bin_cur = bin_head;

	string f_in = des;
	f_in = "./bin/" + f_in + ".bin";
	int n = f_in.length();
	char *f_name = new char[n + 1];
	strcpy_s(f_name, n+1, f_in.c_str());
	ofstream fout(f_name, std::ios::binary);
	//ofstream fout("led.bin", std::ios::binary);
	while (bin_cur != NULL) {
		if (bin_cur->addr > pc) {
			string out;
			out += (uint8_t)0xFF;
			out += (uint8_t)0xFF;
			out += (uint8_t)0xFF;
			out += (uint8_t)0xFF;
			fout.write(out.c_str(), sizeof(char)*(out.size()));
			pc += 4;
		}
		else {
			string out;
			uint32_t x;
			stringstream ss;
			ss << hex << bin_cur->code;
			ss >> x;
			uint8_t b[4];
			b[3] = (uint8_t)(x & 0x000000FF);
			b[2] = (uint8_t)((x & 0x0000FF00) >> 8);
			b[1] = (uint8_t)((x & 0x00FF0000) >> 16);
			b[0] = (uint8_t)((x & 0xFF000000) >> 24);
			for (int j = 0; j < 4; j++) {
				out += b[j];
			}
			fout.write(out.c_str(), sizeof(char)*(out.size()));
			bin_cur = bin_cur->next;
			pc += 4;
		}
	}
	fout.close();
	return state;
}

uint32_t str2uint(string in)
{
	string a, b;
	a = in.substr(in.length() - 1, 1);
	if (a == "H" || a == "h") {
		b = in.substr(0, in.length() - 1);
		uint32_t x;
		stringstream ss;
		ss << hex << b;
		ss >> x;
		return x;
	}
	else {
		b = in.substr(0, in.length());
		uint32_t x;
		stringstream ss;
		ss << dec << b;
		ss >> x;
		return x;
	}
}

string to_hex_string(const unsigned int i)
{
	string out;
	uint32_t mask = 0xF;
	for (int j = 0; j < 8; j++) {
		stringstream s;
		s << hex << ((i & (mask << (28 - (j * 4)))) >> (28 - (j * 4)));
		out += s.str();
	}
	return out;
}

uint32_t calculator(int le, int ri, const char *s) {
	int c = 0;
	for (int i = ri; i >= le; i--) {  //後加減先運算 
		if (s[i] == ')') c++;    //括弧內部分割      
		if (s[i] == '(') c--;
		if (s[i] == '+' && c == 0) { //後加減，先遞迴 
			return calculator(le, i - 1, s) + calculator(i + 1, ri, s);
		}
		if (s[i] == '-' && c == 0) {
			return calculator(le, i - 1, s) - calculator(i + 1, ri, s);
		}
	}
	for (int i = ri; i >= le; i--) {  //先乘除，後遞迴  
		if (s[i] == ')') c++;    //括弧內部分割      
		if (s[i] == '(') c--;
		if (s[i] == '*' && c == 0) { //先乘除，後遞迴 
			return calculator(le, i - 1, s) * calculator(i + 1, ri, s);
		}
		if (s[i] == '/' && c == 0) {
			return calculator(le, i - 1, s) / calculator(i + 1, ri, s);
		}
		if (s[i] == '%' && c == 0) {
			return calculator(le, i - 1, s) % calculator(i + 1, ri, s);
		}
	}
	if ((s[le] == '(') && (s[ri] == ')')) return calculator(le + 1, ri - 1, s);  //去除刮號
	if (s[le] == ' ' && s[ri] == ' ') return  calculator(le + 1, ri - 1, s); //去除左右兩邊空格 
	if (s[le] == ' ') return  calculator(le + 1, ri, s); //去除左邊空格 
	if (s[ri] == ' ') return  calculator(le, ri - 1, s); //去除右邊空格
	long long int num = 0;

	for (int i = le; i <= ri; i++) num = num * 10 + s[i] - '0';
	return num;
}
string rpls(string t, flg_list *flg_head) {
	flg_list *flg_cur = flg_head;
	string buff, flag;
	int state = 0, find = 0;
	for (int i = 0; i < t.length(); i++) {
		if (state == 1) {
			if (t.c_str()[i] > '/' && find == 0) {
				flag += t.c_str()[i];
			}
			else {
				if (find == 0) {
					while (flg_cur != NULL) {
						if (flg_cur->code == flag) {
							stringstream ss;
							ss << flg_cur->addr;
							string s;
							ss >> s;
							buff += s;
						}
						flg_cur = flg_cur->next;
					}
					find = 1;
				}
				buff += t.c_str()[i];
			}
		}
		else {
			if (t.c_str()[i] >= 'A' && t.c_str()[i] <= 'z') {
				state = 1;
				flag += t.c_str()[i];
			}
			else {
				buff += t.c_str()[i];
			}
		}
	}
	if (state == 1 && find == 0) {
		while (flg_cur != NULL) {
			if (flg_cur->code == flag) {
				stringstream ss;
				ss << flg_cur->addr;
				string s;
				ss >> s;
				buff += s;
			}
			flg_cur = flg_cur->next;
		}
	}
	return buff;
}