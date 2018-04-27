from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

extensions = [Extension("kconv", ["kconv_wrapper.pyx", "src/kconv.c"])]

setup(
    ext_modules = cythonize(extensions, gdb_debug=True)
)