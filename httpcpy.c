#include "httppycparser.h"
#include "httppycommon.h"
#include <Python.h>
#include <structmember.h>

struct http_pyctx {
  struct httppy_parserctx pctx;
  char buf[1024];
  size_t sz;
  int ok;
};


typedef struct {
  PyObject_HEAD;
  struct http_pyctx pyctx;
} HttpObject;

static PyObject *
Http_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    HttpObject *self;
    self = (HttpObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
      self->pyctx.sz = 0;
      self->pyctx.ok = 0;
      httppy_parserctx_init(&self->pyctx.pctx);
    }
    return (PyObject *) self;
}

static void
Http_dealloc(HttpObject *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Http_init(HttpObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|", kwlist))
        return -1;
    return 0;
}

static struct PyMemberDef Http_members[] = {
  {}
};

static PyObject *
Http_host(HttpObject *self, PyObject *Py_UNUSED(ignored))
{
    if (!self->pyctx.ok)
    {
        return Py_BuildValue("");
    }
    return PyUnicode_FromFormat("%s", self->pyctx.buf);
}

static PyObject *
Http_feed(HttpObject *self, PyObject *args)
{
  const char *dat;
  int size;
  int p;
  ssize_t consumed;
  if (!PyArg_ParseTuple(args, "s#p", &dat, &size, &p))
  {
    return NULL;
  }
  consumed = httppy_parse_block(&self->pyctx.pctx, dat, size, !!p);
  return PyLong_FromLong(consumed);
}

static PyMethodDef Http_methods[] = {
    {"host", (PyCFunction) Http_host, METH_NOARGS,
     "Return the host"
    },
    {"feed", (PyCFunction) Http_feed, METH_VARARGS,
     "Return the host"
    },
    {}  /* Sentinel */
};

static PyTypeObject HttpType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "httpparser.Http",
  .tp_doc = "HTTP parser",
  .tp_basicsize = sizeof(HttpObject),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = Http_new,
  .tp_init = (initproc) Http_init,
  .tp_dealloc = (destructor) Http_dealloc,
  .tp_members = Http_members,
  .tp_methods = Http_methods,
};

static PyModuleDef httpparsermodule = {
  PyModuleDef_HEAD_INIT,
  .m_name = "httpparser",
  .m_doc = "Example HTTP parser.",
  .m_size = -1,
};

PyMODINIT_FUNC
PyInit_httpparser(void)
{
  PyObject *m;
  if (PyType_Ready(&HttpType) < 0)
    return NULL;

  m = PyModule_Create(&httpparsermodule);
  if (m == NULL)
    return NULL;

  Py_INCREF(&HttpType);
  PyModule_AddObject(m, "Http", (PyObject *) &HttpType);
  return m;
}
  


ssize_t store(const char *buf, size_t siz, int start, struct httppy_parserctx *btn)
{
  struct http_pyctx *pyctx = CONTAINER_OF(btn, struct http_pyctx, pctx);
  if (pyctx->sz + siz > sizeof(pyctx->buf) - 1)
  {
    siz = sizeof(pyctx->buf) - pyctx->sz - 1;
  }
  memcpy(pyctx->buf+pyctx->sz, buf, siz);
  pyctx->sz += siz;
  pyctx->buf[pyctx->sz] = '\0';
  if (start & YALE_FLAG_MAJOR_MISTAKE)
  {
    abort();
  }
  if (start & YALE_FLAG_END)
  {
    if (pyctx->sz > 0 && pyctx->buf[pyctx->sz - 1] == '\n')
    {
      pyctx->sz--;
      pyctx->buf[pyctx->sz] = '\0';
    }
    if (pyctx->sz > 0 && pyctx->buf[pyctx->sz - 1] == '\r')
    {
      pyctx->sz--;
      pyctx->buf[pyctx->sz] = '\0';
    }
    pyctx->ok = 1;
    return -EPIPE;
  }
  return -EAGAIN;
}
