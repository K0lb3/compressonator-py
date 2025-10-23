#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include "compressonator.h"
#include "./CMP_CompressOptions.hpp"
#include "./CMP_Texture.hpp"

PyObject *CMP_TexturePy_ObjectType = nullptr;
PyObject *CMP_CompressOptionsPy_ObjectType = nullptr;

///////////////////////////////////////////////////////////////////////////////////

static PyObject *CMP_ConvertTexturePy(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *kwlist[] = {
        "pSourceTexture",
        "pDestTexture",
        "pOptions",
        // "pFeedbackProc",
        NULL};
    CMP_TexturePy *source = nullptr;
    CMP_TexturePy *dest = nullptr;
    CMP_CompressOptionsPy *options_py = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!|O", (char **)kwlist,
                                     // required
                                     CMP_TexturePy_ObjectType, &source, CMP_TexturePy_ObjectType, &dest,
                                     // optiona
                                     &options_py))
    {
        return nullptr;
    }
    if (!(options_py == nullptr || (PyObject *)options_py == Py_None || PyObject_IsInstance((PyObject *)options_py, (PyObject *)CMP_CompressOptionsPy_ObjectType)))
    {
        return PyErr_Format(PyExc_TypeError, "options has to be None or of type CMP_CompressOptions");
    }

    CMP_ERROR err = CMP_ABORTED;
    Py_BEGIN_ALLOW_THREADS;
    try
    {
        CMP_CompressOptions *options;
        if (options_py == nullptr || (PyObject *)options_py == Py_None)
        {
            options = new CMP_CompressOptions();
            options->dwSize = sizeof(CMP_CompressOptions);
            options->fquality = 0.05f;
            options->dwnumThreads = 8;
        }
        else
        {
            options = &options_py->options;
        }

        err = CMP_ConvertTexture(
            &source->texture,
            &dest->texture,
            options,
            nullptr);

        if (options_py == nullptr || (PyObject *)options_py == Py_None)
        {
            delete options;
        }
    }
    catch (std::exception &e)
    {
        return PyErr_Format(PyExc_RuntimeError, e.what());
    }
    Py_END_ALLOW_THREADS;
    if (err != CMP_OK)
    {
        return PyErr_Format(PyExc_RuntimeError, "failed to convert with error %d", (int)err);
    }
    Py_RETURN_NONE;
}

static PyMethodDef Compressonator_functions[] = {
    {"CMP_ConvertTexture", (PyCFunction)CMP_ConvertTexturePy, METH_VARARGS | METH_KEYWORDS, "converts the src texture to the dest texture"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static PyModuleDef Compressonator_module = {
    PyModuleDef_HEAD_INIT,
    "Compressonator._compressonator", // Module name
    "a python wrapper for Compressonator",
    -1,                       // Optional size of the module state memory
    Compressonator_functions, // Optional table of module-level functions
    NULL,                     // Optional slot definitions
    NULL,                     // Optional traversal function
    NULL,                     // Optional clear function
    NULL                      // Optional module deallocation function
};

int add_object(PyObject *module, const char *name, PyObject *object)
{
    Py_IncRef(object);
    if (PyModule_AddObject(module, name, object) < 0)
    {
        Py_DecRef(object);
        Py_DecRef(module);
        return -1;
    }
    return 0;
}

// void init_format_num()
// {
//     printf("import format");
//     auto module = PyImport_ImportModule(".cmp_format");
//     printf("getattr");
//     FormatEnum = PyObject_GetAttrString(module, "CMP_FORMAT");
//     printf("decref");
//     Py_DecRef(module);
// }

PyMODINIT_FUNC PyInit__compressonator(void)
{
    PyObject *m = PyModule_Create(&Compressonator_module);
    if (m == NULL)
    {
        return NULL;
    }
    // init_format_num();
    CMP_TexturePy_ObjectType = PyType_FromSpec(&CMP_TexturePy_Spec);
    if (add_object(m, "CMP_Texture", CMP_TexturePy_ObjectType) < 0)
    {
        return NULL;
    }
    CMP_CompressOptionsPy_ObjectType = PyType_FromSpec(&CMP_CompressOptionsPy_Spec);
    if (add_object(m, "CMP_CompressOptions", CMP_CompressOptionsPy_ObjectType) < 0)
    {
        return NULL;
    }
    return m;
}