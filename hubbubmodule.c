#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <hubbub/parser.h>
#include <hubbub/tree.h>

static hubbub_error create_comment(void *ctx, const hubbub_string *data, 
		void **result);
static hubbub_error create_doctype(void *ctx, const hubbub_doctype *doctype,
		void **result);
static hubbub_error create_element(void *ctx, const hubbub_tag *tag, 
		void **result);
static hubbub_error create_text(void *ctx, const hubbub_string *data, 
		void **result);
static hubbub_error ref_node(void *ctx, void *node);
static hubbub_error unref_node(void *ctx, void *node);
static hubbub_error append_child(void *ctx, void *parent, void *child, 
		void **result);
static hubbub_error insert_before(void *ctx, void *parent, void *child, 
		void *ref_child, void **result);
static hubbub_error remove_child(void *ctx, void *parent, void *child, 
		void **result);
static hubbub_error clone_node(void *ctx, void *node, bool deep, void **result);
static hubbub_error reparent_children(void *ctx, void *node, void *new_parent);
static hubbub_error get_parent(void *ctx, void *node, bool element_only, 
		void **result);
static hubbub_error has_children(void *ctx, void *node, bool *result);
static hubbub_error form_associate(void *ctx, void *form, void *node);
static hubbub_error add_attributes(void *ctx, void *node,
		const hubbub_attribute *attributes, uint32_t n_attributes);
static hubbub_error set_quirks_mode(void *ctx, hubbub_quirks_mode mode);
static hubbub_error change_encoding(void *ctx, const char *charset);

/* Prototype tree handler struct */
static hubbub_tree_handler tree_handler = {
	create_comment,
	create_doctype,
	create_element,
	create_text,
	ref_node,
	unref_node,
	append_child,
	insert_before,
	remove_child,
	clone_node,
	reparent_children,
	get_parent,
	has_children,
	form_associate,
	add_attributes,
	set_quirks_mode,
	change_encoding,
	NULL
};

typedef struct {
    PyObject_HEAD
    hubbub_parser *the_parser;
    PyObject *tree_handler;
    PyObject *document_node;
} pyhubbub_ParserObject;

static void pyhubbub_parser_dealloc(pyhubbub_ParserObject *self);
static PyObject *pyhubbub_parser_parse_chunk(pyhubbub_ParserObject *self, PyObject *args);
static PyObject *pyhubbub_parser_completed(pyhubbub_ParserObject *self);

static PyMethodDef parser_methods[] = {
    {"parse_chunk", (PyCFunction)pyhubbub_parser_parse_chunk, METH_VARARGS, "Feed a chunk of string to the parser."},
    {"completed", (PyCFunction)pyhubbub_parser_completed, METH_NOARGS, "Complete the input."},
    {NULL}  /* Sentinel */
};

static PyTypeObject pyhubbub_ParserType = {
        PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyhubbub.Parser",             /*tp_name*/
    sizeof(pyhubbub_ParserObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)pyhubbub_parser_dealloc,   /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "hubbub_parser",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    parser_methods,             /* tp_methods */
};

static void *
allocator(void *ptr, size_t size, void *pw)
{
    /* The semantics of this function are the same as for realloc(). */
    return PyMem_Realloc(ptr, size);
}

static PyObject *
pyhubbub_create_parser(PyObject *self, PyObject *args)
{
    const char *enc;
    int fix_enc;
    hubbub_error err;
    hubbub_parser_optparams params;
    pyhubbub_ParserObject *parser;
    PyObject *tree_handler_py;

    if (!PyArg_ParseTuple(args, "ziO", &enc, &fix_enc, &tree_handler_py))
        return NULL;

    parser = PyObject_New(pyhubbub_ParserObject, &pyhubbub_ParserType);

    err = hubbub_parser_create(enc, fix_enc, allocator, NULL, &parser->the_parser);
    if (err != HUBBUB_OK) {
        /* TODO: error handling */
        return NULL;
    }

    params.tree_handler = &tree_handler;
    /* TODO: multiple parsers? */
    Py_INCREF(tree_handler_py);
    tree_handler.ctx = tree_handler_py;
    parser->tree_handler = tree_handler_py;
    hubbub_parser_setopt(parser->the_parser, HUBBUB_PARSER_TREE_HANDLER, &params);

    parser->document_node = PyObject_GetAttrString(tree_handler_py, "document_node");
    Py_INCREF(parser->document_node);
    params.document_node = (void *)parser->document_node;
    hubbub_parser_setopt(parser->the_parser, HUBBUB_PARSER_DOCUMENT_NODE, &params);

    PyObject *result = Py_BuildValue("O", parser);
    Py_DECREF(parser);
    return result;
}

static void
pyhubbub_parser_dealloc(pyhubbub_ParserObject *self)
{
    hubbub_parser_destroy(self->the_parser);
    Py_XDECREF(self->document_node);
    Py_XDECREF(self->tree_handler);
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *
pyhubbub_parser_parse_chunk(pyhubbub_ParserObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t len;
    hubbub_error err;

    if (!PyArg_ParseTuple(args, "s#", &data, &len))
        return NULL;

    /* XXX: is it ok to cast signed * -> unsigned *? */
    err = hubbub_parser_parse_chunk(self->the_parser, (const uint8_t *)data, len);
    if (err != HUBBUB_OK) {
        // TODO: error check
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
pyhubbub_parser_completed(pyhubbub_ParserObject *self)
{
    hubbub_error err;

    /* XXX: is it ok to cast signed * -> unsigned *? */
    err = hubbub_parser_completed(self->the_parser);
    // TODO: error check

    Py_RETURN_NONE;
}

static PyMethodDef module_methods[] = {
    {"create_parser", pyhubbub_create_parser, METH_VARARGS, "Create a Parser instance."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

PyMODINIT_FUNC
init_hubbub(void)
{
    PyObject *m;
    m = Py_InitModule("_hubbub", module_methods);
    if (m == NULL)
        return;

    if (PyType_Ready(&pyhubbub_ParserType) < 0)
        return;

    Py_INCREF(&pyhubbub_ParserType);
    PyModule_AddObject(m, "Parser", (PyObject *)&pyhubbub_ParserType);
}

static inline PyObject *hubbub_string_to_unicode(const hubbub_string *data)
{
    /* ok to cast? */
    return PyUnicode_FromStringAndSize((const char *)data->ptr, data->len);
}
static hubbub_error create_comment(void *ctx, const hubbub_string *data, 
		void **result)
{
    PyObject *arg_data = hubbub_string_to_unicode(data);
    *result = PyObject_CallMethod(ctx, "create_comment", "O", arg_data);
    Py_DECREF(arg_data);
    return *result != NULL ? HUBBUB_OK : HUBBUB_INVALID;
}
static hubbub_error create_doctype(void *ctx, const hubbub_doctype *doctype,
		void **result)
{
    PyObject *arg_name, *arg_public, *arg_system;
    arg_name = hubbub_string_to_unicode(&doctype->name);
    if (doctype->public_missing) {
        Py_INCREF(Py_None);
        arg_public = Py_None;
    } else {
        arg_public = hubbub_string_to_unicode(&doctype->public_id);
    }
    if (doctype->system_missing) {
        Py_INCREF(Py_None);
        arg_system = Py_None;
    } else {
        arg_system = hubbub_string_to_unicode(&doctype->system_id);
    }
    *result = PyObject_CallMethod(ctx, "create_doctype", "OOOi",
                                  arg_name, arg_public, arg_system,
                                  doctype->force_quirks);
    Py_DECREF(arg_name);
    Py_DECREF(arg_public);
    Py_DECREF(arg_system);
    return *result != NULL ? HUBBUB_OK : HUBBUB_INVALID;
}
static hubbub_error create_element(void *ctx, const hubbub_tag *tag, 
		void **result)
{
    PyObject *name = hubbub_string_to_unicode(&tag->name);
    PyObject *elem = PyObject_CallMethod(ctx, "create_element", "iO", tag->ns, name);
    if (elem == NULL)
        return HUBBUB_INVALID;

    for (uint32_t i = 0; i < tag->n_attributes; i++) {
        const hubbub_attribute *a = &tag->attributes[i];
        PyObject *attr_name = hubbub_string_to_unicode(&a->name);
        PyObject *attr_value = hubbub_string_to_unicode(&a->value);
        PyObject_CallMethod(ctx, "add_attribute", "OiOO", elem, a->ns, attr_name, attr_value);
        Py_DECREF(attr_name);
        Py_DECREF(attr_value);
    }

    Py_DECREF(name);
    *result = elem;
    return HUBBUB_OK;
}
static hubbub_error create_text(void *ctx, const hubbub_string *data, 
		void **result)
{
    PyObject *arg_data = hubbub_string_to_unicode(data);
    *result = PyObject_CallMethod(ctx, "create_text", "O", arg_data);
    Py_DECREF(arg_data);
    return *result != NULL ? HUBBUB_OK : HUBBUB_INVALID;
}
static hubbub_error ref_node(void *ctx, void *node)
{
    Py_XINCREF(node);
    return HUBBUB_OK;
}
static hubbub_error unref_node(void *ctx, void *node)
{
    Py_XDECREF(node);
    return HUBBUB_OK;
}
static hubbub_error append_child(void *ctx, void *parent, void *child, 
		void **result)
{
    *result = PyObject_CallMethod(ctx, "append_child", "OO", parent, child);
    return *result != NULL ? HUBBUB_OK : HUBBUB_INVALID;
}
static hubbub_error insert_before(void *ctx, void *parent, void *child, 
		void *ref_child, void **result)
{
    *result = PyObject_CallMethod(ctx, "insert_before", "OOO", parent, child, ref_child);
    return *result != NULL ? HUBBUB_OK : HUBBUB_INVALID;
}
static hubbub_error remove_child(void *ctx, void *parent, void *child, 
		void **result)
{
    *result = PyObject_CallMethod(ctx, "remove_child", "OO", parent, child);
    return *result != NULL ? HUBBUB_OK : HUBBUB_INVALID;
}
static hubbub_error clone_node(void *ctx, void *node, bool deep, void **result)
{
    *result = PyObject_CallMethod(ctx, "clone_node", "Oi", node, deep);
    return *result != NULL ? HUBBUB_OK : HUBBUB_INVALID;
}
static hubbub_error reparent_children(void *ctx, void *node, void *new_parent)
{
    if (PyObject_CallMethod(ctx, "remove_child", "OO", node, new_parent) != NULL)
        return HUBBUB_OK;
    else
        return HUBBUB_INVALID;
}
static hubbub_error get_parent(void *ctx, void *node, bool element_only, 
		void **result)
{
    *result = PyObject_CallMethod(ctx, "get_parent", "Oi", node, element_only);
    if (*result == NULL)
        return HUBBUB_INVALID;

    if (*result == Py_None) {
        Py_DECREF(*result);
        *result = NULL;
    }
    return HUBBUB_OK;
}
static hubbub_error has_children(void *ctx, void *node, bool *result)
{
    PyObject *res = PyObject_CallMethod(ctx, "has_children", "O", node);
    if (res == NULL)
        return HUBBUB_INVALID;

    *result = PyObject_Not(res) ? false : true;
    Py_DECREF(res);
    return HUBBUB_OK;
}
static hubbub_error form_associate(void *ctx, void *form, void *node)
{
    if (PyObject_CallMethod(ctx, "form_associate", "OO", form, node) != NULL)
        return HUBBUB_OK;
    else
        return HUBBUB_INVALID;
}
static hubbub_error add_attributes(void *ctx, void *node,
		const hubbub_attribute *attributes, uint32_t n_attributes)
{
    for (uint32_t i = 0; i < n_attributes; i++) {
        const hubbub_attribute *a = &attributes[i];
        PyObject *attr_name = hubbub_string_to_unicode(&a->name);
        PyObject *attr_value = hubbub_string_to_unicode(&a->value);
        PyObject_CallMethod(ctx, "add_attribute", "OiOO", node, a->ns, attr_name, attr_value);
        Py_DECREF(attr_name);
        Py_DECREF(attr_value);
    }
    return HUBBUB_OK;
}
static hubbub_error set_quirks_mode(void *ctx, hubbub_quirks_mode mode)
{
    if (PyObject_CallMethod(ctx, "set_quirks_mode", "i", mode) != NULL)
        return HUBBUB_OK;
    else
        return HUBBUB_INVALID;
}
static hubbub_error change_encoding(void *ctx, const char *charset)
{
    if (PyObject_CallMethod(ctx, "encoding_change", "s", charset) != NULL)
        return HUBBUB_OK;
    else
        return HUBBUB_INVALID;
}
