#ifndef BIN2HEX_H
#define BIN2HEX_H
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
/***********************************
*********************************************
�N�O�C��Ū�gbin�ɮ�N�Ӧ줸�աA�M��A��Ƭ�hex�榡�y�Ahex�榡�y���׭p��覡
: + ���� + �a�} + ���O + N�Ӽƾ�(N >= 0) + ����
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
RES_OK =0,                    //�ާ@����
RES_BIN_FILE_NOT_EXIST,        //�۷��bin�ɮפ��s�b�A�]�A��J�����|�i��s�b�����T
RES_HEX_FILE_PATH_ERROR        //�ؼ��ɮ׸��|�i���J���~         
} RESULT_STATUS;
RESULT_STATUS BinFile2HexFile(char*src, char *dest);
#endif
