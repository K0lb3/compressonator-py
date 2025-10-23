#include <Python.h>
#include "structmember.h"
#include "compressonator.h"
#include "pyhelper.hpp"

typedef struct
{
    PyObject_HEAD
        //*//
        CMP_Texture texture;
    Py_buffer *inputTextureDataView;
} CMP_TexturePy;

static int CMP_TexturePy_init(CMP_TexturePy *self, PyObject *args, PyObject *kwargs)
{
    const char *kwlist[] = {
        //"dwSize",
        "dwWidth",
        "dwHeight",
        "dwPitch",
        "format",
        "transcodeFormat",
        "nBlockHeight",
        "nBlockWidth",
        "nBlockDepth",
        "pData",
        NULL};
    self->inputTextureDataView = nullptr;
    auto &texture = self->texture;
    texture.dwSize = sizeof(CMP_Texture);
    texture.nBlockHeight = 4;
    texture.nBlockWidth = 4;
    texture.nBlockDepth = 1;
    texture.transcodeFormat = CMP_FORMAT_MAX;
    texture.pMipSet = nullptr;
    PyObject *pyData = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "IIII|IbbbO", (char **)kwlist,
                                     // required
                                     &texture.dwWidth, &texture.dwHeight, &texture.dwPitch, &texture.format,
                                     // optional
                                     &texture.transcodeFormat,
                                     &texture.nBlockHeight, &texture.nBlockWidth, &texture.nBlockDepth, &pyData))
    {
        return -1;
    }

    texture.dwDataSize = CMP_CalculateBufferSize(&texture);
    if (pyData != nullptr && pyData != Py_None)
    {
        if (!PyObject_CheckBuffer(pyData))
        {
            PyErr_SetString(PyExc_ValueError, "given pData doesn't support the buffer interface!");
            return -1;
        }
        self->inputTextureDataView = new Py_buffer();
        if (PyObject_GetBuffer(pyData, self->inputTextureDataView, PyBUF_SIMPLE) == -1)
        {
            return -1;
        }
        if (texture.dwDataSize != self->inputTextureDataView->len)
        {
            PyErr_SetString(PyExc_ValueError, "invalid data size");
            return -1;
        }
        texture.pData = (uint8_t *)self->inputTextureDataView->buf;
    }
    else
    {
        texture.pData = (uint8_t *)malloc(texture.dwDataSize);
    }
    return 0;
}

static int CMP_TexturePy_dealloc(CMP_TexturePy *self)
{
    if (self->inputTextureDataView != nullptr)
    {
        PyBuffer_Release(self->inputTextureDataView);
        self->inputTextureDataView = nullptr;
        self->texture.pData = nullptr;
    }
    else if (self->texture.pData != nullptr)
    {
        free(self->texture.pData);
        self->texture.pData = nullptr;
    }
    PyObject_Del(self);
    return 0;
}

static PyMemberDef CMP_TexturePy_members[] = {
    {"dwSize", T_UINT, offsetof(CMP_TexturePy, texture.dwSize), READONLY, "Size of this structure."},
    {"dwWidth", T_UINT, offsetof(CMP_TexturePy, texture.dwWidth), READONLY, "Width of the texture."},
    {"dwHeight", T_UINT, offsetof(CMP_TexturePy, texture.dwHeight), READONLY, "Height of the texture."},
    {"dwPitch", T_UINT, offsetof(CMP_TexturePy, texture.dwPitch), READONLY, "Distance to start of next line, necessary only for uncompressed textures."},
    {"format", GetEnumPythonTypeCode<CMP_FORMAT>(), offsetof(CMP_TexturePy, texture.format), READONLY, "Format of the texture."},
    {"transcodeFormat", GetEnumPythonTypeCode<CMP_FORMAT>(), offsetof(CMP_TexturePy, texture.transcodeFormat), READONLY, "Format of the texture."},
    {"nBlockHeight", T_UBYTE, offsetof(CMP_TexturePy, texture.nBlockHeight), READONLY, "if the source is a compressed format, specify its block dimensions (Default nBlockHeight = 4)."},
    {"nBlockWidth", T_UBYTE, offsetof(CMP_TexturePy, texture.nBlockWidth), READONLY, "(Default nBlockWidth = 4)"},
    {"nBlockDepth", T_UBYTE, offsetof(CMP_TexturePy, texture.nBlockDepth), READONLY, "For ASTC this is the z setting. (Default nBlockDepth = 1)"},
    {"dwDataSize", T_UINT, offsetof(CMP_TexturePy, texture.dwDataSize), READONLY, "Size of the current pData texture data"},
    // CMP_BYTE * pData; // Pointer to the texture data to process, this can be the
    //                     // image source or a specific MIP level
    // CMP_VOID *pMipSet;  // Pointer to a MipSet structure, typically used by Load Texture
    //                     // and Save Texture. Users can access any MIP level or cube map
    //                     // buffer using MIP Level access API and this pointer.
    {nullptr},
};

// static PyObject *
// CMP_TexturePy_get_format(CMP_TexturePy *self, void *closure)
// {
//     if (FormatEnum == nullptr)
//     {
//         return PyErr_Format(PyExc_AssertionError, "FormatEnum wasn't passed to the C-Code!");
//     }
//     PyObject *ival = PyLong_FromUnsignedLong(self->texture.format);
//     PyObject *args = PyTuple_Pack(1, ival);
//     PyObject *eval = PyObject_Call(FormatEnum, args, nullptr);
//     Py_DecRef(args);
//     Py_DecRef(ival);
//     return eval;
// }

// static PyObject *
// CMP_TexturePy_get_transcodeFormat(CMP_TexturePy *self, void *closure)
// {
//     if (FormatEnum == nullptr)
//     {
//         return PyErr_Format(PyExc_AssertionError, "FormatEnum wasn't passed to the C-Code!");
//     }
//     PyObject *ival = PyLong_FromUnsignedLong(self->texture.transcodeFormat);
//     PyObject *args = PyTuple_Pack(1, ival);
//     PyObject *eval = PyObject_Call(FormatEnum, args, nullptr);
//     Py_DecRef(args);
//     Py_DecRef(ival);
//     return eval;
// }

static PyObject *
CMP_TexturePy_get_pData(PyObject *self, void *closure)
{
    // leveraging the buffer protocol
    return PyBytes_FromObject(self);
}

typedef struct
{
    uint8_t itemsize;
    char *format;
    uint8_t channels;
} FormatInfo;

static FormatInfo getFormatInfo(CMP_FORMAT format)
{
    FormatInfo info = {
        sizeof(uint8_t),
        "B",
        0};
    if (
        // compressed
        (format & 0x1) ||
        // Lossless Based Compression Formats
        (format == 0x2000) ||
        // New Compression Formats
        (format == CMP_FORMAT_BINARY) || (format == CMP_FORMAT_GTC) || (format == CMP_FORMAT_BASIS) ||
        // can't be represented
        (format == CMP_FORMAT_ARGB_2101010) ||
        (format == CMP_FORMAT_RGBA_1010102) ||
        (format == CMP_FORMAT_RGBE_32F))
    {
        // compressed
    }
    else if (format <= CMP_FORMAT_R_8)
    {
        if (format == CMP_FORMAT_RGBA_8888_S || format == CMP_FORMAT_ARGB_8888_S || format == CMP_FORMAT_RGB_888_S || format == CMP_FORMAT_RG_8_S || format == CMP_FORMAT_R_8_S)
        {
            // signed
            info.format = "b";
        }
        if (format <= CMP_FORMAT_BGRA_8888)
        {
            info.channels = 4;
        }
        else if (format <= CMP_FORMAT_BGR_888)
        {
            info.channels = 3;
        }
        else if (format <= CMP_FORMAT_RG_8)
        {
            info.channels = 2;
        }
        else
        {
            info.channels = 1;
        }
    }
    else if (format >= CMP_FORMAT_ARGB_16 && format <= CMP_FORMAT_R_16)
    {
        // 16 bit per channel
        info.itemsize = sizeof(uint16_t);
        info.format = "H";
        if (format <= CMP_FORMAT_BGRA_16)
        {
            info.channels = 4;
        }
        else if (format <= CMP_FORMAT_RG_16)
        {
            info.channels = 2;
        }
        else
        {
            info.channels = 1;
        }
    }
    else if (format >= CMP_FORMAT_ARGB_16F && format <= CMP_FORMAT_R_16F)
    {
        // 16 bit per channel
        info.itemsize = sizeof(float);
        info.format = "f";
        if (format <= CMP_FORMAT_BGRA_16F)
        {
            info.channels = 4;
        }
        else if (format <= CMP_FORMAT_RG_16F)
        {
            info.channels = 2;
        }
        else
        {
            info.channels = 1;
        }
    }
    else if (format >= CMP_FORMAT_ARGB_32F && format <= CMP_FORMAT_R_32F)
    {
        // 32 bit per channel
        info.itemsize = sizeof(double);
        info.format = "d";
        if (format <= CMP_FORMAT_BGRA_32F)
        {
            info.channels = 4;
        }
        else if (format <= CMP_FORMAT_RG_32F)
        {
            info.channels = 2;
        }
        else
        {
            info.channels = 1;
        }
    }
    return info;
}

// static PyObject *
// CMP_TexturePy_get___array_interface__(CMP_TexturePy *self, void *closure)
// {
//     auto info = getFormatInfo(self->texture.format);
//     // numpy's protocol - for usage with PIL
//     PyObject *ret = PyDict_New();

//     PyObject* shape;
//     if (info.channels == 0){
//         shape = PyTuple_Pack()
//     }
//     auto shape = PyTuple_Pack(2, info.channels)
//     PyDict_SetItemString(ret, "shape", );
//     PyDict_SetItemString(ret, "typestr", info.format);
//     auto data = PyTuple_Pack(2, self->texture.pData, 0);
//     PyDict_SetItemString(ret, "data", );
//     PyDict_SetItemString(ret, "version", PyLong_FromLong(3));
//     return ret;
// }

static PyGetSetDef CMP_TexturePy_getsetters[] = {
    // {"format", (getter)CMP_TexturePy_get_format, nullptr, "Format of the texture.", NULL},
    // {"transcodeFormat", (getter)CMP_TexturePy_get_transcodeFormat, nullptr, "", NULL},
    {"pData", (getter)CMP_TexturePy_get_pData, nullptr, "", NULL},
    // {"__array_interface__", (getter)CMP_TexturePy_get___array_interface__, nullptr, "", NULL},
    {NULL} /* Sentinel */
};

static int
CMP_TexturePy_getbuffer(CMP_TexturePy *self, Py_buffer *view, int flags)
{
    if (view == NULL)
    {
        PyErr_SetString(PyExc_ValueError, "NULL view in getbuffer");
        return -1;
    }
    view->obj = (PyObject *)self;
    view->buf = (void *)self->texture.pData;
    view->len = self->texture.dwDataSize;
    view->readonly = 1;
    view->strides = NULL;
    view->suboffsets = NULL;
    view->internal = NULL;

    auto info = getFormatInfo(self->texture.format);
    if (info.channels)
    {
        view->ndim = 2;
        view->itemsize = info.itemsize;
        view->format = info.format;
        view->shape = new Py_ssize_t[2]();
        view->shape[0] = self->texture.dwDataSize / (info.channels * info.itemsize);
        view->shape[1] = info.channels;
    }
    else
    {
        // compressed or unsupported
        view->ndim = 1;
        view->itemsize = sizeof(uint8_t);
        view->format = "B";
        view->shape = new Py_ssize_t[1]();
        view->shape[0] = self->texture.dwDataSize;
    }

    Py_INCREF(self); // need to increase the reference count
    return 0;
}

static void
CMP_TexturePy_releasebuffer(PyObject *exporter, Py_buffer *view)
{
    if (view->shape != NULL)
    {
        delete view->shape;
        view->shape = NULL;
    }
}

PyType_Slot CMP_TexturePy_slots[] = {
    {Py_tp_new, (void *)PyType_GenericNew},
    {Py_tp_init, (void *)CMP_TexturePy_init},
    {Py_tp_dealloc, (void *)CMP_TexturePy_dealloc},
    {Py_tp_doc, (void *)"CMP_Texture"},
    //{Py_tp_repr, (void *)CMP_TexturePy_repr},
    {Py_tp_getset, CMP_TexturePy_getsetters},
    {Py_tp_members, CMP_TexturePy_members},
    {Py_bf_getbuffer, (getbufferproc)CMP_TexturePy_getbuffer},
    {Py_bf_releasebuffer, (releasebufferproc)CMP_TexturePy_releasebuffer},
    //{Py_tp_methods, CMP_TexturePy_methods},
    {0, NULL},
};

static PyType_Spec CMP_TexturePy_Spec = {
    "Compressonator.CMP_Texture",             // const char* name;
    sizeof(CMP_TexturePy),                    // int basicsize;
    0,                                        // int itemsize;
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // unsigned int flags;
    CMP_TexturePy_slots,                      // PyType_Slot *slots;
};