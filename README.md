PyHubbub
========

PyHubbub is a HTML5 parser for Python, based on [Hubbub](http://www.netsurf-browser.org/projects/hubbub/).


Usage
-----

**pyhubbub.parse(data[, enc])**

Arguments:

* `data`: Raw bytes of the document to parse. Encoding is autodetected if `enc` is not provided.
* `enc` (optional): Source document encoding, or `None` to autodetect.

Returns: An [ElementTree element](http://docs.python.org/2/library/xml.etree.elementtree.html#element-objects) object.


Benchmark
---------

In a rough test, PyHubbub outperformed [html5lib](http://code.google.com/p/html5lib/) by ~10x speedup.

Precise benchmark will be added soon.
