#pragma once

#include <Python.h>
#include "structmember.h"
#include "typeslots.h"

#include "compressonator.h"

#include "pyhelper.hpp"

typedef struct
{
    PyObject_HEAD
        //*//
        CMP_Texture texture;
    Py_buffer buffer_view;
    PyObject* pDataObj;
} CMP_TexturePy;

int CMP_TexturePy_init(CMP_TexturePy *self, PyObject *args, PyObject *kwargs);
void CMP_TexturePy_dealloc(CMP_TexturePy *self);
PyObject *CMP_TexturePy_get_pData(PyObject *self, void *closure);
int CMP_TexturePy_getbuffer(CMP_TexturePy *self, Py_buffer *view, int flags);
void CMP_TexturePy_releasebuffer(PyObject *exporter, Py_buffer *view);

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
    {nullptr},
};

static PyGetSetDef CMP_TexturePy_getsetters[] = {
    {"pData", (getter)CMP_TexturePy_get_pData, nullptr, "the texture data to process", NULL},
    // {"__array_interface__", (getter)CMP_TexturePy_get___array_interface__, nullptr, "", NULL},
    {NULL} /* Sentinel */
};

static PyType_Slot CMP_TexturePy_slots[] = {
    {Py_tp_new, (void *)PyType_GenericNew},
    {Py_tp_init, (void *)CMP_TexturePy_init},
    {Py_tp_dealloc, (void *)CMP_TexturePy_dealloc},
    {Py_tp_doc, (void *)"CMP_Texture"},
    //{Py_tp_repr, (void *)CMP_TexturePy_repr},
    {Py_tp_getset, CMP_TexturePy_getsetters},
    {Py_tp_members, CMP_TexturePy_members},
// impl since 3.9 (removed from 3.8), limited api since 3.11
#if (!defined(PY_LIMITED_API) && PY_VERSION_HEX >= 0x030A0000) || (defined(PY_LIMITED_API) && PY_LIMITED_API + 0 >= 0x030B0000)
    {Py_bf_getbuffer, (getbufferproc)CMP_TexturePy_getbuffer},
    {Py_bf_releasebuffer, (releasebufferproc)CMP_TexturePy_releasebuffer},
#endif
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