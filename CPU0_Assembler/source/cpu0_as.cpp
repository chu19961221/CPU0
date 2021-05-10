#include <iostream>
#include <cstdlib>
#include <string.h>

using namespace std;

int main(int argc, char* argv[])
{
	string as0 = "Project1.exe ";
	as0 += argv[1];
	int n = as0.length();
	char f_name1[n+1];
	strcpy(f_name1, as0.c_str());
	
	string b2h = "B2H_project ";
	b2h += argv[1];
	b2h += ".bin ";
	b2h += argv[1];
	b2h += ".hex";
	n = b2h.length();
	char f_name2[n+1];
	strcpy(f_name2, b2h.c_str());
	
	char config[] = "config_project.exe";
	
	system(config);
	system(f_name1);
	system(f_name2);
	return 0;
}
