#include "des.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


//获取子密钥
//稍后会直接在这个变量上进行变换
int initial_subkey_seq[] = {57, 49,  41, 33,  25,  17,  9,
							1, 58,  50, 42,  34,  26, 18,
							10,  2,  59, 51,  43,  35, 27,
							19, 11,   3, 60,  52,  44, 36,
							63, 55,  47, 39,  31,  23, 15,
							7, 62,  54, 46,  38,  30, 22,
							14,  6,  61, 53,  45,  37, 29,
							21, 13,   5, 28,  20,  12,  4};
				   
//多出来的0，是为了让index对准i，方便后续操作
int subkey_shift[] = {0, 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

//56 -> 48
int subkey_seq[] = {14, 17, 11, 24,  1,  5,
					3, 28, 15,  6, 21, 10,
					23, 19, 12,  4, 26,  8,
					16,  7, 27, 20, 13,  2,
					41, 52, 31, 37, 47, 55,
					30, 40, 51, 45, 33, 48,
					44, 49, 39, 56, 34, 53,
					46, 42, 50, 36, 29, 32}; 

void print_char_as_binary(char input) {
	int i;
	// input = "1";
	for (i=0; i<8; i++) {
		char shift_byte = 0x01 << (7-i);
		if (shift_byte & input) {
			printf("1");
		} else {
			printf("0");
		}
		// if((input >> (i-1)) & 0x01 == 1) {
		// 	printf("1");
		// }
		// else printf("0");
	}
	printf("\n");
}


//分别将C、D进行 左移
void shift_CD_part(unchar * half_key, unchar shift_len) {

		unchar shift_char, shift_bit, shift_content;
		//左移一位
		if(shift_len == 1){
			shift_bit = 0x80; //1000 0000
		}
		else {
			shift_bit = 0xc0;	// 1100 0000
		}

		//上面的数字目的是移动要到下一个char的位

		//获取要进行错位对应的位
		unchar shift_next1,shift_next2,shift_next3,shift_next4;
		shift_next1 = shift_bit & half_key[0];
		shift_next2 = shift_bit & half_key[1];
		shift_next3 = shift_bit & half_key[2];
		shift_next4 = shift_bit & half_key[3];

		//将每一个部分都移位
		//并且在后面加上，后一个char的头1或者2位
		half_key[0] <<= shift_len;
		half_key[0] |= (shift_next2 >> (8-shift_len));

		half_key[1] <<= shift_len;
		half_key[1] |= (shift_next3 >> (8-shift_len));

		half_key[2] <<= shift_len;
		half_key[2] |= (shift_next4 >> (8-shift_len));

		half_key[3] <<= shift_len;
		half_key[3] |= (shift_next1 >> (4-shift_len));
}

//这个函数是为了获取 第 shift_len 位置 对应的内容，其中偏移量是为了分辨C、D使用的
unchar get_bit(unchar * key,unchar shift_len, int offset) {
		unchar shift_bit, shift_char, shift_content;
		//计算出来所要移动的位，位于字节的第几位
		shift_bit = 0x80 >> ((shift_len-offset)%8);  //0x80 = 1000 0000
		//计算出来位于第几个字节
		shift_char = (shift_len-offset) / 8;

		//获取位于main_Key 里的那个位，使用 与操作 可以获得
		shift_content = shift_bit & key[shift_char];
		//为了方便将这一位移到对应的位上，需要将这个位先置到的左边第一个位置，然后后面再根据
		//i的位置，再将其进行移位
		shift_content <<= (shift_len - offset) % 8;
		return shift_content;
}

//此步位获取子密钥的第一步 
//通过将为处理的64位数据，先转化成64-8 = 56位数据
//然后再进行后续操作
//输入的main_Key是长度为8，内容是char的存储单位
//
//subKey_sets 用来存放得到的16轮子密钥，每个密钥长度都是56位，但是56位不是很好使用现有结构表示
//所以就是现实用8字节结构存储，后面再进行拆分
//
//
void cal_sub_key(unchar * main_key, key_set * subkey_sets) {
	//step1、将64位初始明文转换成 56位，用于后面计算
	
	// for(int i = 0; i < 8; ++ i) {
	// 	print_char_as_binary(main_key[i]);
	// }
	int i;
	for(i = 0; i < 8; ++ i) {
		subkey_sets[0].k[i] = 0;
	}
	
	//传递进来的是char，main_key.len() = 8，
	
	unchar shift_len,shift_content;
	for(i = 0; i < 56; ++ i) {
		shift_len = initial_subkey_seq[i];

		shift_content = get_bit(main_key, shift_len, 1);
		
		//确定这个位在，目的地的第几位
		shift_content >>= (i % 8);
		//采用或运算，将获取的bit添加到子密钥中

		subkey_sets[0].k[i/8] |= shift_content;
	}

	//检查过没问题
	// for(int i = 0; i < 8; ++ i) { 
	// 	print_char_as_binary(subkey_sets[0].k[i]);	
	// }
	//经过上述步骤，将得到的56位 7个字节存放到了subkey_sets[0].k中了
	//需要注意的是这个时候，k没有占满，后面仍有一个字节全是0

	//接下来将得到的子密钥分成C、D两部分，每个部分有28位，需要把k进行拆分
	for(i = 0; i < 3; ++ i) {
		subkey_sets[0].c[i] = subkey_sets[0].k[i];

		//d是需要进行拼接
		subkey_sets[0].d[i] = (subkey_sets[0].k[i+3] & 0x0f) << 4;
		subkey_sets[0].d[i] |= (subkey_sets[0].k[i+4] & 0xf0) >> 4;
	}
	//将k[3]中的左边一半给c
	//拿出左边一半需要进行 与操作 1111 0000 -> 0xf0
	subkey_sets[0].c[3] = subkey_sets[0].k[3] & 0xf0;

	//拿出k[6]的后一半给d
	//并且左对齐
	subkey_sets[0].d[3] = (subkey_sets[0].k[6] & 0x0f) << 4;

	//上面的没有问题

	//接下来进行16轮旋转得到16个子密钥
	//就是根据规则进行左移位
	//每一对CD都是由上一对CD得来的
	
	for( i = 1; i< 17; ++ i) {
		for(int j = 0; j < 4; ++ j) {
			subkey_sets[i].c[j] = subkey_sets[i-1].c[j];
			subkey_sets[i].d[j] = subkey_sets[i-1].d[j];
		}
		shift_len = subkey_shift[i];
		shift_CD_part(subkey_sets[i].c, shift_len);
		shift_CD_part(subkey_sets[i].d, shift_len);
		//这两个函数也没问题
		// for(int j = 0; j < 4; ++ j) {
			// print_char_as_binary(subkey_sets[i].c[j]);
		// }
		//将CD合并获取一个子密钥
		//56 位映射到 48位
		int j;
		for( j = 0; j < 48; ++ j) {
			shift_len = subkey_seq[j];
			// C
			if(shift_len <= 28) {
				shift_content = get_bit(subkey_sets[i].c, shift_len, 1);
			}
			else {
				shift_content = get_bit(subkey_sets[i].d, shift_len, 29);
			}
			//移到对应的位置
			shift_content >>= (j % 8);
			subkey_sets[i].k[j/8] |= shift_content;
		}
		// for(int j = 0; j < 8; ++ j) {
		// 	// printf("%c\n",subkey_sets[1].k[j]);
		// 	print_char_as_binary(subkey_sets[i].k[j]);
			
		// }
		// printf("\n\n\n");
		//整个函数都检查了，没有问题
	}

}


//接下来就可以将数据分成左右两部分
int initial_message_seq[] =	{58, 50, 42, 34, 26, 18, 10, 2,
							 60, 52, 44, 36, 28, 20, 12, 4,
	 						 62, 54, 46, 38, 30, 22, 14, 6,
							 64, 56, 48, 40, 32, 24, 16, 8,
							 57, 49, 41, 33, 25, 17,  9, 1,
							 59, 51, 43, 35, 27, 19, 11, 3,
							 61, 53, 45, 37, 29, 21, 13, 5,
							 63, 55, 47, 39, 31, 23, 15, 7};
//置换扩展
int E_expansion[] =  {32,  1,  2,  3,  4,  5,
					  4,  5,  6,  7,  8,  9,
					  8,  9, 10, 11, 12, 13,
					  12, 13, 14, 15, 16, 17,
					  16, 17, 18, 19, 20, 21,
					  20, 21, 22, 23, 24, 25,
					  24, 25, 26, 27, 28, 29,
					  28, 29, 30, 31, 32,  1};


//4行 16列
int SBox1[] = {	14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
			 	0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
			 	4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
				15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13};

int SBox2[] = {	15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
				 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
				 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
				13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9};

int SBox3[] = {	10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
				13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
				13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
				 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12};

int SBox4[] = { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
				13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
				10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
				 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14};

int SBox5[] = { 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
				14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
				 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
				11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3};

int SBox6[] = {	12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
				10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
				 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
				 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13};

int SBox7[] = { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
				13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
				 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
				 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12};

int SBox8[] = {	13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
				 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
				 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
				 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11};

//P 置换
int p_substitute[] = {  16,  7, 20, 21,
					    29, 12, 28, 17,
						1, 15, 23, 26,
						5, 18, 31, 10,
						2,  8, 24, 14,
						32, 27,  3,  9,
						19, 13, 30,  6,
						22, 11,  4, 25};


int IP_reverse[] =  {40,  8, 48, 16, 56, 24, 64, 32,
					 39,  7, 47, 15, 55, 23, 63, 31,
					 38,  6, 46, 14, 54, 22, 62, 30,
					 37,  5, 45, 13, 53, 21, 61, 29,
					 36,  4, 44, 12, 52, 20, 60, 28,
					 35,  3, 43, 11, 51, 19, 59, 27,
					 34,  2, 42, 10, 50, 18, 58, 26,
					 33,  1, 41,  9, 49, 17, 57, 25};
// unchar get_SB_bit(int * SBox, int index, )

void deal_message(unchar * wait_deal_part, unchar * dealed_part, key_set * key_sets, int mode) {
	unchar shift_content,shift_len;
	
	//处理后的内容
	//初始置换 IP
	unchar replaced_seq[8];
	memset(replaced_seq, 0, 8);
	memset(dealed_part, 0, 8);
	int i;
	for(i = 0; i < 64; ++ i) {
		shift_len = initial_message_seq[i];
		shift_content = get_bit(wait_deal_part, shift_len, 1);
		shift_content >>= (i%8);

		replaced_seq[i/8] |= shift_content;
	}
	// printf("replace\n");
	// for(int i = 0; i < 8; ++ i) {
	// 	print_char_as_binary(replaced_seq[i]);
	// }
	// printf("\n\n");
	unchar l[4],r[4];
	for(i = 0; i < 4; ++ i) {
		l[i] = replaced_seq[i];
		r[i] = replaced_seq[i+4];
	}

	unchar E_r[6],SB_E_r[4],ln[4],rn[4];

	int key_index; //解密和加密的值不一样
	int turn;
	for(turn = 1; turn < 17; ++ turn) {
		//交叉左右
		memcpy(ln, r, 4);

		memset(E_r, 0, 6);

		//E 扩展
		for(i = 0; i < 48; ++ i) {
			shift_len = E_expansion[i];
			shift_content = get_bit(r, shift_len, 1);

			shift_content >>= (i%8);

			E_r[i/8] |= shift_content;
		}

		// for(int i = 0; i < 6; ++ i) {
		// 	print_char_as_binary(E_r[i]);
		// }
		// printf("\n\n");

		if(mode == 1) {

			key_index = turn;
		}
		//解密是反过来的
		else key_index = 17 - turn;

		//将Er和ki通过异或运算
		for(i = 0; i < 6; ++ i) {
			E_r[i] ^= key_sets[key_index].k[i]; 
		}

		//将Er平均分成8组，每组6位
		//进行S盒变换

		unchar row,col;
		for(i = 0; i < 4; ++ i) {
			SB_E_r[i] = 0;
		}

		//第一个 6 位
		row = 0,col = 0;
		//1 和 6
		row |= ((E_r[0] & 0x80) >> 6);	//1000 0000
		row |= ((E_r[0] & 0x04) >> 2);	//0000 0100
		
		//2345
		col |= ((E_r[0] & 78) >> 3); //0111 1000

		SB_E_r[0] |= ((unchar)SBox1[row * 16 + col] << 4);//左半边

		//第二个6位
		row = 0,col = 0;
		//1 6
		row |= ((E_r[0] & 0x02));
		row |= ((E_r[1] & 0x10) >> 4);

		//2345
		col |= ((E_r[0] & 0x01) << 3);
		col |= ((E_r[1] & 0xe0) >> 5);

		SB_E_r[0] |= ((unchar)SBox2[row * 16 + col]);

		//第三个 6位 
		row = 0,col = 0;

		row |= ((E_r[1] & 0x08) >> 2);
		row |= ((E_r[2] & 0x40) >> 6);

		col |= ((E_r[1] & 0x07) << 1);
		col |= ((E_r[2] & 0x80) >> 7);

		SB_E_r[1] |= ((unchar)SBox3[row * 16 + col] << 4);

		//第四个 6位
		row = 0,col = 0;

		row |= ((E_r[2] & 0x20) >> 4);
		row |= ((E_r[2] & 0x01));

		col |= ((E_r[2] & 0x1E) >> 1);

		SB_E_r[1] |= ((unchar)SBox4[row*16 + col]);

		//第五个 6位
		row = 0, col = 0;

		row |= ((E_r[3] & 0x80) >> 6);
		row |= ((E_r[3] & 0x04) >> 2);

		col |= ((E_r[3] & 0x78) >> 3);

		SB_E_r[2] |= ((unchar)SBox5[row*16+col] << 4);


		//第六个 6位
		row = 0, col = 0;

		row |= ((E_r[3] & 0x02));
		row |= ((E_r[4] & 0x10) >> 4);

		col |= ((E_r[3] & 0x01) << 3);
		col |= ((E_r[4] & 0xE0) >> 5);

		SB_E_r[2] |= ((unchar)SBox6[row*16+col]);

		//第七个 6位
		row = 0, col = 0;

		row |= ((E_r[4] & 0x08) >> 2);
		row |= ((E_r[5] & 0x40) >> 6);

		col |= ((E_r[4] & 0x07) << 1);
		col |= ((E_r[5] & 0x80) >> 7);

		SB_E_r[3] |= ((unchar)SBox7[row*16+col] << 4);

		//第八个6位
		row = 0, col = 0;

		row |= ((E_r[5] & 0x20) >> 4);
		row |= ((E_r[5] & 0x01));

		col |= ((E_r[5] & 0x1e) >> 1);

		SB_E_r[3] |= ((unchar)SBox8[row*16+col]);


		//P 置换
		for(i = 0; i < 4; ++ i) {
			rn[i] = 0;
		}

		for(i = 0; i < 32; ++ i) {
			shift_len = p_substitute[i];
			shift_content = get_bit(SB_E_r, shift_len, 1);
			shift_content >>= (i%8);

			rn[i/8] |= shift_content;
		}


		for(i = 0; i < 4; ++ i) {
			rn[i] ^= l[i];
		}

		for(i = 0; i < 4; ++ i) {
			l[i] = ln[i];
			r[i] = rn[i];
		}

	}

	//逆置换
	unchar temp[8];
	for(i = 0; i < 4; ++ i) {
		temp[i] = r[i];
		temp[i + 4] = l[i];
	}

	for(i = 0; i < 64; ++ i) {
		shift_len = IP_reverse[i];
		shift_content = get_bit(temp, shift_len, 1);

		shift_content >>= (i % 8);

		dealed_part[i/8] |= shift_content;
	}
}

void generate_key(unchar * key) {
	int i = 0;
	for(; i < 8; ++ i) {
		key[i] = rand()%255;
	}
}




