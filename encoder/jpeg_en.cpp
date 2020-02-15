#include "jpeg_en.h"

int main(int argc, char* argv[])
{

	FILE *fp;
	char line[MAGIC_NUM_LEN];

	int img_width;
	int img_height;
	int gray_level;
	
	int i, j, k;
	int p, q, r, s;
	unsigned char c;

	PIXEL_DATA_TYPE **pix_buf;
    PIXEL_DATA_TYPE **blk_buf;

	int hor_blk;
	int ver_blk;

	double q_fact;

	JPEG_CODING_DATA_TYPE **cod_buf;

	int dc_pre;
	int dc_cur;
	int dc_diff;

	int cat;
	int cat2;

	int run;
	int run_start_idx;

	char huff_line[HUFF_LINE_LEN];

	char      huff_dc_cat[HUFF_DC_TBL_LEN];
	short int huff_dc_cod[HUFF_DC_TBL_LEN];
	char      huff_dc_len[HUFF_DC_TBL_LEN];
	char      dc_coded_len;
	int		  dc_coded_word;
			
	int       huff_ac_cod[HUFF_AC_TBL_LEN];
	char      huff_ac_len[HUFF_AC_TBL_LEN];
	short int huff_ac_run_cat[HUFF_AC_TBL_LEN];
	short int ac_run, ac_cat;
	short int h_val, l_val;
	int       ac_val;
	char      ac_coded_len;
	int		  ac_coded_word;

	int       line_cnt;
	int       cod;
	char      cod_len;

	char      pre_coded_len;
	int       pre_coded_word;
	char      cur_coded_len;
	int       cur_coded_word;
	char      tmp_coded_len;
	int       tmp_coded_word;
	char	  send_byte;
	int		  t_byte_count;
	int       t_bit_count;
	double	  bpp;
		
	////////////////////
	// CLI check
	////////////////////
	if(argc<5){
		printf(
			"Command line error\n"
			"jpeg_bl para input_file output_file\n"
			"Example: jpeg_bl jpeg.book lena512.pgm lena512_en.cjpeg Qs\n"
			);
		return 1;
	}

	////////////////////
	// Q Factor
	////////////////////
	q_fact = atof(argv[4]);

	////////////////////////////////////////////////////////////
	// pixels loading from .pgm
	////////////////////////////////////////////////////////////	
	fp = fopen(argv[2], "rb");
	if(fp==NULL){
		printf("Could not open\n");
		return 1;
	}

	////////////////////
	// read P5, width, height, gray level
	////////////////////

	// P5
	fgets(line, MAGIC_NUM_LEN, fp);
	if((line[0] != 'P') && (line[1] != '5')){
		printf("PGM file magic number is not P5!\n");
	}

	// width
	fscanf(fp, "%d", &img_width);
	
	// height
	fscanf(fp, "%d", &img_height);
	
	// gray level
	fscanf(fp, "%d", &gray_level);

	////////////////////
	// allocate ptrs for image height
	////////////////////
	pix_buf = (PIXEL_DATA_TYPE **) malloc(img_height*sizeof(PIXEL_DATA_TYPE *));
    if(pix_buf == NULL){
		printf("Could not allocate height pointers\n");
		return 1;
	}

	////////////////////
	// allocate space for rows
	////////////////////
	for(i=0; i<img_height; i++){
		
		pix_buf[i] = (PIXEL_DATA_TYPE *) malloc(img_width*sizeof(PIXEL_DATA_TYPE));		
		if(pix_buf[i] == NULL){
			printf("Could not allocate row space\n");
			return 1;
		}

		for(j=0; j<img_width; j++){
			if(fread(&c, sizeof(unsigned char), 1, fp)==1){                
				pix_buf[i][j] = (double)c;
				//printf("%d ", c);
			}
		}
		//printf("\n");
	}
	fclose(fp);

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
	
	////////////////////
	// open file (.cjpeg)
	////////////////////
	fp = fopen(argv[3], "wb");
	if(fp==NULL){
		printf("Could not open\n");
		return 1;
	}
	fwrite(&img_width, sizeof(int), 1, fp);
	fwrite(&img_height, sizeof(int), 1, fp);
	fwrite(&gray_level, sizeof(int), 1, fp);
	fwrite(&q_fact, sizeof(double), 1, fp);
	
	////////////////////
	// get num of blocks
	////////////////////
	hor_blk = img_width >> BLOCK_SIZE_IN_SHIFT_AMT;
	ver_blk = img_height >> BLOCK_SIZE_IN_SHIFT_AMT;
	
	////////////////////
	// allocate ptrs for block buffer height
	////////////////////
	blk_buf = (PIXEL_DATA_TYPE **) malloc(BLOCK_SIZE*sizeof(PIXEL_DATA_TYPE *));
    if(blk_buf == NULL){
		printf("Could not allocate block height pointers\n");
		return 1;
	}

	////////////////////
	// allocate space for block buffer rows
	////////////////////
	for(i=0; i<BLOCK_SIZE; i++){
		blk_buf[i] = (PIXEL_DATA_TYPE *) malloc(BLOCK_SIZE*sizeof(PIXEL_DATA_TYPE));		
		if(blk_buf[i] == NULL){
			printf("Could not allocate block row space\n");
			return 1;
		}
	}
	
	////////////////////
	// copy blocks to buffer, do DCT, quantize, round, copy back
	////////////////////
	for(i=0; i<ver_blk; i++){
		for(j=0; j<hor_blk; j++){

			// Copy block by block
			for(p=0; p<BLOCK_SIZE; p++){
				for(q=0; q<BLOCK_SIZE; q++){
					blk_buf[p][q] = pix_buf[(i*BLOCK_SIZE)+p][(j*BLOCK_SIZE)+q] - LEVEL_SHIFT_AMT;					
				}
			}
			
			// DCT
			dct_2d(-1, BLOCK_SIZE, blk_buf);

			// Copy back with quantize and rounding
			for(r=0; r<BLOCK_SIZE; r++){
				for(s=0; s<BLOCK_SIZE; s++){
					pix_buf[(i*BLOCK_SIZE)+r][(j*BLOCK_SIZE)+s] = round(blk_buf[r][s]/(q_mat[r][s]*q_fact));
				}
			}
		}
	}	
	
	////////////////////
	// allocate ptrs for code blocks
	////////////////////
	cod_buf = (JPEG_CODING_DATA_TYPE **) malloc((ver_blk*hor_blk)*sizeof(JPEG_CODING_DATA_TYPE *));
    if(cod_buf == NULL){
		printf("Could not allocate code blocks pointers\n");
		return 1;
	}

	////////////////////
	// allocate space for code blocks, cast into int, and reorder (zigzag)
	////////////////////
	for(i=0; i<ver_blk; i++){
		for(j=0; j<hor_blk; j++){
			//allocate space for a code block
			k = (i*BLOCK_SIZE*BLOCK_SIZE)+j;
			cod_buf[k] = (JPEG_CODING_DATA_TYPE *) malloc((BLOCK_SIZE*BLOCK_SIZE)*sizeof(JPEG_CODING_DATA_TYPE));
			if(cod_buf[i] == NULL){
				printf("Could not allocate space for a code block\n");
				return 1;
			}
			
			for(p=0; p<BLOCK_SIZE; p++){
				for(q=0; q<BLOCK_SIZE; q++){
					cod_buf[k][zz_order[(p*BLOCK_SIZE)+q]] = (int)pix_buf[(i*BLOCK_SIZE)+p][(j*BLOCK_SIZE)+q];					
				}
			}			
		}
	}	

	free(pix_buf);
	free(blk_buf);

	////////////////////
	// JPEG coding stage
	////////////////////
	pre_coded_len  = 0;
	pre_coded_word = 0;
	t_byte_count   = 0;

	for(i=0; i<(ver_blk*hor_blk); i++){	

		run = 0;
		run_start_idx = 0;

		for(j=0; j<(BLOCK_SIZE*BLOCK_SIZE); j++){
			//printf("%d\t", cod_buf[i][j]);
			
			////////////////////
			// DC
			////////////////////			
			if(j==0){
				if(i==0){					
					dc_cur  = cod_buf[0][0];
					dc_diff = dc_cur;					
				}
				else if(fmod((double)i, (BLOCK_SIZE*BLOCK_SIZE))==0){					
					dc_cur  = cod_buf[i][j];
					dc_diff = dc_cur - cod_buf[i-(BLOCK_SIZE*BLOCK_SIZE)][j];					
				}
				else if(fmod((double)i, (BLOCK_SIZE*BLOCK_SIZE))!=0){			
					dc_cur  = cod_buf[i][j];
					dc_diff = dc_cur - dc_pre;
				}

				cat = get_cat(dc_diff);

				// if -ve, minus 1
				if(dc_diff < 0){
					dc_diff = dc_diff - 1;
				}

				// leave useful bits only
				dc_diff = dc_diff & bit_mask[cat];

				// get full length, base code and shift by cat, append dc diff
				dc_coded_len  = huff_dc_len[cat] + cat;
				dc_coded_word = huff_dc_cod[cat] << cat;
				dc_coded_word = dc_coded_word | dc_diff;

				cur_coded_len  = dc_coded_len;
				cur_coded_word = dc_coded_word;

				if((pre_coded_len+cur_coded_len)>=CODE_WORD_SIZE){

					while((pre_coded_len+cur_coded_len)>=CODE_WORD_SIZE){

						// 0<pre<8
						if(pre_coded_len > 0){
							pre_coded_word = pre_coded_word << (CODE_WORD_SIZE-pre_coded_len);

							// cur >= (8 - pre)
							if(cur_coded_len >= (CODE_WORD_SIZE-pre_coded_len)){
								tmp_coded_word = cur_coded_word;
								tmp_coded_len  = CODE_WORD_SIZE-pre_coded_len;
								tmp_coded_word = tmp_coded_word >> (cur_coded_len - tmp_coded_len);
								
								send_byte      = (char)(pre_coded_word | tmp_coded_word);
								t_byte_count   = t_byte_count + 1;
								fwrite(&send_byte, sizeof(char), 1, fp);
								//printf("Sent Byte: %x\n", send_byte);

								cur_coded_len = cur_coded_len - tmp_coded_len;
								cur_coded_word = cur_coded_word & bit_mask[cur_coded_len];

								pre_coded_len  = 0;
								pre_coded_word = 0;
							}		
						}

						// no more pre
						else if(cur_coded_len >= CODE_WORD_SIZE){

							tmp_coded_word = cur_coded_word;
							tmp_coded_len  = cur_coded_len;
							tmp_coded_word = tmp_coded_word >> (tmp_coded_len - CODE_WORD_SIZE);
							send_byte      = (char)tmp_coded_word;
							t_byte_count   = t_byte_count + 1;
							fwrite(&send_byte, sizeof(char), 1, fp);
							//printf("Sent Byte: %x\n", send_byte);

							cur_coded_word = cur_coded_word & bit_mask[cur_coded_len - CODE_WORD_SIZE];
							cur_coded_len  = cur_coded_len - CODE_WORD_SIZE;	
						}
					}			
					
					// update left over
					if(cur_coded_len > 0){
						pre_coded_len  = cur_coded_len;
						pre_coded_word = cur_coded_word;						
					}					
				}
				
				// not enough for a byte
				else{
					// (pre>0) && (cur>0)
					if((pre_coded_len>0) && (cur_coded_len>0)){
						pre_coded_word = pre_coded_word << (cur_coded_len);
						pre_coded_word = pre_coded_word | cur_coded_word;						
						pre_coded_len  = pre_coded_len + cur_coded_len;
					}
					else if((pre_coded_len==0) && (cur_coded_len>0)){
						pre_coded_len  = cur_coded_len;
						pre_coded_word = cur_coded_word;						
					}
				}
				dc_pre = dc_cur;
			}

			////////////////////
			// AC
			////////////////////
			if(j>0){
				// non-zero
				if(cod_buf[i][j]!=0){
					// some zeros in fornt case
					if(run!=0){
						//printf("%d runs %d\n", run, cod_buf[i][j]);

						// no more than 15 zeros in front
						if(run<=15){
							
							cat2  = get_cat(cod_buf[i][j]);							
							h_val = run;

							h_val = int_to_ascii[h_val];
							h_val = h_val << 8;
							l_val = int_to_ascii[cat2];
							l_val = h_val | l_val;

							for(p=0; p<HUFF_AC_TBL_LEN; p++){
								if(huff_ac_run_cat[p] == l_val){
									break;
								}
							}
						}

						// more than 15 zeros in front
						else{							
							while(run >= 16){
								
								cat2  = get_cat(0);								
								h_val = 15;

								h_val = int_to_ascii[h_val];
								h_val = h_val << 8;
								l_val = int_to_ascii[cat2];
								l_val = h_val | l_val;

								for(p=0; p<HUFF_AC_TBL_LEN; p++){
									if(huff_ac_run_cat[p] == l_val){
										break;
									}
								}
						
								// get full length, base code
								ac_coded_len  = huff_ac_len[p];
								ac_coded_word = huff_ac_cod[p];						
								cur_coded_len  = ac_coded_len;
								cur_coded_word = ac_coded_word;								

								if((pre_coded_len+cur_coded_len)>=CODE_WORD_SIZE){
						
									while((pre_coded_len+cur_coded_len)>=CODE_WORD_SIZE){
				
										// 0<pre<8
										if(pre_coded_len > 0){
											pre_coded_word = pre_coded_word << (CODE_WORD_SIZE-pre_coded_len);
							
											// cur >= (8 - pre)
											if(cur_coded_len >= (CODE_WORD_SIZE-pre_coded_len)){
												tmp_coded_word = cur_coded_word;
												tmp_coded_len  = CODE_WORD_SIZE-pre_coded_len;
												tmp_coded_word = tmp_coded_word >> (cur_coded_len - tmp_coded_len);
								
												send_byte      = (char)(pre_coded_word | tmp_coded_word);
												t_byte_count   = t_byte_count + 1;
												fwrite(&send_byte, sizeof(char), 1, fp);
												//printf("Sent Byte: %x\n", send_byte);

												cur_coded_len = cur_coded_len - tmp_coded_len;
												cur_coded_word = cur_coded_word & bit_mask[cur_coded_len];
								
												pre_coded_len  = 0;
												pre_coded_word = 0;
											}
										}
						
										// no more pre
										else if(cur_coded_len >= CODE_WORD_SIZE){

											tmp_coded_word = cur_coded_word;
											tmp_coded_len  = cur_coded_len;
											tmp_coded_word = tmp_coded_word >> (tmp_coded_len - CODE_WORD_SIZE);
											send_byte      = (char)tmp_coded_word;
											t_byte_count   = t_byte_count + 1;
											fwrite(&send_byte, sizeof(char), 1, fp);
											//printf("Sent Byte: %x\n", send_byte);

											cur_coded_word = cur_coded_word & bit_mask[cur_coded_len - CODE_WORD_SIZE];
											cur_coded_len  = cur_coded_len - CODE_WORD_SIZE;							
										}
									}
					
									// update left over
									if(cur_coded_len > 0){
										pre_coded_len  = cur_coded_len;
										pre_coded_word = cur_coded_word;							
									}

								}
				
								// not enough for a byte
								else{
									// (pre>0) && (cur>0)
									if((pre_coded_len>0) && (cur_coded_len>0)){
										pre_coded_word = pre_coded_word << (cur_coded_len);
										pre_coded_word = pre_coded_word | cur_coded_word;						
										pre_coded_len  = pre_coded_len + cur_coded_len;
									}
									else if((pre_coded_len==0) && (cur_coded_len>0)){
										pre_coded_len  = cur_coded_len;
										pre_coded_word = cur_coded_word;							
									}
								}						
								run = run - 16;								
							}
							cat2  = get_cat(cod_buf[i][j]);							
							h_val = run;

							h_val = int_to_ascii[h_val];
							h_val = h_val << 8;
							l_val = int_to_ascii[cat2];
							l_val = h_val | l_val;

							for(p=0; p<HUFF_AC_TBL_LEN; p++){
								if(huff_ac_run_cat[p] == l_val){
									break;
								}
							}							
						}
						run = 0;
						run_start_idx = 0;
					}
					// no zero infront case
					else{
						
						cat2  = get_cat(cod_buf[i][j]);							
						h_val = run;
							
						h_val = int_to_ascii[h_val];
						h_val = h_val << 8;
						l_val = int_to_ascii[cat2];
						l_val = h_val | l_val;
														
						for(p=0; p<HUFF_AC_TBL_LEN; p++){
							if(huff_ac_run_cat[p] == l_val){
								break;
							}
						}						
					}
					ac_val = cod_buf[i][j];
							
					// if -ve, minus 1
					if(ac_val < 0){
						ac_val = ac_val - 1;
					}

					// leave useful bits only
					ac_val = ac_val & bit_mask[cat2];

					// get full length, base code and shift by cat, append dc diff
					ac_coded_len  = huff_ac_len[p] + cat2;
					ac_coded_word = huff_ac_cod[p] << cat2;
					ac_coded_word = ac_coded_word | ac_val;
					cur_coded_len  = ac_coded_len;
					cur_coded_word = ac_coded_word;
					
					if((pre_coded_len+cur_coded_len)>=CODE_WORD_SIZE){
					
						while((pre_coded_len+cur_coded_len)>=CODE_WORD_SIZE){
				
							// 0<pre<8
							if(pre_coded_len > 0){
								pre_coded_word = pre_coded_word << (CODE_WORD_SIZE-pre_coded_len);
							
								// cur >= (8 - pre)
								if(cur_coded_len >= (CODE_WORD_SIZE-pre_coded_len)){
									tmp_coded_word = cur_coded_word;
									tmp_coded_len  = CODE_WORD_SIZE-pre_coded_len;
									tmp_coded_word = tmp_coded_word >> (cur_coded_len - tmp_coded_len);
								
									send_byte      = (char)(pre_coded_word | tmp_coded_word);
									t_byte_count   = t_byte_count + 1;
									fwrite(&send_byte, sizeof(char), 1, fp);
									//printf("Sent Byte: %x\n", send_byte);

									cur_coded_len = cur_coded_len - tmp_coded_len;
									cur_coded_word = cur_coded_word & bit_mask[cur_coded_len];
								
									pre_coded_len  = 0;
									pre_coded_word = 0;
								}							
							}
						
							// no more pre
							else if(cur_coded_len >= CODE_WORD_SIZE){

								tmp_coded_word = cur_coded_word;
								tmp_coded_len  = cur_coded_len;
								tmp_coded_word = tmp_coded_word >> (tmp_coded_len - CODE_WORD_SIZE);
								send_byte      = (char)tmp_coded_word;
								t_byte_count   = t_byte_count + 1;
								fwrite(&send_byte, sizeof(char), 1, fp);
								//printf("Sent Byte: %x\n", send_byte);

								cur_coded_word = cur_coded_word & bit_mask[cur_coded_len - CODE_WORD_SIZE];
								cur_coded_len  = cur_coded_len - CODE_WORD_SIZE;							
							}
						}
					
						// update left over
						if(cur_coded_len > 0){
							pre_coded_len  = cur_coded_len;
							pre_coded_word = cur_coded_word;							
						}
					}
				
					// not enough for a byte
					else{
						// (pre>0) && (cur>0)
						if((pre_coded_len>0) && (cur_coded_len>0)){
							pre_coded_word = pre_coded_word << (cur_coded_len);
							pre_coded_word = pre_coded_word | cur_coded_word;						
							pre_coded_len  = pre_coded_len + cur_coded_len;
						}
						else if((pre_coded_len==0) && (cur_coded_len>0)){
							pre_coded_len  = cur_coded_len;
							pre_coded_word = cur_coded_word;							
						}
					}
				}
				// first zero case
				else if(run==0){
					run_start_idx = j;
					run = run+1;
				}
				// second, etc zeros
				else{
					run = run+1;					
				}
			}				
		}
		
		// end of block
		// get EOB length, code
		ac_coded_len   = huff_ac_len[0];
		ac_coded_word  = huff_ac_cod[0];					
		cur_coded_len  = ac_coded_len;
		cur_coded_word = ac_coded_word;
		
		if((pre_coded_len+cur_coded_len)>=CODE_WORD_SIZE){					
			while((pre_coded_len+cur_coded_len)>=CODE_WORD_SIZE){
				
				// 0<pre<8
				if(pre_coded_len > 0){
					pre_coded_word = pre_coded_word << (CODE_WORD_SIZE-pre_coded_len);							
					// cur >= (8 - pre)
					if(cur_coded_len >= (CODE_WORD_SIZE-pre_coded_len)){
						tmp_coded_word = cur_coded_word;
						tmp_coded_len  = CODE_WORD_SIZE-pre_coded_len;
						tmp_coded_word = tmp_coded_word >> (cur_coded_len - tmp_coded_len);
								
						send_byte      = (char)(pre_coded_word | tmp_coded_word);
						t_byte_count   = t_byte_count + 1;
						fwrite(&send_byte, sizeof(char), 1, fp);
						//printf("Sent Byte: %x\n", send_byte);

						cur_coded_len = cur_coded_len - tmp_coded_len;
						cur_coded_word = cur_coded_word & bit_mask[cur_coded_len];
								
						pre_coded_len  = 0;
						pre_coded_word = 0;
					}
				}
	
				// no more pre
				else if(cur_coded_len >= CODE_WORD_SIZE){
					tmp_coded_word = cur_coded_word;
					tmp_coded_len  = cur_coded_len;
					tmp_coded_word = tmp_coded_word >> (tmp_coded_len - CODE_WORD_SIZE);
					send_byte      = (char)tmp_coded_word;
					t_byte_count   = t_byte_count + 1;
					fwrite(&send_byte, sizeof(char), 1, fp);
					//printf("Sent Byte: %x\n", send_byte);

					cur_coded_word = cur_coded_word & bit_mask[cur_coded_len - CODE_WORD_SIZE];
					cur_coded_len  = cur_coded_len - CODE_WORD_SIZE;							
				}
			}
					
			// update left over
			if(cur_coded_len > 0){
				pre_coded_len  = cur_coded_len;
				pre_coded_word = cur_coded_word;				
			}
		}				
		// not enough for a byte
		else{
			// (pre>0) && (cur>0)
			if((pre_coded_len>0) && (cur_coded_len>0)){
				pre_coded_word = pre_coded_word << (cur_coded_len);
				pre_coded_word = pre_coded_word | cur_coded_word;						
				pre_coded_len  = pre_coded_len + cur_coded_len;
			}
			else if((pre_coded_len==0) && (cur_coded_len>0)){
				pre_coded_len  = cur_coded_len;
				pre_coded_word = cur_coded_word;				
			}
		}		
	}

	// cal total bits
	t_bit_count = (t_byte_count*CODE_WORD_SIZE)+pre_coded_len;
	bpp = (double)t_bit_count/(double)(img_width*img_height);
	
	// make a last byte
	// 0<pre<8
	if(pre_coded_len > 0){
		pre_coded_word = pre_coded_word << (CODE_WORD_SIZE-pre_coded_len);
		
		tmp_coded_word = 0x00000000;
		tmp_coded_len  = CODE_WORD_SIZE-pre_coded_len;

		send_byte      = (char)(pre_coded_word | tmp_coded_word);
		t_byte_count   = t_byte_count + 1;
		fwrite(&send_byte, sizeof(char), 1, fp);
		//printf("Sent Byte: %x\n", send_byte);		
	}
	printf("Total Byte Count: %d\n", t_byte_count);
	printf("Total Bit  Count: %d\n", t_bit_count);	
	printf("BPP Rate        : %f\n", bpp);
	fclose(fp);	

	return 0;
}

////////////////////
// Round Func.
////////////////////
double round (double value)
{
  if (value < 0)
     return -(floor(-value + 0.5));     
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