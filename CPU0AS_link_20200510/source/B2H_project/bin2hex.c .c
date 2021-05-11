#include "bin2hex.h"
#include <stdio.h>
#include <string.h>
/********************************************************************************
input:
dest: ���ഫ�᪺���G
p->addr[0]: ���a�}
p->addr[1]: �C�a�}
p->type: �O�����O
p->data: ��bin�榡�y���ĸ�ƫ���
p->len: ��bin�榡�y���ĸ�ƪ���
output:
��^���ĸ�ƪ�����
********************************************************************************/
uint16_t BinFormatEncode(uint8_t *dest, HexFormat *p)
{
uint16_t offset =0;
uint8_t check =0, num = 0;    //:(1) + ����(2) + �a�}(4) + ���O(2)
sprintf(&dest[offset],":%02X%02X%02X%02X", p->len, p->addr[0], p->addr[1], p->type);
offset +=9;                   //hex�榡�y��ƫ��а���2
check = p->len + p->addr[0] + p->addr[1] + p->type; //�p�����M
while(num < p->len)          //���ƪ��פ���0�A�~��b���e��hex�榡�y�s�W���
{                              
sprintf(&dest[offset],"%02X", p->data[num]);
check += p->data[num];      //�p�����M
offset +=2;               //hex�榡��Ƭy��ƫ��а���2
num++;                     //�U�@�Ӧr��
}
check = ~check +1;             //�ϽX+1
sprintf(&dest[offset],"%02X", check);
offset +=2;       
return offset;                  //��^hex�榡��Ƭy������
}
RESULT_STATUS BinFile2HexFile(char*src, char *dest)
{
FILE *src_file, *dest_file;
uint16_t tmp;
HexFormat gHexFor;
uint32_t low_addr =0, hign_addr = 0;
uint8_t buffer_bin[NUMBER_OF_ONE_LINE], buffer_hex[MAX_BUFFER_OF_ONE_LINE];
uint32_t src_file_length;
uint16_t src_file_quotient, cur_file_page =0;
uint8_t src_file_remainder;
char all_src_name[100] = "./bin/";
char all_des_name[100] = "./output/";
int i;
for(i=0; i<strlen(src); i++) {
	all_src_name[6+i] = src[i];
	all_src_name[6+i+1] = '\0';
}
src_file = fopen(all_src_name,"rb");        //��l�ɬ�bin�ɮ�,�H�G�i���Φ��}��
if(!src_file)                      //�o�̤]�O�۷��Ψ��ˬd�ϥΪ̪���J�O�_�ǳ�
{
return RES_BIN_FILE_NOT_EXIST;
}
for(i=0; i<strlen(dest); i++) {
	all_des_name[9+i] = dest[i];
	all_des_name[9+i+1] = '\0';
}
dest_file = fopen(all_des_name,"w");       //�ت��ɮ׬�hex�ɮסA�H��r���Φ��}��
if(!dest_file)                    
{
return RES_HEX_FILE_PATH_ERROR;
}
fseek(src_file,0, SEEK_END);      //�w����ɮץ�
src_file_length = ftell(src_file);
fseek(src_file,0, SEEK_SET);      //���s�w���}�Y�A�ǳƶ}�lŪ�����
src_file_quotient = (uint16_t)(src_file_length / NUMBER_OF_ONE_LINE);  //�ӡA�ݭnŪ���h�֦�
src_file_remainder = (uint8_t)(src_file_length % NUMBER_OF_ONE_LINE);  //�l�ơA�̫�@���ݭn�h�֭Ӧr��
gHexFor.data = buffer_bin;         //���V�ݭn�ഫ��bin��Ƭy
while(cur_file_page < src_file_quotient)
{
fread(buffer_bin,1, NUMBER_OF_ONE_LINE, src_file);
gHexFor.len = NUMBER_OF_ONE_LINE;
if((low_addr & 0xffff0000) != hign_addr && hign_addr !=0)     //�u���j��64K�H��~�g�J�X�R�M��u�ʦa�}�A�Ĥ@���@��O�S��
{
hign_addr = low_addr &0xffff0000;                         
gHexFor.addr[0] = (uint8_t)((hign_addr &0xff000000) >> 24);
gHexFor.addr[1] = (uint8_t)((hign_addr &0xff0000) >> 16);
gHexFor.type =4;                                      
gHexFor.len =0;                                           //�O���X�R�M��a�}           
tmp = BinFormatEncode(buffer_hex, &gHexFor);
fwrite(buffer_hex,1, tmp, dest_file);
fprintf(dest_file,"\n"); ;
}
gHexFor.addr[0] = (uint8_t)((low_addr &0xff00) >> 8);
gHexFor.addr[1] = (uint8_t)(low_addr &0xff);
gHexFor.type =0;                                              //��ưO��
tmp = BinFormatEncode(buffer_hex, &gHexFor);
fwrite(buffer_hex,1, tmp, dest_file);
fprintf(dest_file,"\n"); ;
cur_file_page++;
low_addr += (NUMBER_OF_ONE_LINE/4);
}
if(src_file_remainder != 0)       //�̫�@��Ū�����ӼƤ���0�A�o�~��Ū��
{
fread(buffer_bin,1, src_file_remainder, src_file);
gHexFor.addr[0] = (uint8_t)((low_addr &0xff00) >> 8);
gHexFor.addr[1] = (uint8_t)(low_addr &0x00ff);
gHexFor.len = src_file_remainder;
gHexFor.type =0;                                              //��ưO��                     
tmp = BinFormatEncode(buffer_hex, &gHexFor);
fwrite(buffer_hex,1, tmp, dest_file);
fprintf(dest_file,"\n"); ;
}
gHexFor.addr[0] =0;               
gHexFor.addr[1] =0;
gHexFor.type =1;                  //������
gHexFor.len =0;
tmp = BinFormatEncode(buffer_hex, &gHexFor);
fwrite(buffer_hex,1, tmp, dest_file);
fprintf(dest_file,"\n"); ;
fclose(src_file);
fclose(dest_file);
return RES_OK;
}
