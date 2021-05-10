#ifndef BIN2HEX_H
#define BIN2HEX_H
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
/***********************************
*********************************************
就是每次讀寫bin檔案N個位元組，然後再轉化為hex格式流，hex格式流長度計算方式
: + 長度 + 地址 + 型別 + N個數據(N >= 0) + 校驗
1 + 2    + 4    + 2    + N * 2           + 2
********************************************************************************/
#define NUMBER_OF_ONE_LINE     0x20
#define MAX_BUFFER_OF_ONE_LINE  (NUMBER_OF_ONE_LINE *2 + 11)
typedef struct {
uint8_t len;
uint8_t addr[2];
uint8_t type;
uint8_t *data;
} HexFormat;
typedef enum{
RES_OK =0,                    //操作完成
RES_BIN_FILE_NOT_EXIST,        //相當於bin檔案不存在，包括輸入的路徑可能存在不正確
RES_HEX_FILE_PATH_ERROR        //目標檔案路徑可能輸入有誤         
} RESULT_STATUS;
RESULT_STATUS BinFile2HexFile(char*src, char *dest);
#endif
