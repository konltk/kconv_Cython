#ifndef __KCONV_H__
#define __KCONV_H__

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX	100

typedef unsigned char UCHAR;
typedef unsigned int USHORT;
enum { UTF8='t', UTF8_BOM='T', UTF16_BE='U', UTF16_LE='u', EUCKR='k' };

int is_KSC5601(unsigned char c1, unsigned char c2);

int is_UTF16(unsigned char ch);

/*
	BOM에 의해 유니코드 인코딩 유형 결정 && skip BOM
	Return value: BOM 코드 skip
*/
UCHAR *skip_BOM_code(UCHAR *str, int *codetype);

/*
	한글 코드 자동 탐지 함수
		- KSC5601: 0xA1-0xFE
		- UNICODE: 0xAC00-0xD7A3

	인자 size = strlen(str)

	Return value:
		UTF8='t', UTF16_BE='U', UTF16_LE='u', EUCKR='k'
		0 -- undefined
*/
int detect_kcode(UCHAR *str, int size);

// 유니코드 한글/한자/부호 -> KS 완성형 변환
int bsearch_unitab(int unicode, int table[][2], int n);

UCHAR *UTF2UNI(UCHAR *utf, UCHAR *uni);

/*
	변환오류로 xAAAA로 출력된 것을 unicode로 변환할 때 필요한 함수
			if (s+4 < end && *s == 'x' && is_hexa(s+1)) {
			unicode = hexa2uint(s+1);

*/
int is_hexa(char *p);

int hexa2uint(char *hexa);

/* 정수를 16진수로 변환
	input: 0-15
	output: '0'-'F'
*/
UCHAR toxdigit(int i);

// Unicode B.E --> KS 완성형
void conv_uni2ksc(USHORT *uni, UCHAR *ksc);

/*
	EUC-KR 또는 Unicode BE/LE/UTF8 문자 입력
	Return value:
		BE/LE/utf8 -- 모두 BE로 변환
		string 끝이면 0을 return
*/
UCHAR *get_nextcode(UCHAR *input, int in_code, int *nextchar);

/*
	유니코드 -> UTF-8 인코딩
	return value: byte-length of 'utf8'
*/
int conv_uni2utf8(int unicode, char utf8[]);

UCHAR *add_BOM(UCHAR *str, int out_mode);

/*
	한글 코드 변환 함수 -- EUC-KR <-> Unicode BE/LE/UTF8
	- 입력 스트링: str_in
	- 출력 스트링: str_out
	- 입력 스트링의 한글 코드: in_code
	- 출력 스트링의 한글 코드: out_code
*/
int kconv(UCHAR *str_in, UCHAR *str_out, int in_code, int out_code);

void synopsis();

// 입력 파일 전체를 memory로 load
char *load_text(FILE *fp, int *size);

// Byte-Order-Mark 출력
void put_BOM(FILE *fpout, int out_mode);

char *hancode(char codetype);


#endif 
