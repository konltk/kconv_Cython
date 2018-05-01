/*
'kconv' version 1.0 -- a library for Korean code conversion
	EUCKR <-> Unicode code conversion for Hangul code and automatic detection of Hangul code.
	- Input : EUC-KR, UTF8, UTF16BE, UTF16LE
	- Output: EUC-KR, UTF8, UTF16BE, UTF16LE
	
Copyright (C) 2018 Seung-Shik Kang

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program;
if not, see <http://www.gnu.org/licenses>.

2018/04/01, Seung-Shik Kang, nlpkang@gmail.com
Also, see the following sites for Korean NLP softwares.
http://konltk.org, http://konlp.com
http://cafe.daum.net/nlpk, http://cafe.naver.com/nlpk
*/
#include "kconv.h"
#include "uni2ksc.h"
#include "ksc2uni.h"


int n_uni2ksc=17188, n_ksc2uni=17188;	// 코드 변환표 크기 -- uni2ksc.h, ksc2uni.h


int is_KSC5601(unsigned char c1, unsigned char c2)
{
	return ((0xA1 <= c1 && c1 <= 0xFE) && (0xA1 <= c2 && c2 <= 0xFE));
}

int is_UTF16(unsigned char ch)
{
	return (0xAC <= ch && ch <= 0xD7);
}

/*
	BOM에 의해 유니코드 인코딩 유형 결정 && skip BOM
	Return value: BOM 코드 skip
*/
UCHAR *skip_BOM_code(UCHAR *str, int *codetype) {
	int c;

	*codetype = 0;
	c = *str;
	if (c == 0xFE && *(str+1) == 0xFF) {
		*codetype = UTF16_BE;
		return str+2;
	} else if (c == 0xFF && *(str+1) == 0xFE) {
		*codetype = UTF16_LE;
		return str+2;
	} else if (c == 0xEF && *(str+1) == 0xBB && *(str+2) == 0xBF) {
		*codetype = UTF8;
		return str+3;
	}

	return str;
}

/*
	한글 코드 자동 탐지 함수
		- KSC5601: 0xA1-0xFE
		- UNICODE: 0xAC00-0xD7A3

	인자 size = strlen(str)

	Return value:
		UTF8='t', UTF16_BE='U', UTF16_LE='u', EUCKR='k'
		0 -- undefined
*/
int detect_kcode(UCHAR *str, int size)
{
	int i, code=0;
	int nKSC, nUTF8, nBE, nLE, nASCII;	// 각 코드 개수
	int noneKSC, noneUTF8, noneBE, noneLE;	// 각 코드 개수
	unsigned char *p=str;

	p = skip_BOM_code(p, &code);	// 유니코드 인코딩 형태 자동 설정 <- BOM
	if (code) return code;

	nKSC = nUTF8 = nBE = nLE = nASCII = 0;
	noneKSC = noneUTF8 = noneBE = noneLE = 0;

	for (i=0; i < size-2;) {
		if (p[i] == 0) {	// 0x00 + ASCII문자
			if (!(p[i+1] & 0x80)) return UTF16_BE;
			return UTF16_LE;	// 0x00AC
		}

		if (p[i+1] == 0) {	// ASCII문자 + 0x00
			if (!(p[i] & 0x80)) return UTF16_LE;
			return UTF16_BE;	// 0xAC00
		}

		if (!(p[i] & 0x80)) {	// MSB == 0 (ASCII 문자)
			nASCII++;
			i++;
			continue;
		}
		
		if ((p[i] & 0xE0) == 0xE0 && (p[i+1] & 0xC0) == 0x80 && (p[i+2] & 0xC0) == 0x80) {
			// UTF8 format: 1110xxxx + 10xxxxxx + 10xxxxxx
			nUTF8++;
			nKSC--;
			i += 3;
		} else if ((p[i] & 0xE0) == 0xC0 && (p[i+1] & 0xC0) == 0x80) {
			nUTF8++;
			nKSC--;
			i += 2;
		} else {
//printf("nonUTF(%d): %x, %x\n", i, p[i], p[i+1]); getchar();
			noneUTF8++;
			i += 2;
//			break;
		}

		if (nUTF8 > MAX) return UTF8;
	}
//printf("nUTF8= %d, noneUTF8= %d, nASCII= %d\n", nUTF8, noneUTF8, nASCII);

	for (i=0; i < size-2;) {
		if ((p[i] & 0x80)) {	// MSB == 1
			if (is_KSC5601(p[i], p[i+1])) nKSC++;
			else noneKSC++;
//printf("KSC(%d): %x, %x, nKSC= %d, NoneKSC= %d\n", i, p[i], p[i+1], nKSC, noneKSC); getchar();
			i += 2;
		} else {	// MSB == 0 (ASCII 문자)
			i++;
		}

		if (nKSC > MAX || nBE > MAX) break;
	}
//printf("nKSC= %d, noneKSC= %d\n", nKSC, noneKSC);

	for (i=0; i < size-2;) {
		if ((p[i] & 0x80)) {	// BE?
			if (is_UTF16(p[i])) nBE++;
			else nBE--;
		} else {	// ASCII 문자
			if (p[i+1] == 0) nBE++;
			else noneBE = 1;
		}
		if ((p[i+1] & 0x80)) {	// LE?
			if (is_UTF16(p[i+1])) nLE++;
			else nLE--;
		} else {
			if (p[i] == 0) nLE++;
			else noneLE = 1;
		}
		i += 2;

		if (nBE > MAX || nLE > MAX) break;
	}

	if (noneBE == 0) return UTF16_BE;
	if (noneLE == 0) return UTF16_LE;
	if (noneKSC == 0) return EUCKR;
	if (noneUTF8 == 0) return UTF8;
		
	return 0;	// undefined
}

// 유니코드 한글/한자/부호 -> KS 완성형 변환
int bsearch_unitab(int unicode, int table[][2], int n)
{
	int low=0, upp=n, cur;

	while (low < upp) {
		cur = (low + upp) / 2;
		if (table[cur][0] < unicode) {
			low = cur+1;
		} else if (table[cur][0] > unicode) {
			upp = cur;
		} else return table[cur][1];
	}

	return -1;
}

UCHAR *UTF2UNI(UCHAR *utf, UCHAR *uni)
{
	UCHAR *p=utf, *q=uni;

	while (*p) {
		if (*p & 0x80) {
			if ((*p & 0xF0) == 0xE0 && (*(p+1) & 0xC0) == 0x80 && (*(p+2) & 0xC0) == 0x80) {
				*q++ = (UCHAR) (((*p << 4) & 0xF0) | ((*(p+1) >> 2) & 0x0F));
				*q++ = (UCHAR) (((*(p+1) << 6) & 0xC0) | (*(p+2) & 0x3F));
				p += 3;
			} else if ((*p & 0xE0) == 0xC0 && (*(p+1) & 0xC0) == 0x80) {
				*q++ = (UCHAR) (*p & 0x1F);
				*q++ = (UCHAR) (*(p+1) & 0x3F);
				p += 2;
			} else if ((*p & 0xF8) == 0xF0 && (*(p+1) & 0xC0) == 0x80 &&
				(*(p+2) & 0xC0) == 0x80 &&(*(p+3) & 0xC0) == 0x80) {
				*q++ = (UCHAR) (((*p << 2) & 0x1C) | ((*(p+1) >> 4) & 0x03));
				*q++ = (UCHAR) (((*(p+1) << 4) & 0xF0) | ((*(p+2) >> 2) & 0x0F));
				*q++ = (UCHAR) (((*(p+2) << 6) & 0xC0) | (*(p+3) & 0x3F));
				p += 4;
			} else {	// UTF8 Error
				*q++ = *p++;
				if (*p == 0) break;	// 2008.06.22. 강승식 추가 for random string error 
				*q++ = *p++;
			}
		} else {
			*q++ = 0;
			*q++ = *p++;
		}
	}

	*q++ = 0; *q++ = 0; *q++ = 0;

	return uni;
}

/*
	변환오류로 xAAAA로 출력된 것을 unicode로 변환할 때 필요한 함수
			if (s+4 < end && *s == 'x' && is_hexa(s+1)) {
			unicode = hexa2uint(s+1);

*/
int is_hexa(char *p)
{
	int i;

	for (i=0; i < 4; i++) {
		if (!isxdigit(p[i])) return 0;
	}
	return 1;
}

int hexa2uint(char *hexa)
{
	int i, a=0;
	int ch;

	for (i=0; i < 4; i++) {
		a <<= 4;
		ch = toupper(hexa[i]);
		if (ch <= '9')
			a += (ch - '0');
		else a += (ch - 'A') + 10;
	}
	return a;
}

/* 정수를 16진수로 변환
	input: 0-15
	output: '0'-'F'
*/
UCHAR toxdigit(int i)
{
	if (i < 10) return '0'+i;
	else if (i < 16) return 'A'+i-10;
	else return 'a'+i-10;	// 이 경우는 발생하면 안됨!
}

// Unicode B.E --> KS 완성형
void conv_uni2ksc(USHORT *uni, UCHAR *ksc)
{
	UCHAR *q=ksc;
	int i, ksccode;

	for (i=0; uni[i]; i++) {
		ksccode = bsearch_unitab(uni[i], uni2ksc, n_uni2ksc);
		if (ksccode > 0) {
			*q++ = (ksccode >> 8) & 0xFF;
			*q++ = ksccode & 0xFF;
		} else if (ksccode < 0) {	// KS완성형 변환표에 없는 유니코드 문자
			*q++ = 'x';
			*q++ = toxdigit((uni[i] >> 12) & 0xF);
			*q++ = toxdigit((uni[i] >> 8) & 0xF);
			*q++ = toxdigit((uni[i] >> 4) & 0xF);
			*q++ = toxdigit(uni[i] & 0xF);
		}
	}
	*q = 0;
}

/*
	EUC-KR 또는 Unicode BE/LE/UTF8 문자 입력
	Return value:
		BE/LE/utf8 -- 모두 BE로 변환
		string 끝이면 0을 return
*/
UCHAR *get_nextcode(UCHAR *input, int in_code, int *nextchar)
{
	int c, d, e, f;
	unsigned int ch;
	char a[7];
	int i=0, j;
	UCHAR *str = input;

	switch (in_code) {
	case EUCKR:
		if (*str == 0) return 0;
		ch = *str & 0x00FF;
		str++;
		if (ch & 0x80) {
			ch = ((ch << 8) & 0x00FF00) | (*str & 0x00FF);
			str++;
		} else if (ch == 'U') {
			c = *str++;
			if (c == '+') {
				c = *str++;
				if (isxdigit(c)) {
					a[0] = (char) c;
					for (i=1; i < 6 && *str; i++) {
						c = *str++;
						if (!isxdigit(c)) break;
						a[i] = (char) c;
					}
					a[i] = '\0';
					if (i != 6) {
						str--;
						for (j=i-1; j >= 0; j--) str--;	// ungetc(a[j], str);
						str--;	//ungetc('+', str);
						*nextchar = ch;
						return str;
					}
					ch = strtol(a, NULL, 16);
				} else {
					str -= 2;	//ungetc(c, str); ungetc('+', str);	// 역순으로...
				}
			} else str--;	//ungetc(c, str);
		}
		*nextchar = ch;
		return str;	// 유니코드 -> 완성형 프로그램에 반대방향 기능을 추가했더니 변수명이 안 맞음! -.-
	case UTF16_BE:
		if (*str == 0 && *(str+1) == 0) return 0;
		ch = ((*str & 0x00FF) << 8) | (*(str+1) & 0x00FF);
		str += 2;
		if ((ch & 0xFC00) == 0xD800) {	// SMP 영역의 4바이트 인코딩된 문자 -> 21비트로
			ch = (((ch >> 6) & 0xF) + 1) << 16 | (ch & 0x3F) << 10;
			ch |= ((*str & 0x00FF) << 8) | *(str+1) & 0x3FF;
			str += 2;
		}
		*nextchar = ch;
		return str;
	case UTF16_LE:
		if (*str == 0 && *(str+1) == 0) return 0;
		ch = (*str & 0x00FF) | ((*(str+1) & 0x00FF) << 8);
		str += 2;
		if ((ch & 0xFC00) == 0xD800) {	// SMP 영역의 4바이트 인코딩된 문자 -> 21비트로
			ch = (((ch >> 6) & 0xF) + 1) << 16 | (ch & 0x3F) << 10;
			ch |= ((*str & 0x00FF) << 8) | *(str+1) & 0x3FF;
			str += 2;
		}
		*nextchar = ch;
		return str;
	case UTF8:
	default:
		if (*str == 0) return 0;
		c = *str++;
		if ((c & 0xF0) == 0xE0) {	// 3바이트 인코딩
			d = *str++; e = *str++;
			if ((d & 0xC0) == 0x80 && (e & 0xC0) == 0x80) {
				*nextchar = ((c << 12) & 0xF000) | ((d << 6) & 0x0FC0) | e & 0x3F;
				return str;
			} else {	// UTF-8 ERROR
				str--;	//ungetc(e, fpin);
				*nextchar = (c << 8 | d);
				return str;
			}
		}
		if ((c & 0xE0) == 0xC0) {	// 2바이트 인코딩
			d = *str++;
			if ((d & 0xC0) == 0x80) {
				*nextchar = ((c << 6) & 0x07C0) | (d & 0x3F);
				return str;
			} else {
				*nextchar = (c << 8 | d);	// UTF-8 ERROR
				return str;
			}
		}
		if ((c & 0xF8) == 0xF0) {	// 4바이트 인코딩
			d = *str++; e = *str++; f = *str++;
			if ((d & 0xC0) == 0x80 && (e & 0xC0) == 0x80 && (f & 0xC0) == 0x80) {
				*nextchar = ((c << 18) & 0x1C0000) | ((d << 12) & 0x3F000) | ((e << 6) & 0x0FC0) | f & 0x3F;
				return str;
			} else {	// UTF-8 ERROR
				str -= 2;	//ungetc(f, str); ungetc(e, str);	// 역순으로...
				*nextchar = (c << 8 | d);
				return str;
			}
		}

		*nextchar = c;
		return str;
	}
}

/*
	유니코드 -> UTF-8 인코딩
	return value: byte-length of 'utf8'
*/
int conv_uni2utf8(int unicode, char utf8[])
{
	char *p=utf8;

	if (unicode <= 0x7F) {	// 0 - 7F
		*p++ = unicode & 0x7F;
		*p = 0;
		return 1;
	} else if (unicode <= 0x7FF) {	// 80 - 7FF
		*p++ = 0xC0 | ((unicode >> 6) & 0x1F);
		*p++ = 0x80 | (unicode & 0x3F);
		*p = 0;
		return 2;
	} else if (unicode <= 0xFFFF) {	// 800 - FFFF
		*p++ = 0xE0 | ((unicode >> 12) & 0x0F);
		*p++ = 0x80 | ((unicode >> 6) & 0x3F);
		*p++ = 0x80 | (unicode & 0x3F);
		*p = 0;
		return 3;
	} else {	// 10000 - 10FFFF
		*p++ = 0xF0 | ((unicode >> 18) & 0x07);
		*p++ = 0xE0 | ((unicode >> 12) & 0x3F);
		*p++ = 0x80 | ((unicode >> 6) & 0x3F);
		*p++ = 0x80 | (unicode & 0x3F);
		*p = 0;
		return 4;
	}
}

UCHAR *add_BOM(UCHAR *str, int out_mode)
{
	UCHAR *p=str;

	switch (out_mode) {
	case UTF8_BOM:	// UTF-8 BOM
		*p++ = (UCHAR)0xEF; *p++ = (UCHAR)0xBB; *p++ = (UCHAR)0xBF;
		break;
	case UTF16_BE:	// Unicode Big Endian BOM
		*p++ = (UCHAR)0xFE; *p++ = (UCHAR)0xFF;
		break;
	case UTF16_LE:	// Unicode Little Endian BOM
		*p++ = (UCHAR)0xFF; *p++ = (UCHAR)0xFE;
		break;
	}

	return p;
}

/*
	한글 코드 변환 함수 -- EUC-KR <-> Unicode BE/LE/UTF8
	- 입력 스트링: str_in
	- 출력 스트링: str_out
	- 입력 스트링의 한글 코드: in_code
	- 출력 스트링의 한글 코드: out_code
*/
int kconv(UCHAR *str_in, UCHAR *str_out, int in_code, int out_code)
{
	UCHAR *p=str_in, *q=str_out;
	int i, ch, ksc, hsur, lsur;
	char err_mode=0;
	int nlines=1, len;
	UCHAR utf8[5];
	FILE *fperr=stdout;

//	n_uni2ksc = sizeof(uni2ksc) / sizeof(uni2ksc[0]);	// uni2ksc 변환표 크기 계산
//	n_ksc2uni = sizeof(ksc2uni) / sizeof(ksc2uni[0]);	// ksc2uni 변환표 크기 계산
//	printf("Table size: %d, %d\n", n_uni2ksc, n_ksc2uni);

	// 'U': Unicode B.E., 'u': Unicode L.E., 't': UTF-8, 'k': KS완성형
	p = skip_BOM_code(str_in, &i);	// 유니코드 인코딩 형태 자동 설정 <- BOM
	if (i) in_code = i;

	//q = add_BOM(q, out_mode);	// 맨 앞에 Byte-Order-Mark 추가
	while (p = get_nextcode(p, in_code, &ch)) {
		if (in_code == EUCKR) {
			ksc = ch;
			ch = bsearch_unitab(ksc, ksc2uni, n_ksc2uni);
			if (ch < 0) ch = ksc;
		}	// 입력 문자 ch --> 유니코드 BE !!!

		switch (out_code) {
		case EUCKR:	// ks완성형 출력
			if (ch < 128) {	// 2009/08/01 수정 256->128 -- 아스키 확장영역 변환 오류 수정
				*q++ = ch;
				if (ch == '\n') nlines++;
				break;
			}
			ksc = bsearch_unitab(ch, uni2ksc, n_uni2ksc);
			if (ksc < 0) {	// 변환 실패한 경우
				// 세종 말뭉치에 사용된 "이상한 따옴표" 문자 변환
				if (ch == 0x00FF62 || ch == 0x00FF63)
					*q++ = '\'';	// 작은 따옴표
				else if (ch == 0x0F08DC || ch == 0x0F09DC)
					*q++ = '\'';	// 작은 따옴표
				else if (ch == 0x0F0ADC || ch == 0x0F0BDC)
					*q++ = '\"';	// 큰 따옴표
				else if (ch == 0x00F85E)
					*q++ = ' ';	// 가운데점? 공백으로 처리...
				else {
					*q++ = 'U'; *q++ = '+';
					*q++ = toxdigit((ch >> 20) & 0x1);
					*q++ = toxdigit((ch >> 16) & 0xF);
					*q++ = toxdigit((ch >> 12) & 0xF);
					*q++ = toxdigit((ch >> 8) & 0xF);
					*q++ = toxdigit((ch >> 4) & 0xF);
					*q++ = toxdigit(ch & 0xF);

					fprintf(fperr, "U+%c%c%c%c%c%c",	// 변환 에러 발생 코드 출력
						toxdigit((ch >> 20) & 0x1), toxdigit((ch >> 16) & 0xF),
						toxdigit((ch >> 12) & 0xF), toxdigit((ch >> 8) & 0xF),
						toxdigit((ch >> 4) & 0xF), toxdigit(ch & 0xF));
					if (!err_mode)	// 변환 에러 문자 위치정보 출력
						fprintf(fperr, " -- line %d\n", nlines);
					else fprintf(fperr, "\n");
				}
			} else {
				if (ksc & 0xFF00) {
					*q++ = (ksc >> 8) & 0xFF;
					*q++ = ksc & 0xFF;
				} else *q++ = ksc & 0xFF;
			}
			break;
		case UTF8:	// utf-8 출력
		case UTF8_BOM:	// utf-8 출력
			len = conv_uni2utf8(ch, utf8);
			strcpy(q, utf8);
			q += len;
			break;
		case UTF16_BE:	// Unicode Big Endian
			if (ch <= 0xFFFF) {
				*q++ = (ch >> 8) & 0xFF;
				*q++ = ch & 0xFF;
			} else {
				hsur = 0xD800 | ((ch-0x10000) >> 10) & 0x3F;
				lsur = 0xDC00 | (ch & 0x3FF);
				*q++ = (hsur>>8) & 0xFF;
				*q++ = hsur & 0xFF;
				*q++ = (lsur>>8) & 0xFF;
				*q++ = lsur & 0xFF;
			}
			break;
		case UTF16_LE:	// Unicode Little Endian
			if (ch <= 0xFFFF) {
				*q++ = ch & 0xFF;
				*q++ = (ch >> 8) & 0xFF;
			} else {
				hsur = 0xD800 | ((ch-0x10000) >> 10) & 0x3F;
				lsur = 0xDC00 | (ch & 0x3FF);
				*q++ = lsur & 0xFF;
				*q++ = (lsur>>8) & 0xFF;
				*q++ = hsur & 0xFF;
				*q++ = (hsur>>8) & 0xFF;
			}
			break;
		}
	}

	return q-str_out;	// byte-size of the string
}

void synopsis()
{
	printf("\n\tusage: kconv -[t/k/u/U][t/k/u/U] infile outfile [errfile]\n\n");
	printf("\t\t-scan : detect Hangul code\n");
	printf("\t\t-tk : UTF8     -> EUCKR\n");
	printf("\t\t-uk : UTF16_LE -> EUCKR\n");
	printf("\t\t-Uk : UTF16_BE -> EUCKR\n");

	printf("\t\t-kt : EUCKR    -> UTF8\n");
	printf("\t\t-ku : EUCKR    -> UTF16_LE\n");
	printf("\t\t-kU : EUCKR    -> UTF16_BE\n");

	printf("\t\t-tu : UTF8     -> UTF16_LE\n");
	printf("\t\t-tU : UTF8     -> UTF16_BE\n");
	printf("\t\t-ut : UTF16_LE -> UTF8\n");
	printf("\t\t-Ut : UTF16_BE -> UTF8\n");
	printf("\t\t-uU : UTF16_LE -> UTF16_BE\n");
	printf("\t\t-Uu : UTF16_BE -> UTF16_LE\n");
	printf("\n\t(c) 2018, Seung-Shik Kang, nlpkang@gmail.com\n");
}

// 입력 파일 전체를 memory로 load
char *load_text(FILE *fp, int *size)
{
	long n;
	char *p;

	if (fp == NULL) return NULL;

	fseek(fp, 0L, 2);
	n = ftell(fp);	/* n: byte size of file 'fp' */

	fseek(fp, 0L, 0);
	p = (char *) malloc(n+1);	/* memory allocation */
	if (p == NULL) return NULL;

	fread(p, sizeof(char), n, fp);	/* read 'fp' to 'p' */
	*(p+n) = '\0';

	*size = n;

	return p;
}

// Byte-Order-Mark 출력
void put_BOM(FILE *fpout, int out_mode)
{
	switch (out_mode) {
	case UTF8_BOM:	// UTF-8 BOM
		putc(0xEF, fpout); putc(0xBB, fpout); putc(0xBF, fpout);
		break;
	case UTF16_BE:	// Unicode Big Endian BOM
		putc(0xFE, fpout); putc(0xFF, fpout);
		break;
	case UTF16_LE:	// Unicode Little Endian BOM
		putc(0xFF, fpout); putc(0xFE, fpout);
		break;
	}
}

char *hancode(char codetype)
{
	switch (codetype) {
		case 'k': return "EUCKR";
		case 't': return "UTF8";
		case 'T': return "UTF8_BOM";
		case 'U': return "UTF16_BE";
		case 'u': return "UTF16_LE";
		default: "Unknown!";
	}
}
