PyHubbub
========

PyHubbub is a HTML5 parser for Python, based on [Hubbub](http://www.netsurf-browser.org/projects/hubbub/).


Installation
------------

Currently system-wide installation of libhubbub and its dependencies (libparserutils, buildsystem) is required.

```bash
git clone git://git.netsurf-browser.org/buildsystem.git
git clone git://git.netsurf-browser.org/libparserutils.git
git clone git://git.netsurf-browser.org/libhubbub.git
[sudo] make -C buildsystem install
[sudo] make -C libparserutils install
[sudo] make -C libhubbub install
git clone git://github.com/dittos/pyhubbub.git
cd pyhubbub
python setup.py install
```


Usage
-----

**pyhubbub.parse(source[, encoding])**

Arguments:

* `source`: String or stream of the document to parse. Encoding is autodetected if `enc` is not provided.
* `encoding` (optional): Source document encoding, or `None` to autodetect.

Returns: An [ElementTree element](http://docs.python.org/2/library/xml.etree.elementtree.html#element-objects) object.


Benchmark
---------

In a rough test, PyHubbub outperformed [html5lib](http://code.google.com/p/html5lib/) by ~10x speedup.

Precise benchmark will be added soon.
