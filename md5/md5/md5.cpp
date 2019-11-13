#include <iostream>
#include <string>

using namespace std;

#define unchar unsigned char

//小端模式
#define A 0x67452301
#define B 0xefcdab89
#define C 0x98badcfe
#define D 0x10325476


//轮转操作
#define F(b, c, d) ((b & c) | ((~b) & d))
#define G(b, c, d) ((b & d) | (c & (~d)))
//					这里写错了写成了d，应该是b

#define H(b, c, d) (b ^ c ^ d)
#define I(b, c, d) (c ^ (b | (~d)))


// #define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
// #define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
// #define H(x, y, z) ((x) ^ (y) ^ (z))
// #define I(x, y, z) ((y) ^ ((x) | (~z)))
//T表
//计算方法
//T[i]= int(2^32 * abs(sin(i)))
//i 为弧度输入
//共有64个
const  unsigned int T[] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
    0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,
    0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,
    0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,
    0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,
    0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
    0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,
    0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,
    0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
	};

//s表，左移位数

const unsigned int S[] = {
	7 ,12,17,22,7 ,12,17,22,7 ,12,17,22,7 ,12,17,22,
	5 ,9 ,14,20,5 ,9 ,14,20,5 ,9 ,14,20,5 ,9 ,14,20,
	4 ,11,16,23,4 ,11,16,23,4 ,11,16,23,4 ,11,16,23,
	6 ,10,15,21,6 ,10,15,21,6 ,10,15,21,6 ,10,15,21
};

int group_num;	//总信息的分组数量


//填充原文
//OK
unsigned int * padding(string originalText) {
	//512位 = 64 char = 16 unsigned int
	//最终填充的结果必然是64的整数倍，单位是char
	//如果这个数字mod 512 小于 448那么也就意味着填充完之后不会超过512
	//如果 mod 512 大于 448，那么填充完之后肯定跳到了下一个512了
	//而mod 512 = 448，说明相差64，那么接下来就可以稍微转化一下
	//x + 64 / 512 + 1就是填充之后512的组数
	group_num = (originalText.length()+8) / 64 + 1;

	unsigned int * res = new unsigned int[group_num * 16];
	int i = 0;
	for(i = 0; i < group_num * 16; ++ i) {
		res[i] = 0;	//初始化，并且扩充时不用理会添加的0	
	}

	//将原文内容放入res，这个放入时挺讲究
	//1 unsignned int = 4 char
	//每一个char需要左移移动的依据就是他会在uint哪一部分
	int cnt = 0;
	for(i = 0; i < originalText.length(); ++ i) {
		res[cnt] |=  (originalText[i] << ((i%4)*8));
		if((i+1) % 4 == 0) cnt ++;
	}

	//尾部加个1
	res[cnt] |= 0x80 << ((i%4)*8);	//1000 0000

	//后面64位加上长度
	//string.length本来就是unsigned long 32位
	//所以只会用到32位
	res[group_num*16 - 2] = originalText.length() * 8;
	return res;

}


unsigned int aTemp;
unsigned int bTemp;
unsigned int cTemp;
unsigned int dTemp;

unsigned int CLS(unsigned int content, unsigned int offset) {
	unsigned int res = content << offset;
	//还需要处理将左边的移到右边
	res |= content >> (32 - offset);
	return res;
}

void Hmd5(unsigned int * mes) {
	unsigned int a = aTemp;
	unsigned int b = bTemp;
	unsigned int c = cTemp;
	unsigned int d = dTemp;
	unsigned f,k;
	for(int i = 0; i < 64; ++ i) {
		if(i < 16) {	//F
			f = F(b,c,d);
			k = i;
		}
		else if(i < 32) {
			f = G(b,c,d);
			k = (5*i + 1) % 16;
			//按照算法的要求实际上应该写成
			//k = (5*(i - 16) + 1) % 16
		}
		else if(i < 48) {
			f = H(b,c,d);
			k = (3 * i + 5) % 16;
		}
		else if(i < 64) {
			f = I(b,c,d);
			k = (7 * i) % 16;
		}

		//第一步对A进行迭代，得到的值假设为t
		//那么下一个位置A的值就会变成t。
		//如果顺序为
		//ABCD
		//abcd
		//
		//那么接下来就变成了
		//ABCD
		//tcda
		unsigned int t = b + CLS(a + f + mes[k] + T[i], S[i]);
		//这个T【i】可以写成
		//unsigned int T = 2^32 * abs(sin(i/PI))
		unsigned int temp = d;
		d = c;
		c = b;
		b = t;
		a = temp;
	}
	//分别加起来
	aTemp += a;
	bTemp += b;
	cTemp += c;
	dTemp += d;
}

string hexToStr(unsigned int hexNum) {
    string plate = "0123456789abcdef";
    string res = "";
    string temp = "";
    for(int i = 0; i < 8; ++i) {
        unsigned int index = hexNum>>(i*4) & 0xf;
//        res += plate[index];
        temp += plate[index];
        if((i+1) % 2 == 0) {
            reverse(temp.begin(), temp.end());
            res += temp;
            temp = "";
        }
    }
//    reverse(res.begin(), res.end());
    return res;
}

//OK
string MD5(string message) {
	aTemp=A;    //初始化
    bTemp=B;
    cTemp=C;
    dTemp=D;
	unsigned int * paddingMes = padding(message);
	//分组
	for(int i = 0; i < group_num; ++ i) {
		unsigned int mes[16];
		for(int j = 0; j < 16; ++ j){
			mes[j] = paddingMes[i * 16 + j];
		}
		Hmd5(mes);
	}
	return hexToStr(aTemp)+hexToStr(bTemp)+hexToStr(cTemp)+hexToStr(dTemp);
	//接下来需要将ABCD加起来然后变成string
}


int main() {
	string s;
	cout<<"Pleasr input string \nthat you want to jiami:\n";
	cin>>s;
	string res = MD5(s);
	cout<<"MD5 message:\n";
	cout<<res<<endl;
}


