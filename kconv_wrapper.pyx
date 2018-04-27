from libc.stdio cimport FILE, fopen, fclose, fwrite
from libc.stdlib cimport free, malloc


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

    fpin = fopen(file_dir.encode('cp949'), 'rb')

    if fpin is NULL:
        raise FileNotFoundError('"{}" is not found'.format(file_dir))

    text = _load_text(fpin, nbytes)

    if text is NULL:
        raise MemoryError()

    return text

def scan(file_dir="input.txt", verbose=False):
    cdef int nbytes, i
    cdef UCHAR *ptext, *text

    text = _init(file_dir, &nbytes)
    ptext = _skip_BOM_code(text, &i)

    i = _detect_kcode(ptext, nbytes)
    free(ptext)

    if verbose:
        print('Hangul code of <{}> is <{}>!'.format(file_dir, _hancode(i)))

    return i

cdef int _enc2enum(str enc):
    enc = enc.lower().replace('-', '').replace('_', '')

    if enc == 'euckr':
        return EUCKR
    elif enc == 'utf8':
        return UTF8
    elif enc == 'utf8bom':
        return UTF8_BOM
    elif enc == 'utf16le':
        return UTF16_LE
    elif enc == 'utf16be':
        return UTF16_BE

def kconv(infile_dir, outfile_dir, in_enc, out_enc, verbose=False):
    cdef UCHAR *ptext, *text2
    cdef FILE *fpout
    cdef int nbytes, i
    cdef int in_code, out_code

    in_code = _enc2enum(in_enc)
    out_code = _enc2enum(out_enc)

    i = scan(infile_dir)

    if i != in_code:
        print('Input hangul code error! Change <{}> -> <{}>\n'.format(_hancode(in_code), _hancode(i)))
        in_code = i

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

    if verbose:
        print('Convert complete!')