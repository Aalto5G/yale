#include "ssl1cparser.h"
#include "sslcommon.h"
#include <Python.h>
#include <structmember.h>

struct ssl_pyctx {
  struct ssl1_parserctx pctx;
  char buf[1024];
  size_t sz;
  int ok;
};


typedef struct {
  PyObject_HEAD;
  struct ssl_pyctx pyctx;
} SslObject;

static PyObject *
Ssl_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SslObject *self;
    self = (SslObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
      self->pyctx.sz = 0;
      self->pyctx.ok = 0;
      ssl1_parserctx_init(&self->pyctx.pctx);
    }
    return (PyObject *) self;
}

static void
Ssl_dealloc(SslObject *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Ssl_init(SslObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|", kwlist))
        return -1;
    return 0;
}

static struct PyMemberDef Ssl_members[] = {
  {}
};

static PyObject *
Ssl_host(SslObject *self, PyObject *Py_UNUSED(ignored))
{
    if (!self->pyctx.ok)
    {
        return Py_BuildValue("");
    }
    return PyUnicode_FromFormat("%s", self->pyctx.buf);
}

static PyObject *
Ssl_feed(SslObject *self, PyObject *args)
{
  const char *dat;
  int size;
  int p;
  ssize_t consumed;
  if (!PyArg_ParseTuple(args, "s#p", &dat, &size, &p))
  {
    return NULL;
  }
  printf("sz: %d\n", size);
  consumed = ssl1_parse_block(&self->pyctx.pctx, dat, size, !!p);
  return PyLong_FromLong(consumed);
}

static PyMethodDef Ssl_methods[] = {
    {"host", (PyCFunction) Ssl_host, METH_NOARGS,
     "Return the host"
    },
    {"feed", (PyCFunction) Ssl_feed, METH_VARARGS,
     "Return the host"
    },
    {}  /* Sentinel */
};

static PyTypeObject SslType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "sslparser.Ssl",
  .tp_doc = "SSL parser",
  .tp_basicsize = sizeof(SslObject),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = Ssl_new,
  .tp_init = (initproc) Ssl_init,
  .tp_dealloc = (destructor) Ssl_dealloc,
  .tp_members = Ssl_members,
  .tp_methods = Ssl_methods,
};

static PyModuleDef sslparsermodule = {
  PyModuleDef_HEAD_INIT,
  .m_name = "sslparser",
  .m_doc = "Example SSL parser.",
  .m_size = -1,
};

PyObject *PyInit_sslparser(void);

PyMODINIT_FUNC
PyInit_sslparser(void)
{
  PyObject *m;
  if (PyType_Ready(&SslType) < 0)
    return NULL;

  m = PyModule_Create(&sslparsermodule);
  if (m == NULL)
    return NULL;

  Py_INCREF(&SslType);
  PyModule_AddObject(m, "Ssl", (PyObject *) &SslType);
  return m;
}


ssize_t szbe1(const char *buf, size_t siz, int flags, struct ssl1_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
  {
    pctx->bytes_sz = 0;
  }
  for (i = 0; i < siz; i++)
  {
    pctx->bytes_sz <<= 8;
    pctx->bytes_sz |= (unsigned char)buf[i];
  }
  return -EAGAIN;
}
ssize_t feed1(const char *buf, size_t siz, int flags, struct ssl1_parserctx *pctx)
{
  return ssl2_parse_block(&pctx->ssl2, buf, siz, 0); // FIXME eofindicator
}

ssize_t szbe2(const char *buf, size_t siz, int flags, struct ssl2_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
  {
    pctx->bytes_sz = 0;
  }
  for (i = 0; i < siz; i++)
  {
    pctx->bytes_sz <<= 8;
    pctx->bytes_sz |= (unsigned char)buf[i];
  }
  return -EAGAIN;
}
ssize_t feed2(const char *buf, size_t siz, int flags, struct ssl2_parserctx *pctx)
{
  ssize_t result;
  printf("feed2 called\n");
  if (flags & YALE_FLAG_START)
  {
    ssl3_parserctx_init(&pctx->ssl3);
  }
  printf("feed2 eof? %d\n", !!(flags & YALE_FLAG_END));
  result = ssl3_parse_block(&pctx->ssl3, buf, siz, !!(flags & YALE_FLAG_END)); // FIXME eofindicator
  if (result != -EAGAIN && result != -EWOULDBLOCK && result != (ssize_t)siz)
  {
    return result;
  }
  if (flags & YALE_FLAG_END)
  {
    if (pctx->ssl3.stacksz > 0)
    {
      return -EINVAL;
    }
  }
  return result;
}

ssize_t szset32_3(const char *buf, size_t siz, int flags, struct ssl3_parserctx *pctx)
{
  printf("szset32_3 called\n");
  pctx->bytes_sz = 32;
  return -EAGAIN;
}

ssize_t szbe3(const char *buf, size_t siz, int flags, struct ssl3_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
  {
    pctx->bytes_sz = 0;
  }
  for (i = 0; i < siz; i++)
  {
    pctx->bytes_sz <<= 8;
    pctx->bytes_sz |= (unsigned char)buf[i];
  }
  return -EAGAIN;
}
ssize_t feed3(const char *buf, size_t siz, int flags, struct ssl3_parserctx *pctx)
{
  ssize_t result;
  printf("feed3 called siz=%zu\n", siz);
  if (flags & YALE_FLAG_START)
  {
    ssl4_parserctx_init(&pctx->ssl4);
  }
  printf("feed3 eof? %d\n", !!(flags & YALE_FLAG_END));
  result = ssl4_parse_block(&pctx->ssl4, buf, siz, !!(flags & YALE_FLAG_END));
  if (result != -EAGAIN && result != -EWOULDBLOCK && result != (ssize_t)siz)
  {
    return result;
  }
  if (flags & YALE_FLAG_END)
  {
    if (pctx->ssl4.stacksz != 1)
    {
      return -EINVAL;
    }
  }
  return result;
}

ssize_t szbe4(const char *buf, size_t siz, int flags, struct ssl4_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
  {
    pctx->bytes_sz = 0;
  }
  for (i = 0; i < siz; i++)
  {
    pctx->bytes_sz <<= 8;
    pctx->bytes_sz |= (unsigned char)buf[i];
  }
  return -EAGAIN;
}
ssize_t feed4(const char *buf, size_t siz, int flags, struct ssl4_parserctx *pctx)
{
  ssize_t result;
  printf("feed4 called siz=%zu\n", siz);
  if (flags & YALE_FLAG_START)
  {
    ssl5_parserctx_init(&pctx->ssl5);
  }
  printf("feed4 eof? %d\n", !!(flags & YALE_FLAG_END));
  result = ssl5_parse_block(&pctx->ssl5, buf, siz, !!(flags & YALE_FLAG_END));
  if (result != -EAGAIN && result != -EWOULDBLOCK && result != (ssize_t)siz)
  {
    return result;
  }
  if (flags & YALE_FLAG_END)
  {
    if (pctx->ssl5.stacksz > 0)
    {
      return -EINVAL;
    }
  }
  return result;
}

ssize_t szbe5(const char *buf, size_t siz, int flags, struct ssl5_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
  {
    pctx->bytes_sz = 0;
  }
  for (i = 0; i < siz; i++)
  {
    pctx->bytes_sz <<= 8;
    pctx->bytes_sz |= (unsigned char)buf[i];
  }
  return -EAGAIN;
}
ssize_t feed5(const char *buf, size_t siz, int flags, struct ssl5_parserctx *pctx)
{
  ssize_t result;
  printf("feed5 called siz=%zu\n", siz);
  printf("feed5 called\n");
  if (flags & YALE_FLAG_START)
  {
    ssl6_parserctx_init(&pctx->ssl6);
  }
  printf("feed5 eof? %d\n", !!(flags & YALE_FLAG_END));
  result = ssl6_parse_block(&pctx->ssl6, buf, siz, !!(flags & YALE_FLAG_END));
  if (result != -EAGAIN && result != -EWOULDBLOCK && result != (ssize_t)siz)
  {
    return result;
  }
  if (flags & YALE_FLAG_END)
  {
    if (pctx->ssl6.stacksz != 1)
    {
      return -EINVAL;
    }
  }
  return result;
}

ssize_t szbe6(const char *buf, size_t siz, int flags, struct ssl6_parserctx *pctx)
{
  size_t i;
  if (flags & YALE_FLAG_START)
  {
    pctx->bytes_sz = 0;
  }
  for (i = 0; i < siz; i++)
  {
    pctx->bytes_sz <<= 8;
    pctx->bytes_sz |= (unsigned char)buf[i];
  }
  return -EAGAIN;
}
  


ssize_t print6(const char *buf, size_t siz, int start, struct ssl6_parserctx *btn)
{
  struct ssl5_parserctx *ctx5 = CONTAINER_OF(btn, struct ssl5_parserctx, ssl6);
  struct ssl4_parserctx *ctx4 = CONTAINER_OF(ctx5, struct ssl4_parserctx, ssl5);
  struct ssl3_parserctx *ctx3 = CONTAINER_OF(ctx4, struct ssl3_parserctx, ssl4);
  struct ssl2_parserctx *ctx2 = CONTAINER_OF(ctx3, struct ssl2_parserctx, ssl3);
  struct ssl1_parserctx *ctx1 = CONTAINER_OF(ctx2, struct ssl1_parserctx, ssl2);
  struct ssl_pyctx *pyctx = CONTAINER_OF(ctx1, struct ssl_pyctx, pctx);
  printf("print6 called\n");
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
    pyctx->ok = 1;
    return -EPIPE;
  }
  return -EAGAIN;
}
