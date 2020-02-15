#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dct.h"

#define MAGIC_NUM_LEN                5
#define PIXEL_DATA_TYPE         double
#define BLOCK_SIZE                   8
#define BLOCK_SIZE_IN_SHIFT_AMT      3
#define LEVEL_SHIFT_AMT	           128
#define JPEG_CODING_DATA_TYPE      int
#define HUFF_LINE_LEN               32
#define HUFF_DC_TBL_LEN				12
#define HUFF_AC_TBL_LEN			   162
#define CODE_WORD_SIZE				 8

static unsigned int q_mat[BLOCK_SIZE][BLOCK_SIZE]={
	{16,11,10,16,24,40,51,61},
	{12,12,14,19,26,58,60,55},
	{14,13,16,24,40,57,69,56},
	{14,17,22,29,51,87,80,62},
	{18,22,37,56,68,109,103,77},
	{24,35,55,64,81,104,113,92},
	{49,64,78,87,103,121,120,101},
	{72,92,95,98,112,100,103,99}
};

unsigned int zz_order[BLOCK_SIZE*BLOCK_SIZE] = {
	0,1,5,6,14,15,27,28,
	2,4,7,13,16,26,29,42,
	3,8,12,17,25,30,41,43,
	9,11,18,24,31,40,44,53,
   10,19,23,32,39,45,52,54,
   20,22,33,38,46,51,55,60,
   21,34,37,47,50,56,59,61,
   35,36,48,49,57,58,62,63
};

unsigned int seq_order[BLOCK_SIZE*BLOCK_SIZE] = {
0,1,8,16,9,2,3,10,
17,24,32,25,18,11,4,5,
12,19,26,33,40,48,41,34,
27,20,13,6,7,14,21,28,
35,42,49,56,57,50,43,36,
29,22,15,23,30,37,44,51,
58,59,52,45,38,31,39,46,
53,60,61,54,47,55,62,63
};
  
char int_to_ascii[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
int  bit_mask[]     = {0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};


double round(double value);
int    get_cat(int value);

char get_nxt_bit(int *cnt, char *b, 
				 int *total_byte_cnt,
				 int *cod_buf_index, char *cod_stream_buf,
				 char *bit_rd_err);

short int get_nxt_n_bit(short int *n, char *n_bit_rd_err, 
						int *cnt, char *b, 
						int *total_byte_cnt, 
						int *cod_buf_index, char *cod_stream_buf, 
						char *bit_rd_err);

short int sign_extend(short int v, short int t);

short int decode_dc_huff(char huff_dc_cat[], short int huff_dc_cod[], char huff_dc_len[],
					int *cnt, char *b,
					int *total_byte_cnt, int *cod_buf_index, char *cod_stream_buf, 
					char *bit_rd_err);

short int decode_ac_huff(int huff_ac_cod[], char huff_ac_len[], short int huff_ac_run_cat[],
					int *cnt, char *b,
					int *total_byte_cnt, int *cod_buf_index, char *cod_stream_buf,
					char *bit_rd_err);

void get_ac_run_cat(short int run_cat_ascii, short int *ac_run, short int *ac_cat);