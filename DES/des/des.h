#pragma once
#define unchar unsigned char

typedef struct {
	unchar k[8];
	unchar c[4];	//表示
	unchar d[4];
} key_set;


// void initial_sub_key(unchar * main_key);
void cal_sub_key(unchar * main_key, key_set * keysets);
unchar get_bit(unchar * key,unchar shift_len, int offset);
void shift_CD_part(unchar * half_key, unchar shift_len);
void generate_key(unchar * key);
void print_char_as_binary(char input);

void deal_message(unchar * wait_deal_part, unchar * dealed_part, key_set * key_sets, int mode);