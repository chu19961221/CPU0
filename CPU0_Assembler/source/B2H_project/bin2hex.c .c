#include "bin2hex.h"
#include <stdio.h>
#include <string.h>
/********************************************************************************
input:
dest: 為轉換後的結果
p->addr[0]: 高地址
p->addr[1]: 低地址
p->type: 記錄型別
p->data: 為bin格式流有效資料指標
p->len: 為bin格式流有效資料長度
output:
返回有效資料的長度
********************************************************************************/
uint16_t BinFormatEncode(uint8_t *dest, HexFormat *p)
{
uint16_t offset =0;
uint8_t check =0, num = 0;    //:(1) + 長度(2) + 地址(4) + 型別(2)
sprintf(&dest[offset],":%02X%02X%02X%02X", p->len, p->addr[0], p->addr[1], p->type);
offset +=9;                   //hex格式流資料指標偏移2
check = p->len + p->addr[0] + p->addr[1] + p->type; //計算校驗和
while(num < p->len)          //當資料長度不為0，繼續在之前的hex格式流新增資料
{                              
sprintf(&dest[offset],"%02X", p->data[num]);
check += p->data[num];      //計算校驗和
offset +=2;               //hex格式資料流資料指標偏移2
num++;                     //下一個字元
}
check = ~check +1;             //反碼+1
sprintf(&dest[offset],"%02X", check);
offset +=2;       
return offset;                  //返回hex格式資料流的長度
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
src_file = fopen(all_src_name,"rb");        //原始檔為bin檔案,以二進位制的形式開啟
if(!src_file)                      //這裡也是相當於用來檢查使用者的輸入是否準備
{
return RES_BIN_FILE_NOT_EXIST;
}
for(i=0; i<strlen(dest); i++) {
	all_des_name[9+i] = dest[i];
	all_des_name[9+i+1] = '\0';
}
dest_file = fopen(all_des_name,"w");       //目的檔案為hex檔案，以文字的形式開啟
if(!dest_file)                    
{
return RES_HEX_FILE_PATH_ERROR;
}
fseek(src_file,0, SEEK_END);      //定位到檔案末
src_file_length = ftell(src_file);
fseek(src_file,0, SEEK_SET);      //重新定位到開頭，準備開始讀取資料
src_file_quotient = (uint16_t)(src_file_length / NUMBER_OF_ONE_LINE);  //商，需要讀取多少次
src_file_remainder = (uint8_t)(src_file_length % NUMBER_OF_ONE_LINE);  //餘數，最後一次需要多少個字元
gHexFor.data = buffer_bin;         //指向需要轉換的bin資料流
while(cur_file_page < src_file_quotient)
{
fread(buffer_bin,1, NUMBER_OF_ONE_LINE, src_file);
gHexFor.len = NUMBER_OF_ONE_LINE;
if((low_addr & 0xffff0000) != hign_addr && hign_addr !=0)     //只有大於64K以後才寫入擴充套件線性地址，第一次一般是沒有
{
hign_addr = low_addr &0xffff0000;                         
gHexFor.addr[0] = (uint8_t)((hign_addr &0xff000000) >> 24);
gHexFor.addr[1] = (uint8_t)((hign_addr &0xff0000) >> 16);
gHexFor.type =4;                                      
gHexFor.len =0;                                           //記錄擴充套件地址           
tmp = BinFormatEncode(buffer_hex, &gHexFor);
fwrite(buffer_hex,1, tmp, dest_file);
fprintf(dest_file,"\n"); ;
}
gHexFor.addr[0] = (uint8_t)((low_addr &0xff00) >> 8);
gHexFor.addr[1] = (uint8_t)(low_addr &0xff);
gHexFor.type =0;                                              //資料記錄
tmp = BinFormatEncode(buffer_hex, &gHexFor);
fwrite(buffer_hex,1, tmp, dest_file);
fprintf(dest_file,"\n"); ;
cur_file_page++;
low_addr += (NUMBER_OF_ONE_LINE/4);
}
if(src_file_remainder != 0)       //最後一次讀取的個數不為0，這繼續讀取
{
fread(buffer_bin,1, src_file_remainder, src_file);
gHexFor.addr[0] = (uint8_t)((low_addr &0xff00) >> 8);
gHexFor.addr[1] = (uint8_t)(low_addr &0x00ff);
gHexFor.len = src_file_remainder;
gHexFor.type =0;                                              //資料記錄                     
tmp = BinFormatEncode(buffer_hex, &gHexFor);
fwrite(buffer_hex,1, tmp, dest_file);
fprintf(dest_file,"\n"); ;
}
gHexFor.addr[0] =0;               
gHexFor.addr[1] =0;
gHexFor.type =1;                  //結束符
gHexFor.len =0;
tmp = BinFormatEncode(buffer_hex, &gHexFor);
fwrite(buffer_hex,1, tmp, dest_file);
fprintf(dest_file,"\n"); ;
fclose(src_file);
fclose(dest_file);
return RES_OK;
}
