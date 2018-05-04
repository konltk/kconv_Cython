from libc.stdio cimport FILE, fopen, fclose, fwrite
from libc.stdlib cimport free, malloc
from libc.stdio cimport printf
from libc.string cimport memset

import sys


ctypedef unsigned char UCHAR
ctypedef unsigned int USHORT

cdef enum:
    UTF8 = 116
    UTF8_BOM = 84
    UTF16_BE = 85
    UTF16_LE = 117
    EUCKR = 107


cdef extern from "src/kconv.h":
    void _synopsis "synopsis"()
    UCHAR *_load_text "load_text"(FILE *fp, int *size)
    UCHAR *_skip_BOM_code "skip_BOM_code"(UCHAR *str, int *codetype)
    char *_hancode "hancode"(char codetype);
    int _detect_kcode "detect_kcode"(UCHAR *str, int size)
    int _kconv "kconv"(UCHAR *str_in, UCHAR *str_out, int in_code, int out_code);
    void _put_BOM "put_BOM"(FILE *fpout, int out_mode);


def synopsis():
    _synopsis()

cdef UCHAR *_init(str file_dir, int *nbytes):
    cdef FILE *fpin
    cdef UCHAR *text
    cdef int i

    if file_dir is None or file_dir == '':
        raise ValueError('file_dir is None or ""')

    fpin = fopen(file_dir.encode(sys.getdefaultencoding()), 'rb')

    if fpin is NULL:
        raise FileNotFoundError('"{}" is not found'.format(file_dir))

    text = _load_text(fpin, nbytes)

    if text is NULL:
        raise MemoryError()

    return text

def scan(file_dir):
    cdef int nbytes, i
    cdef UCHAR *ptext, *text

    text = _init(file_dir, &nbytes)
    ptext = _skip_BOM_code(text, &i)

    i = _detect_kcode(ptext, nbytes)
    free(text)

    return _hancode(i)

cdef int _enc2enum(str enc):
    enc = enc.lower().replace('-', '').replace('_', '')

    if enc == 'euckr' or enc == 'cp949':
        return EUCKR
    elif enc == 'utf8':
        return UTF8
    elif enc == 'utf8bom':
        return UTF8_BOM
    elif enc == 'utf16le':
        return UTF16_LE
    elif enc == 'utf16be':
        return UTF16_BE

def convert(string, in_enc, out_enc):
    cdef int in_code, out_code
    cdef size_t i, src_len, dst_len
    cdef UCHAR *src_string, *dst_string
    cdef bytes result

    if isinstance(string, str):
        in_enc = 'CP949'
        string = string.encode(in_enc)

    in_code = _enc2enum(in_enc)
    out_code = _enc2enum(out_enc)

    src_len = len(string)
    src_string = <UCHAR*>malloc(sizeof(UCHAR) * src_len)
    dst_string = <UCHAR*>malloc(sizeof(UCHAR) * src_len * 2)
    memset(src_string, 0, sizeof(UCHAR) * src_len)
    memset(dst_string, 0, sizeof(UCHAR) * src_len * 2)

    for i in range(src_len):
        src_string[i] = string[i]
        # printf("%x%s", src_string[i], "\n" if i == src_len - 1 else " ")

    dst_len = _kconv(src_string, dst_string, in_code, out_code)
    result = b''

    for i in range(dst_len):
        result += bytes([dst_string[i]])
        # printf("%x%s", dst_string[i], "\n" if i == dst_len - 1 else " ")

    free(src_string)
    free(dst_string)

    return result

def convert_file(infile_dir, outfile_dir, in_enc, out_enc):
    cdef UCHAR *ptext, *text2
    cdef FILE *fpout
    cdef int nbytes, i
    cdef int in_code, out_code

    in_code = _enc2enum(in_enc)
    out_code = _enc2enum(out_enc)

    if outfile_dir is None or outfile_dir == '':
        raise ValueError('outfile dir is None or ""')

    fpout = fopen(outfile_dir.encode('cp949'), 'wb')

    ptext = _init(infile_dir, &nbytes)
    text2 = <UCHAR*> malloc(nbytes * 2)

    i = _kconv(ptext, text2, in_code, out_code)

    _put_BOM(fpout, out_code)
    fwrite(text2, i, 1, fpout)

    fclose(fpout)
    free(text2)
    free(ptext)