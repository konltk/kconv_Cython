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


cdef class Kconv:
    cdef UCHAR *text, *text2, *ptext
    cdef FILE *fpin, *fpout
    cdef bytes file_dir
    cdef int size, code_type

    def __cinit__(self):
        pass

    def init(self, _file_dir='input.txt', verbose=False):
        cdef str encode = 'utf8'

        if _file_dir is None or _file_dir == '':
            raise ValueError('file_dir is None or ""')

        self.file_dir = _file_dir.encode(encode)
        self.fpin = fopen(self.file_dir, 'rb')

        if self.fpin is NULL:
            raise FileNotFoundError('"{}" is not found'.format(_file_dir))

        self.text = _load_text(self.fpin, &self.size)

        if verbose:
            print(self.text.decode(encode))
            print(self.size)

        self.ptext = _skip_BOM_code(self.text, &self.code_type)

    def synopsis(self):
        _synopsis()

    def scan(self, file_dir):
        cdef char code_type

        self.init(file_dir)

        code_type = _detect_kcode(self.ptext, self.size)
        print(_hancode(code_type))

        fclose(self.fpin)
        free(self.text)

    def kconv(self, infile_dir, outfile_dir, in_code, out_code):
        cdef int i

        outfile_dir = outfile_dir.encode('utf8')

        self.init(infile_dir)
        self.fpout = fopen(outfile_dir, 'wb')

        in_code = self._summary_encode(in_code)
        out_code = self._summary_encode(out_code)

        self.text2 = <UCHAR *> malloc(self.size * 2)
        i = _kconv(self.ptext, self.text2, in_code, out_code)

        _put_BOM(self.fpout, out_code)
        fwrite(self.text2, i, 1, self.fpout)

        fclose(self.fpin)
        fclose(self.fpout)
        free(self.text)
        free(self.text2)

    def _summary_encode(self, code):
        cdef str encode_type

        encode_type = code.lower().replace('-', '').replace('_', '')
        if encode_type == 'euckr':
            return EUCKR
        elif encode_type == 'utf8':
            return UTF8
        elif encode_type == 'utf8bom':
            return UTF8_BOM
        elif encode_type == 'utf16le':
            return UTF16_LE
        elif encode_type == 'utf16be':
            return UTF16_BE
