from distutils.core import setup, Extension
import os

prefix = '/opt/netsurf'
module = Extension('_hubbub',
                   sources=['hubbubmodule.c'],
                   include_dirs=[prefix + '/include'],
                   library_dirs=[prefix + '/lib'],
                   libraries=['hubbub', 'parserutils', 'iconv'])

setup(name='pyhubbub',
      version='1.0',
      description='Python binding of Hubbub HTML5 parser',
      py_modules=['pyhubbub'],
      ext_modules=[module])
