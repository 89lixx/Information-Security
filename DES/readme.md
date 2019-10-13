# 信息安全第一次作业

## 使用C语言实现DES算法



### 一、算法原理概述

整体算法分为两大部分：1、明文加密 2、子密钥生成

#### 1、子密钥生成

- 提供一个64位长度的密钥k，用于后续生成16个子密钥
- 然后通过位置换将k压缩成56位，并且将这56位平均分成C、D两部分，每部分28位
- 接着根据规则，进行16轮移位，每一对CD由上一对CD移位得来，在每一次移位时，将移位后的CD合并，并且通过位置换压缩成48位，这48位是本轮置换得到的一个子密钥成为ki

#### 2、明文加密/解密

- 每次加密的明文长度都是64位，如果不够则需要补齐到64位，将明文进行P置换，得到一个新的64位内容
- 将得到的64位，平均分成两部分，L、R，左右两部分。接着按照Feistel 轮函数 规则，对L、R、ki进行16轮操作，每轮操作如下
  - 将上一轮的R进行E扩展，扩展成48位，得到ER
  - 经过E扩展后，ER和ki进行异或操作，得到ER
  - 接着将ER分成8部分，每部分6位，用于S盒操作，设经过S盒操作得到的结果为SBER，有8个S盒，S盒操作规则如下：
    - 对于每个部分，取第一二位作为，行数row，取2345位作为列数col
    - SBER[i] |=  $SBox[row][col]$
  - 经过S盒操作后，还需要进行P置换，就是按照P表的规则进行位置换，最终得到SBER，作为本轮结果存储起来

### 二、总体结构

按照算法的原理，可以将整体结构分成两部分：求子密钥、加密/解密明文。

- 求子密钥，有一个主要函数`cal_sub_key`
- 加密/解密明文，有一个主要函数`deal_message`
- main函数，主要是对使用者进行指导，并且进行文件读写，以及处理信息

### 三、模块分解及数据结构

#### 1、子密钥获取

- 为了方便每一轮子密钥的表示，建立一个结构体，用于存储这个密钥

```c
typedef struct {
	unchar k[8];
	unchar c[4];	//表示
	unchar d[4];
} key_set;
```

​	k表示每一轮的到的子密钥，由于子密钥只有48位，所以后需要的16位都是0

​	c表示每一轮的C部分，d表示每一轮的d部分

- 通过位操作，来实现对数据的位置换，获取56位数据

  ```c
  unchar shift_len,shift_content;
  	for(int i = 0; i < 56; ++ i) {
  		shift_len = initial_subkey_seq[i];
  
  		shift_content = get_bit(main_key, shift_len, 1);
  		
  		//确定这个位在，目的地的第几位
  		shift_content >>= (i % 8);
  		//采用或运算，将获取的bit添加到子密钥中
  
  		subkey_sets[0].k[i/8] |= shift_content;
  	}
  ```

  - shift_len表示要置换的位置
  - shift_content表示对应位置下的某个位，这里有一个函数`get_bit`用于获取对应位置下的某个位

  - unchar get_bit(unchar * key,unchar shift_len, int offset)，传入的参数：key表示数据存放的结构，shift_len表示要寻找的数据的位置，offset在这里没什么作用，主要是为了后面区分不同操作来服务的

  - ```c
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
    ```

- 接下来需要将上步的56位数据分解成CD两部分，由于选择存放56位数据的是`unchar`，所以CD的分解有点绕，C包含$k[0] k[1] k[2]$ ，$k[3]前半部分$，D包含$k[3]后半部分$，$k[4]k[5]k[6]$

- ```c
  //接下来将得到的子密钥分成C、D两部分，每个部分有28位，需要把k进行拆分
  	for(int i = 0; i < 3; ++ i) {
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
  ```

- 接着就是进行16轮旋转，其实就是位置换，和第一步差不多，主要需要注意的还是C、D的左移，由于我的CD都是单位unchar的数组，所以处理起来麻烦一点，下面是做左移函数

- ```c
  //分别将C、D进行 左移
  void shift_CD_part(unchar * half_key, unchar shift_len) {
  
  		unchar shift_char, shift_bit, shift_content;
  		//左移一位
  		if(shift_len == 1){
  			shift_bit = 0x80; //1000 0000
  		}
    	//左移两位
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
  ```

  接下来就是调用上面函数，获取移动后的CD然后将其按照规则进行压缩置换

  ```c
  for(int i = 1; i< 17; ++ i) {
  		for(int j = 0; j < 4; ++ j) {
  			subkey_sets[i].c[j] = subkey_sets[i-1].c[j];
  			subkey_sets[i].d[j] = subkey_sets[i-1].d[j];
  		}
  		shift_len = subkey_shift[i];
  		shift_CD_part(subkey_sets[i].c, shift_len);
  		shift_CD_part(subkey_sets[i].d, shift_len);
  
  		//将CD合并获取一个子密钥
  		//56 位映射到 48位
  		for(int j = 0; j < 48; ++ j) {
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
  ```

#### 2、明文加密/解密

> 加密和解密过程基本上是一致的，只是加密时，16个子密钥调用是按照顺序来的，解密时16个子密钥的调用按照逆序使用

- IP置换，就是压缩位置换

  ```c
  unchar replaced_seq[8];
  	memset(replaced_seq, 0, 8);
  	memset(dealed_part, 0, 8);
  	for(int i = 0; i < 64; ++ i) {
  		shift_len = initial_message_seq[i];
  		shift_content = get_bit(wait_deal_part, shift_len, 1);
  		shift_content >>= (i%8);
  
  		replaced_seq[i/8] |= shift_content;
  	}
  ```

  经过IP置换后的到的内容存放在replaced_seq里面

- 将replaced_seq分解成L、R两部分

  ```c
  unchar l[4],r[4];
  	for(int i = 0; i < 4; ++ i) {
  		l[i] = replaced_seq[i];
  		r[i] = replaced_seq[i+4];
  	}
  
  ```

- 这里新建了4个变量

- ```c
  unchar E_r[6],SB_E_r[4],ln[4],rn[4];
  ```

  - E_r表示经过E扩展后的到的内容
  - SB_E_r表示经过S盒操作后的到的内容
  - ln、rn表示每一轮的L、R

- 接下来进行16轮操作，首先需要将上一轮的R赋给Ln

- ```c
  memcpy(ln, r, 4);
  ```

  然后进行E扩展，

  ```c
  //E 扩展
  		for(int i = 0; i < 48; ++ i) {
  			shift_len = E_expansion[i];
  			shift_content = get_bit(r, shift_len, 1);
  
  			shift_content >>= (i%8);
  
  			E_r[i/8] |= shift_content;
  		}
  ```

  接下来需要将扩展后的内容与子密钥ki进行异或操作，**区分加密、解密操作的终点在这里里**，如果是加密操作，那么我们子密钥就是$k[i]$，如果是解密那么子密钥就是$k[16-i]$

  ```c
  if(mode == 1) {
  			key_index = turn;
  		}
  		//解密是反过来的
  		else key_index = 17 - turn;
  
  		//将Er和ki通过异或运算
  		for(int i = 0; i < 6; ++ i) {
  			E_r[i] ^= key_sets[key_index].k[i]; 
  		}
  ```

  *使用了17-turn，是因为我的循环是从1开始的，这样方便操作*

- 得到了E_r后，开始进行S盒操作，每个S盒操作，都是类似的，这里给出一组操作为例

- ```c
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
  ```

- 得到SB_E_r后需要最后一步操作P置换

- ```c
  //P 置换
  		for(int i = 0; i < 4; ++ i) {
  			rn[i] = 0;
  		}
  
  		for(int i = 0; i < 32; ++ i) {
  			shift_len = p_substitute[i];
  			shift_content = get_bit(SB_E_r, shift_len, 1);
  			shift_content >>= (i%8);
  
  			rn[i/8] |= shift_content;
  		}
  ```

- 接下来就是将这一轮的L、R保存起来，用于下一轮

- ```c
  for(int i = 0; i < 4; ++ i) {
  			rn[i] ^= l[i];
  		}
  
  		for(int i = 0; i < 4; ++ i) {
  			l[i] = ln[i];
  			r[i] = rn[i];
  		}
  ```

### 四、编译运行结果

- 在命令行输入`make`，生成des可执行文件
- 这里我提供了一个密钥文件`key.txt`内容是

```
12345678
```

 并让其对数据message.txt进行加密，message.txt内容为

```
12345678
abcdefgh
```

- 输入 `./des -e message.txt key.txt result.txt`进行加密，得到的内容存放在result.txt中

  得到的内容如下

<img src="/Users/apple/Desktop/assets/屏幕快照 2019-10-13 下午3.20.36.png" style="zoom:50%;" />

- 再输入`./des -d result.txt key.txt result1.txt`进行解密，得到的内容如下

<img src="/Users/apple/Desktop/assets/屏幕快照 2019-10-13 下午3.22.03.png" style="zoom:50%;" />

得到的结果与message.txt一致，说明算法正确

