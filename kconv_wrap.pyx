from libc.stdlib cimport malloc, free

cdef extern from "src/kconv-main.c":
	int __kconv "main"(int argc, char *argv[])

def kconv(f, t, infile, outfile):
	def summary_encode(encode_type):
		encode_type = encode_type.lower().replace('-', '').replace('_', '')
		if encode_type == 'euckr':
			return 'k'
		elif encode_type == 'utf8':
			return 't'
		elif encode_type == 'utf16le':
			return 'u'
		elif encode_type == 'utf16be':
			return 'U'
		else:
			raise RuntimeError('Unknown encode type')

	encode_type = '-' + summary_encode(f) + summary_encode(t)

	argv = ["kconv", encode_type, infile, outfile]
	argc = len(argv)

	cdef char** c_argv = <char**>malloc(sizeof(char*) * len(argv))

	argv = [s.encode('utf8') for s in argv]

	for idx, s in enumerate(argv):
		c_argv[idx] = s

	__kconv(argc, c_argv)

	free(c_argv)
