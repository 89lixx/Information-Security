#include "des.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



FILE* key_file, *dealed_message_file, *message_file;


void help() {
	printf("You should private at least 1 parameters\n");
	// printf("Input ./des -g key.txt  , we'll provide a random key file for you\n");
	printf("Input ./des -e message_file key.txt dealed_message_file , start to encrypt message\n");
	printf("Input ./des -d message_file key.txt dealed_message_file , start to decrypt message\n");
}


int main(int argc, char * argv[]) {
	if(argc < 2) {
		help();
		return 1;
	}

	if(strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "-d") ==0 ) {
		if(argc != 5) {
			printf("Lack of parameters\n");
			return 1;
		}
		key_file = fopen(argv[3], "rb");
		if(!key_file) {
			printf("Cannot open key file\n");
			return 1;
		}
		//read key file
		unchar * key = (unchar*) malloc(8*(sizeof(char)));
		int len = fread(key, sizeof(unchar), 8, key_file);
		if(len != 8) {
			printf("Error occured when writing key file\n");
			return 1;
		}
		fclose(key_file);
		// for(int i = 0; i < 8; ++ i) {
		// 	print_char_as_binary(key[i]);
		// }

		//open message_file

		message_file = fopen(argv[2], "rb");
		if(!message_file) {
			printf("Cannot open message file\n");
			return 1;
		}

		dealed_message_file = fopen(argv[4],"wb");
		if(!dealed_message_file) {
			printf("Cannot open output file\n");
			return 1;
		}

		//生成 16 个 key_Set

		unchar *data_block = (unchar*)malloc(8*(sizeof(char)));
		unchar *processed_block = (unchar*)malloc(8*(sizeof(char)));
		int process_mode = 0;
		key_set * key_sets = (key_set*)malloc(17*sizeof(key_set));
		cal_sub_key(key, key_sets);
		// printf("key%s",key);
		if(strcmp(argv[1], "-e") == 0) {
			printf("Encrypting...\n");
			process_mode = 1;
		}
		else printf("Decrypting...\n");

		//分离输出的块数
		fseek(message_file, 0L, SEEK_END);
		int file_size = ftell(message_file);
		fseek(message_file, 0L, SEEK_SET);

		int block_number = file_size/8 + ((file_size%8)?1:0);

		//读取铭文
		int block_cnt = 0;
		int padding = 0; //缩进
		len = 0;
		while(fread(data_block, 1, 8, message_file)) {
			block_cnt ++;
			// printf("%d  ",block_cnt);
			if(block_cnt == block_number) {
				if(process_mode == 1) {
					padding = 8 - file_size%8;
					if(padding < 8) {
						memset((data_block + 8 - padding), (unchar)padding, padding);
					}
					deal_message(data_block, processed_block, key_sets, process_mode);
					// printf("Print processed message in binary\n");
					// for(int i = 0; i < 8; ++ i) {
					// 	print_char_as_binary(processed_block[i]);
					// }
					len = fwrite(processed_block, 1, 8, dealed_message_file);

					//额外补充8字节
					if(padding == 8) {
						memset(data_block, (unchar)padding, 8);
						deal_message(data_block, processed_block, key_sets, process_mode);
						len = fwrite(processed_block, 1, 8, dealed_message_file);
					}

				}
				else {
					deal_message(data_block, processed_block, key_sets, process_mode);
					padding  = processed_block[7];
					if(padding < 8) {
						len = fwrite(processed_block, 1, 8-padding, dealed_message_file);
					}
				}
			}
			else {
				deal_message(data_block, processed_block, key_sets, process_mode);
				// for(int i = 0; i < 8; ++ i) {
				// 		print_char_as_binary(processed_block[i]);
				// }
				len = fwrite(processed_block, 1, 8, dealed_message_file);
			}
			memset(data_block, 0, 8);
		}
	}

}