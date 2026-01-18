#include "./CMP_Texture.hpp"
#include <string>
#include <cstring>

int CMP_TexturePy_init(CMP_TexturePy *self, PyObject *args, PyObject *kwargs)
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
    self->buffer_view = {0};
    auto &texture = self->texture;
    texture.dwSize = sizeof(CMP_Texture);
    texture.nBlockHeight = 4;
    texture.nBlockWidth = 4;
    texture.nBlockDepth = 1;
    texture.transcodeFormat = CMP_FORMAT_MAX;
    texture.pMipSet = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "IIII|IbbbO", (char **)kwlist,
                                     // required
                                     &texture.dwWidth, &texture.dwHeight, &texture.dwPitch, &texture.format,
                                     // optional
                                     &texture.transcodeFormat,
                                     &texture.nBlockHeight, &texture.nBlockWidth, &texture.nBlockDepth, &self->pDataObj))
    {
        return -1;
    }

    texture.dwDataSize = CMP_CalculateBufferSize(&texture);

    if (self->pDataObj == nullptr || self->pDataObj == Py_None)
    {
        self->pDataObj = PyByteArray_FromStringAndSize(nullptr, texture.dwDataSize);
        memset(PyByteArray_AsString(self->pDataObj), 0, texture.dwDataSize);
    }
    else
    {
        Py_IncRef(self->pDataObj);
    }

    if (PyObject_GetBuffer(self->pDataObj, &self->buffer_view, PyBUF_SIMPLE) == -1)
    {
        PyErr_SetString(PyExc_ValueError, "failed to get buffer from pData object");
        Py_DecRef(self->pDataObj);
        self->pDataObj = nullptr;
        return -1;
    }

    if (self->buffer_view.len < texture.dwDataSize)
    {
        std::string err = "provided data size is too small, expected at least " + std::to_string(texture.dwDataSize) + " bytes";
        PyErr_SetString(PyExc_ValueError, err.c_str());
        PyBuffer_Release(&self->buffer_view);
        // Drop our owned reference to the provided buffer object to avoid a leak
        Py_DecRef(self->pDataObj);
        self->pDataObj = nullptr;
        // Ensure we don't attempt to release the buffer again during dealloc
        self->buffer_view.buf = nullptr;
        return -1;
    }
    texture.pData = (uint8_t *)self->buffer_view.buf;

    return 0;
}

void CMP_TexturePy_dealloc(CMP_TexturePy *self)
{
    if (self->buffer_view.buf != nullptr)
    {
        PyBuffer_Release(&self->buffer_view);
    }
    if (self->pDataObj != nullptr)
    {
        Py_DecRef(self->pDataObj);
        self->pDataObj = nullptr;
    }
    self->texture.pData = nullptr;
    /* Use the type's tp_free to match allocation from PyType_GenericNew */
    Py_TYPE((PyObject *)self)->tp_free((PyObject *)self);
}

// PyObject *
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

// PyObject *
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

PyObject *
CMP_TexturePy_get_pData(PyObject *self, void *closure)
{
    CMP_TexturePy *obj = reinterpret_cast<CMP_TexturePy *>(self);
    if (obj->pDataObj == NULL)
    {
        Py_RETURN_NONE; // or raise, depending on your contract
    }
    Py_IncRef(obj->pDataObj);
    return obj->pDataObj; // new reference
}

typedef struct
{
    uint8_t itemsize;
    char *format;
    uint8_t channels;
} FormatInfo;

FormatInfo getFormatInfo(CMP_FORMAT format)
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

// PyObject *
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

int CMP_TexturePy_getbuffer(CMP_TexturePy *self, Py_buffer *view, int flags)
{
    if (view == NULL)
    {
        PyErr_SetString(PyExc_ValueError, "NULL view in getbuffer");
        return -1;
    }
    // Honor writability requests
    if ((flags & PyBUF_WRITABLE) && self->buffer_view.readonly)
    {
        PyErr_SetString(PyExc_BufferError, "requested writable buffer but underlying data is read-only");
        return -1;
    }

    // Expose the underlying pData buffer directly to consumers to avoid any
    // lifetime or aliasing issues with wrappers. Keep a strong reference to
    // the actual buffer owner (pDataObj) so the memory stays valid for the
    // duration of the exported view.
    if (self->pDataObj == nullptr)
    {
        PyErr_SetString(PyExc_BufferError, "no backing buffer available");
        return -1;
    }
    Py_IncRef((PyObject *)self);
    view->obj = reinterpret_cast<PyObject *>(self);
    view->buf = self->buffer_view.buf;
    view->len = self->texture.dwDataSize;
    view->readonly = self->buffer_view.readonly ? 1 : 0;
    view->shape = NULL;
    view->strides = NULL;
    view->suboffsets = NULL;
    view->internal = NULL;

    const auto info = getFormatInfo(self->texture.format);
    const auto size = info.channels ? 2 : 1;
    view->ndim = (flags & PyBUF_ND) ? size : 0;
    view->itemsize = info.itemsize;
    view->format = (flags & PyBUF_FORMAT) ? info.format : NULL;

    if (flags & PyBUF_ND)
    {
        view->shape = new Py_ssize_t[size]();
        if (size == 1)
        {
            view->shape[0] = static_cast<Py_ssize_t>(self->texture.dwDataSize / info.itemsize);
        }
        else
        {
            view->shape[0] = static_cast<Py_ssize_t>(self->texture.dwDataSize / (info.channels * info.itemsize));
            view->shape[1] = info.channels;
        }
    }

    if (flags & PyBUF_STRIDES)
    {
        view->strides = new Py_ssize_t[size]();
        if (size == 1)
        {
            view->strides[0] = info.itemsize;
        }
        else
        {
            view->strides[1] = info.itemsize;                 // step between channels
            view->strides[0] = info.itemsize * info.channels; // step between rows
        }
    }

    return 0;
}

void CMP_TexturePy_releasebuffer(PyObject *exporter, Py_buffer *view)
{
    if (view->shape != NULL)
    {
        /* shape was allocated with new[] in getbuffer */
        delete[] view->shape;
        view->shape = NULL;
    }
    if (view->strides != NULL)
    {
        delete[] view->strides;
        view->strides = NULL;
    }
    /* DECREF the exported object which was INCREF'd in getbuffer */
    if (view->obj)
    {
        Py_DecRef(view->obj);
        view->obj = NULL;
    }
}
