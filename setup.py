from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

ext_modules = Extension("pykconv", 
	["kconv_wrap.pyx", "src/kconv.c"], 
	language="c", 
	extra_compile_args=["-fPIC"])

setup(ext_modules=[ext_modules],
      cmdclass = {'build_ext': build_ext})
