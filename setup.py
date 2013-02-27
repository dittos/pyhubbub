from distutils.core import setup, Extension
import os
import sys

prefix = '/opt/netsurf'
libs = ['hubbub', 'parserutils']
if sys.platform != 'linux2':
    # glibc already includes iconv function
    libs.append('iconv')
module = Extension('_hubbub',
                   sources=['hubbubmodule.c'],
                   include_dirs=[prefix + '/include'],
                   library_dirs=[prefix + '/lib'],
                   libraries=libs)

setup(name='pyhubbub',
      version='1.0',
      description='Python binding of Hubbub HTML5 parser',
      py_modules=['pyhubbub'],
      ext_modules=[module])
