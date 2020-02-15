#include "jpeg_de.h"

int main(int argc, char* argv[])
{
	int       i, j, k;
	int       p, q, r, s;

	FILE      *fp;

	int       line_cnt;
	char      huff_line[HUFF_LINE_LEN];
	int       cod;
	char      cod_len;	

	char      huff_dc_cat[HUFF_DC_TBL_LEN];
	short int huff_dc_cod[HUFF_DC_TBL_LEN];
	char      huff_dc_len[HUFF_DC_TBL_LEN];

    int       huff_ac_cod[HUFF_AC_TBL_LEN];
	char      huff_ac_len[HUFF_AC_TBL_LEN];
	short int huff_ac_run_cat[HUFF_AC_TBL_LEN];
	short int ac_run, ac_cat;

	int       img_width;
	int       img_height;
	int       gray_level;
	double    q_fact;

	char      *cod_stream_buf;

	int       cnt;
	char      b;
	int       total_byte_cnt;	
	int       cod_buf_index;
	char      bit_rd_err;
	char      n_bit_rd_err;

	short int dc_cat;
	short int dc_ext;
	short int dc_val;

	short int ac_ext;
	short int ac_val;
	short int run_cat_ascii;

	int hor_blk;
	int ver_blk;

	JPEG_CODING_DATA_TYPE tmp_cod_buf[BLOCK_SIZE*BLOCK_SIZE];
	JPEG_CODING_DATA_TYPE **cod_buf;
	PIXEL_DATA_TYPE       **blk_buf;
				
	////////////////////
	// CLI check
	////////////////////
	if(argc<4){
		printf(
			"Command line error\n"
			"jpeg_bl para input_file output_file\n"
			"Example: jpeg_bl jpeg.book lena512_en.cjpeg lena512_de.pgm\n"
			);
		return 1;
	}

	////////////////////////////////////////////////////////////
	// huff table loading from jpeg.book
	////////////////////////////////////////////////////////////
	fp = fopen(argv[1], "r");
	if(fp==NULL){
		printf("Could not open\n");
		return 1;
	}

	line_cnt = 0;	
	while(!feof(fp) && (line_cnt<HUFF_DC_TBL_LEN)){
		fgets(huff_line, HUFF_LINE_LEN, fp);
		
		if(huff_line[0] != 0x0A){
			
			i=0;
			cod = 0;
			cod_len = 0;

			while(huff_line[i] != 0x0A){				
				
				if(i==0){
					huff_dc_cat[line_cnt] = huff_line[i];					
				}

				if(i>1){
					if(huff_line[i] == '1'){
						cod = cod << 1;
						cod = cod | 1;
						cod_len++;
					}
					else{
						cod = cod << 1;
						cod_len++;
					}					
				}
				i++;
			}
						
			huff_dc_cod[line_cnt] = cod;
			huff_dc_len[line_cnt] = cod_len;
			line_cnt++;
		}		
	}
	//for(i=0; i<HUFF_DC_TBL_LEN; i++){
	//	printf("%c\t", huff_dc_cat[i]);
	//	printf("%d\t", huff_dc_cod[i]);
	//	printf("%d\n", huff_dc_len[i]);
	//}
	
	line_cnt = 0;
	while(!feof(fp) && (line_cnt<HUFF_AC_TBL_LEN)){
		fgets(huff_line, HUFF_LINE_LEN, fp);
		
		if(huff_line[0] != 0x0A){
			
			i=0;
			cod = 0;
			cod_len = 0;

			while(huff_line[i] != 0x0A){				
				
				if(i==0){
					ac_run = (int)huff_line[i];					
				}
				if(i==2){
					ac_cat = (int)huff_line[i];
					ac_run = ac_run << 8;
					ac_cat = ac_run | ac_cat;
					huff_ac_run_cat[line_cnt] = ac_cat;					
				}

				if(i>3){
					if(huff_line[i] == '1'){
						cod = cod << 1;
						cod = cod | 1;
						cod_len++;
					}
					else if(huff_line[i] == '0'){
						cod = cod << 1;
						cod_len++;
					}					
				}
				i++;
			}
			huff_ac_cod[line_cnt] = cod;
			huff_ac_len[line_cnt] = cod_len;
			line_cnt++;
		}		
	}
	//for(i=0; i<HUFF_AC_TBL_LEN; i++){
	//	printf("%x\t", huff_ac_run_cat[i]);
	//	printf("%d\t", huff_ac_cod[i]);
	//	printf("%d\n", huff_ac_len[i]);
	//}
	fclose(fp);	

	////////////////////////////////////////////////////////////
	// code words loading from .cjpeg (width, height, gray level, Q, codes...)
	////////////////////////////////////////////////////////////
	
	fp = fopen(argv[2], "rb");
	if(fp==NULL){
		printf("Could not open\n");
		return 1;
	}

	// width
	fread(&img_width, sizeof(int), 1, fp);
	
	// height
	fread(&img_height, sizeof(int), 1, fp);
	
	// gray level
	fread(&gray_level, sizeof(int), 1, fp);
	
	// q factor
	fread(&q_fact, sizeof(double), 1, fp);

	////////////////////
	// get num of blocks
	////////////////////
	hor_blk = img_width >> BLOCK_SIZE_IN_SHIFT_AMT;
	ver_blk = img_height >> BLOCK_SIZE_IN_SHIFT_AMT;
	
	cod_stream_buf = (char *) malloc((img_width*img_height)*sizeof(char));
	if(cod_stream_buf == NULL){
		printf("Could not allocate space\n");
		return 1;
	}

	i=0;
	while(!feof(fp)){
		if(fread(&cod_stream_buf[i], sizeof(char), 1, fp)==1){			
			i++;
		}
	}
	total_byte_cnt = i;
	fclose(fp);

	////////////////////
	// allocate ptrs & space for code blocks
	////////////////////
	cod_buf = (JPEG_CODING_DATA_TYPE **) malloc((ver_blk*hor_blk)*sizeof(JPEG_CODING_DATA_TYPE *));
    if(cod_buf == NULL){
		printf("Could not allocate code blocks pointers\n");
		return 1;
	}
	
	for(i=0; i<(ver_blk*hor_blk); i++){
		cod_buf[i] = (JPEG_CODING_DATA_TYPE *) malloc((BLOCK_SIZE*BLOCK_SIZE)*sizeof(JPEG_CODING_DATA_TYPE));
		if(cod_buf[i] == NULL){
			printf("Could not allocate space for a code block\n");
			return 1;
		}
	}

	////////////////////
	// allocate ptrs and spaces for block buffer
	////////////////////
	blk_buf = (PIXEL_DATA_TYPE **) malloc(BLOCK_SIZE*sizeof(PIXEL_DATA_TYPE *));
    if(blk_buf == NULL){
		printf("Could not allocate block height pointers\n");
		return 1;
	}
	for(i=0; i<BLOCK_SIZE; i++){
		blk_buf[i] = (PIXEL_DATA_TYPE *) malloc(BLOCK_SIZE*sizeof(PIXEL_DATA_TYPE));
		if(blk_buf[i] == NULL){
			printf("Could not allocate block row space\n");
			return 1;
		}
	}	
	
	// set bit counter and buf index to zero
	cnt = 0;
	cod_buf_index = 0;

	//for(j=0; j<1; j++){
	for(j=0; j<(hor_blk*ver_blk); j++){

		// init to zero
		for(k=0; k<(BLOCK_SIZE*BLOCK_SIZE); k++){
			tmp_cod_buf[k] = 0;
		}
		// load into buf
		i=0;
		while(i<(BLOCK_SIZE*BLOCK_SIZE)){
			if(i==0){
				dc_cat = decode_dc_huff(huff_dc_cat, huff_dc_cod, huff_dc_len, &cnt, &b, &total_byte_cnt, &cod_buf_index, cod_stream_buf, &bit_rd_err);
				dc_ext = get_nxt_n_bit(&dc_cat, &n_bit_rd_err, &cnt, &b, &total_byte_cnt, &cod_buf_index, cod_stream_buf, &bit_rd_err);
				dc_val = sign_extend(dc_ext, dc_cat);				
				tmp_cod_buf[i] = dc_val;				
				i++;
			}
			else{
				run_cat_ascii = decode_ac_huff(huff_ac_cod, huff_ac_len, huff_ac_run_cat, &cnt, &b, &total_byte_cnt, &cod_buf_index, cod_stream_buf, &bit_rd_err);
				get_ac_run_cat(run_cat_ascii, &ac_run, &ac_cat);

				if((ac_run==15) && (ac_cat==0)){
					ac_val = 0;					
				}
				else{
					ac_ext = get_nxt_n_bit(&ac_cat, &n_bit_rd_err, &cnt, &b, &total_byte_cnt, &cod_buf_index, cod_stream_buf, &bit_rd_err);
					ac_val = sign_extend(ac_ext, ac_cat);
				}

				i = i + ac_run;
				tmp_cod_buf[i] = ac_val;

				if((ac_run==0)&&(ac_cat==0)){
					i = (BLOCK_SIZE*BLOCK_SIZE)-1;
				}
				i++;
			}
		}
		// copy from tmp buf to cod_buf with 1-D DPCM and reorder of zig zag
		for(k=0; k<(BLOCK_SIZE*BLOCK_SIZE); k++){
			if((j==0) && (k==0)){
				cod_buf[j][seq_order[k]] = tmp_cod_buf[k];
			}
			else if((j!=0) && (k==0) && (fmod((double)j, (BLOCK_SIZE*BLOCK_SIZE))==0)){
				cod_buf[j][seq_order[k]] = tmp_cod_buf[k] + cod_buf[j-(BLOCK_SIZE*BLOCK_SIZE)][0];				
			}
			else if((fmod((double)j, (BLOCK_SIZE*BLOCK_SIZE))!=0) && (k==0)){
				cod_buf[j][seq_order[k]] = tmp_cod_buf[k] + cod_buf[j-1][0];				
			}
			else{
				cod_buf[j][seq_order[k]] = tmp_cod_buf[k];
			}
		}
	}
	
	//*****************************************************************************************
	//***************************	JPEG decoding stage		***********************************
	//*****************************************************************************************	

	// asume the decoder side given an 1-D (int) array in "**cod_buf" which stored 0->4095 of 
	// 1-D zigzag order of each 8 by 8 block of the image in sequence left to right, top to bottom
	
	PIXEL_DATA_TYPE	**decoded_pix_buf;

	// allocate ptrs for image height of the decode pix buffer
	decoded_pix_buf = (PIXEL_DATA_TYPE **) malloc(img_height*sizeof(PIXEL_DATA_TYPE *));	
    if(decoded_pix_buf == NULL){
		printf("Could not allocate height pointers\n");
		return 1;
	}
	////////////////////
	// allocate space for rows of the decode pix buffer
	for(i=0; i<img_height; i++){
		decoded_pix_buf[i] = (PIXEL_DATA_TYPE *) malloc(img_width*sizeof(PIXEL_DATA_TYPE));				
		if(decoded_pix_buf[i] == NULL){
			printf("Could not allocate row space\n");
			return 1;
		}
		
	}	// end for-loop

	////////////////////
	// copy the re-order block back into the decode pix buffer
	for(i=0; i<ver_blk; i++){				// -> for each block from left to right, top to bottom
		for(j=0; j<hor_blk; j++){
														
			//copy the re-order block back into the decode pix buffer and cast them into double 
			k = (i*BLOCK_SIZE*BLOCK_SIZE)+j;
			for(p=0; p<BLOCK_SIZE; p++){		// -> within the block of the inner 64 elements
				for(q=0; q<BLOCK_SIZE; q++){
					decoded_pix_buf[(i*BLOCK_SIZE)+p][(j*BLOCK_SIZE)+q] 
							= (double) cod_buf[k][(p*BLOCK_SIZE)+q];
				}
			}		
		}		
	}	// end for-loop

	////////////////////
	// copy into block buffers, do dequantize, rounding, inverse-DCT, shifting and copy back
	for(i=0; i<ver_blk; i++){
		for(j=0; j<hor_blk; j++){

			// Copy block by block and perform dequantize and rounding
			for(p=0; p<BLOCK_SIZE; p++){
				for(q=0; q<BLOCK_SIZE; q++){					
					blk_buf[p][q] = round( decoded_pix_buf[(i*BLOCK_SIZE)+p][(j*BLOCK_SIZE)+q]*(q_mat[p][q]*q_fact) );					
				}
			}

			// Inverse DCT
			dct_2d(1, BLOCK_SIZE, blk_buf);

			// Copy block by block back and perform shifting
			for(r=0; r<BLOCK_SIZE; r++){
				for(s=0; s<BLOCK_SIZE; s++){
					decoded_pix_buf[(i*BLOCK_SIZE)+r][(j*BLOCK_SIZE)+s] = blk_buf[r][s] + LEVEL_SHIFT_AMT;					
				}
			}						
		}
	}	// end for-loop

	////////////////////
	// creating the PGM file for the decoded image
	FILE *pFile;	
	char pixel;		
	int decode_img_width = 512;
	int decode_img_height = 512 ;
	int decode_gray_level = 255;
	
	////////////////////
	// open a new file	based on the input argument
	pFile = fopen(argv[3], "wb");
	fprintf (pFile, "P5\n");											// magic number
	fprintf (pFile, "%d %d\n", decode_img_width, decode_img_height);	// width, height		
	fprintf (pFile, "%d\n", decode_gray_level);							// grayscale


	////////////////////
	// creating the binary stream use for the each pixel value
	for(i=0; i<img_height; i++){				
		for(j=0; j<img_width; j++){
			pixel = (char) decoded_pix_buf[i][j];	
			fwrite (&pixel , sizeof(pixel) , 1 , pFile );
		}
		//printf("\n");
	}
	fclose (pFile);

	return 0;
}

////////////////////
// Round Func.
////////////////////
double round (double value)
{
  if (value < 0)
     return -(floor(-value + 0.5));     
     // return ceil ( value - 0.5);
  else
     return   floor( value + 0.5);
}
////////////////////
// Category Func.
////////////////////
int get_cat(int value){

	double temp;
	
	if(value != 0){
		value = abs(value);
		value = value + 1;		
		temp  = (double)value;
		temp  = ceil(log10(temp)/log10(2));
		return (int)temp;
	}
	else{
		return 0;
	}
}

char get_nxt_bit(int *cnt, char *b, 
				 int *total_byte_cnt, int *cod_buf_index, char *cod_stream_buf, 
				 char *bit_rd_err){
	
	char bit;

	if(*cod_buf_index <= *total_byte_cnt){
		if(*cnt == 0){
			*b   = cod_stream_buf[*cod_buf_index];
			*cnt = 8;
			*cod_buf_index = *cod_buf_index + 1;
		}
		bit  = (*b>>7) & bit_mask[1];
		*cnt = *cnt - 1;
		*b   = *b << 1;
		*bit_rd_err = 0;
		return bit;
	}
	else{
		*bit_rd_err = 1;
		return 0;
	}
}

short int get_nxt_n_bit(short int *n, char *n_bit_rd_err, 
						int *cnt, char *b, int *total_byte_cnt,
						int *cod_buf_index, char *cod_stream_buf,
						char *bit_rd_err){
	int       i;
	short int v;
	char      bit;

	i=0;
	v=0;

	while(i!=*n){
		i = i+1;

		bit = get_nxt_bit(cnt, b, total_byte_cnt, cod_buf_index, cod_stream_buf, bit_rd_err);		
		if(*bit_rd_err == 0){			
			v = (v<<1) + bit;
		}
		else{
			*n_bit_rd_err = 1;
			return v;
		}
	}
	*n_bit_rd_err = 0;
	return v;
}

short int sign_extend(short int v, short int t){
	short int vt;
	
	vt = 1 << (t-1);	
	if(v < vt){
		vt = (-1 << t) + 1;
		v  = v + vt;		
	}
	return v;
}

short int decode_dc_huff(char huff_dc_cat[], short int huff_dc_cod[], char huff_dc_len[],
					int *cnt, char *b,
					int *total_byte_cnt, int *cod_buf_index, char *cod_stream_buf, 
					char *bit_rd_err){
	int        len;
	char	   bit;
	short int  code;
	short int  i;
	char	   found;
	
	found = 0;
	len   = 1;	
	code  = get_nxt_bit(cnt, b, total_byte_cnt, cod_buf_index, cod_stream_buf, bit_rd_err);
	code  = code & bit_mask[len];
	
	if(*bit_rd_err == 0){
		while(found == 0){
			for(i=0; i<HUFF_DC_TBL_LEN; i++){
				if((code==huff_dc_cod[i])&&(huff_dc_len[i]==len)){					
					found = 1;
					break;
				}
			}
			if(found == 0){
				bit = get_nxt_bit(cnt, b, total_byte_cnt, cod_buf_index, cod_stream_buf, bit_rd_err);
				if(*bit_rd_err == 0){
					len = len + 1;
					code = (code << 1) + bit;				
				}
			}			
		}		
	}
	else{
		printf("Error in decode_dc_huff\n");
	}
	return i;
}

short int decode_ac_huff(int huff_ac_cod[], char huff_ac_len[], short int huff_ac_run_cat[],
					int *cnt, char *b,
					int *total_byte_cnt, int *cod_buf_index, char *cod_stream_buf,
					char *bit_rd_err){
	int        len;
	char	   bit;
	int        code;
	short int  i;
	char	   found;
		
	found = 0;
	len   = 1;	
	code  = get_nxt_bit(cnt, b, total_byte_cnt, cod_buf_index, cod_stream_buf, bit_rd_err);
	code  = code & bit_mask[len];
		
	if(*bit_rd_err == 0){
		while(found == 0){
			for(i=0; i<HUFF_AC_TBL_LEN; i++){
				if((code==huff_ac_cod[i])&&(huff_ac_len[i]==len)){					
					found  = 1;
					break;
				}
			}
			if(found == 0){
				bit = get_nxt_bit(cnt, b, total_byte_cnt, cod_buf_index, cod_stream_buf, bit_rd_err);
				if(*bit_rd_err == 0){
					len = len + 1;
					code = (code << 1) + bit;					
				}
			}						
		}		
	}
	else{
		printf("Error in decode_ac_huff\n");
	}
	return huff_ac_run_cat[i];
}

void get_ac_run_cat(short int run_cat_ascii, short int *ac_run, short int *ac_cat){

	char h_val;
	char l_val;

	short int i;

	h_val = run_cat_ascii >> 8;
	l_val = run_cat_ascii & bit_mask[8];

	for(i=0; i<16; i++){
		if(int_to_ascii[i]==h_val){
			*ac_run = i;
			break;
		}
	}
	for(i=0; i<16; i++){
		if(int_to_ascii[i]==l_val){
			*ac_cat = i;
			break;
		}
	}
}