/*
'kconv' version 1.0 -- 'kconv' library interface.
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

/*
Usage
	$ kconv -scan infile
	$ kconv -[t/k/u/U][t/k/u/U] infile outfile [errfile]
	
Convert UTF8 to EUCKR
	$ kconv -tk infile outfile

Convert EUCKR to UTF8
	$ kconv -kt infile outfile
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "kconv.h"

int main(int argc, char *argv[])
{
	FILE *fpin, *fpout, *fperr=stdout;
	int in_code, out_code;
	int i, nbytes, nlines=1;
	UCHAR *text, *text2, *ptext;

	if (argc == 1 || argc > 1 && argv[1][0] != '-') {
		synopsis(); return 0;
	}

	switch (argc) {
	case 2:
		fpin = stdin;
		fpout = stdout;
		break;
	case 3:
		fpin = fopen(argv[2], "rb");
		fpout = stdout;
		break;
	case 4:
	case 5:
		fpin = fopen(argv[2], "rb");
		fpout = fopen(argv[3], "rb");
		if (fpout) {
			fclose(fpout);
			printf("Overwrite <%s> ? ", argv[3]);
			if (getchar() != 'y') return 0;
			getchar();
		}
		fpout = fopen(argv[3], "wb");
		if (argc == 5) {
			fperr = fopen(argv[4], "rb");
			if (fperr) {
				fclose(fperr);
				printf("Overwrite <%s> ? ", argv[4]);
				if (getchar() != 'y') return 0;
			}
			fperr = fopen(argv[4], "w");
		}
		break;
	default:
		synopsis();
		return 0;
	}

	if (!fpin) {
		printf("No such file: <%s>!\n", argv[2]);
		return 0;
	}

	// 'k': KSC5601(EUCKR), 't': UTF-8, 'u': Unicode LE, 'U': Unicode BE
	in_code = argv[1][1];
	out_code= argv[1][2];

	text = load_text(fpin, &nbytes);	// n: byte-size
	if (text == NULL) return 0;

	ptext = skip_BOM_code(text, &i);	// Byte Order Mark
	if (i != 0) in_code = i;

	i = detect_kcode(ptext, nbytes);	// 한글코드 자동 탐지
	if (!strcmp(argv[1], "-scan")) {
		printf("Hangul code of <%s> is <%s>!\n", argv[2], hancode(i));
		return 0;
	}

	if (i != in_code) {
		fprintf(stderr, "Input Hangul code error! Change <%s> -> <%s>\n", hancode(in_code), hancode(i));
		in_code = i;
	}

	text2 = (UCHAR *) malloc(nbytes*2);

	i = kconv(ptext, text2, in_code, out_code);

	put_BOM(fpout, out_code);	// Byte-Order-Mark
	fwrite(text2, i, 1, fpout);

	if (fpin != stdin) fclose(fpin);
	if (fpout != stdout) fclose(fpout);
	if (argc == 5) fclose(fperr);
	
	free(text); free(text2);

	return 0;
}
