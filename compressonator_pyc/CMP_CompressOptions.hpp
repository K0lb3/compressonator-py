#include <Python.h>
#include "structmember.h"
#include "compressonator.h"

typedef struct
{
    PyObject_HEAD
        //
        CMP_CompressOptions options;
} CMP_CompressOptionsPy;

PyMemberDef CMP_CompressOptionsPy_members[] = {
    {"dwSize", T_UINT, offsetof(CMP_CompressOptionsPy, options.dwSize), 0, ""},
    {"doPreconditionBRLG", T_BOOL, offsetof(CMP_CompressOptionsPy, options.doPreconditionBRLG), 0, ""},
    {"doDeltaEncodeBRLG", T_BOOL, offsetof(CMP_CompressOptionsPy, options.doDeltaEncodeBRLG), 0, ""},
    {"doSwizzleBRLG", T_BOOL, offsetof(CMP_CompressOptionsPy, options.doSwizzleBRLG), 0, ""},
    {"dwPageSize", T_UINT, offsetof(CMP_CompressOptionsPy, options.dwPageSize), 0, ""},
    {"bUseRefinementSteps", T_BOOL, offsetof(CMP_CompressOptionsPy, options.bUseRefinementSteps), 0, ""},
    {"nRefinementSteps", T_INT, offsetof(CMP_CompressOptionsPy, options.nRefinementSteps), 0, ""},
    {"bUseChannelWeighting", T_BOOL, offsetof(CMP_CompressOptionsPy, options.bUseChannelWeighting), 0, ""},
    {"fWeightingRed", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fWeightingRed), 0, ""},
    {"fWeightingGreen", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fWeightingGreen), 0, ""},
    {"fWeightingBlue", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fWeightingBlue), 0, ""},
    {"bUseAdaptiveWeighting", T_BOOL, offsetof(CMP_CompressOptionsPy, options.bUseAdaptiveWeighting), 0, ""},
    {"bDXT1UseAlpha", T_BOOL, offsetof(CMP_CompressOptionsPy, options.bDXT1UseAlpha), 0, ""},
    {"bUseGPUDecompress", T_BOOL, offsetof(CMP_CompressOptionsPy, options.bUseGPUDecompress), 0, ""},
    {"bUseCGCompress", T_BOOL, offsetof(CMP_CompressOptionsPy, options.bUseCGCompress), 0, ""},
    {"nAlphaThreshold", T_UBYTE, offsetof(CMP_CompressOptionsPy, options.nAlphaThreshold), 0, ""},
    {"bDisableMultiThreading", T_BOOL, offsetof(CMP_CompressOptionsPy, options.bDisableMultiThreading), 0, ""},
    /*
    CMP_Speed nCompressionSpeed;
    CMP_GPUDecode nGPUDecode;
    CMP_Compute_type nEncodeWith;
    */
    {"dwnumThreads", T_UINT, offsetof(CMP_CompressOptionsPy, options.dwnumThreads), 0, ""},
    {"fquality", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fquality), 0, ""},
    {"brestrictColour", T_BOOL, offsetof(CMP_CompressOptionsPy, options.brestrictColour), 0, ""},
    {"brestrictAlpha", T_BOOL, offsetof(CMP_CompressOptionsPy, options.brestrictAlpha), 0, ""},
    {"dwmodeMask", T_UINT, offsetof(CMP_CompressOptionsPy, options.dwmodeMask), 0, ""},
    /*
    int NumCmds;
    AMD_CMD_SET CmdSet[AMD_MAX_CMDS];
    */
    {"fInputDefog", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fInputDefog), 0, ""},
    {"fInputExposure", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fInputExposure), 0, ""}, //
    {"fInputKneeLow", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fInputKneeLow), 0, ""},   //
    {"fInputKneeHigh", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fInputKneeHigh), 0, ""}, //
    {"fInputGamma", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fInputGamma), 0, ""},       //
    {"fInputFilterGamma", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fInputFilterGamma), 0, ""},
    {"iCmpLevel", T_INT, offsetof(CMP_CompressOptionsPy, options.iCmpLevel), 0, ""},
    {"iPosBits", T_INT, offsetof(CMP_CompressOptionsPy, options.iPosBits), 0, ""},
    {"iTexCBits", T_INT, offsetof(CMP_CompressOptionsPy, options.iTexCBits), 0, ""},
    {"iNormalBits", T_INT, offsetof(CMP_CompressOptionsPy, options.iNormalBits), 0, ""},
    {"iGenericBits", T_INT, offsetof(CMP_CompressOptionsPy, options.iGenericBits), 0, ""},
#ifdef USE_3DMESH_OPTIMIZE
    {"iVcacheSize", T_INT, offsetof(CMP_CompressOptionsPy, options.iVcacheSize), 0, ""},
    {"iVcacheFIFOSize", T_INT, offsetof(CMP_CompressOptionsPy, options.iVcacheFIFOSize), 0, ""},
    {"fOverdrawACMR", T_FLOAT, offsetof(CMP_CompressOptionsPy, options.fOverdrawACMR), 0, ""},
    {"iSimplifyLOD", T_INT, offsetof(CMP_CompressOptionsPy, options.iSimplifyLOD), 0, ""},
    {"bVertexFetch", T_BOOL, offsetof(CMP_CompressOptionsPy, options.bVertexFetch), 0, ""},
#endif
    /*
    CMP_FORMAT SourceFormat;
    CMP_FORMAT DestFormat;
    */
    {"format_support_hostEncoder", T_BOOL, offsetof(CMP_CompressOptionsPy, options.format_support_hostEncoder), 0, ""},
    /*
    CMP_PrintInfoStr m_PrintInfoStr;
    */
    {"getPerfStats", T_BOOL, offsetof(CMP_CompressOptionsPy, options.getPerfStats), 0, ""},
    /*
    KernelPerformanceStats perfStats;
    */
    {"getDeviceInfo", T_BOOL, offsetof(CMP_CompressOptionsPy, options.getDeviceInfo), 0, ""},
    /*
    KernelDeviceInfo deviceInfo;
    */
    {"genGPUMipMaps", T_BOOL, offsetof(CMP_CompressOptionsPy, options.genGPUMipMaps), 0, ""},
    {"useSRGBFrames", T_BOOL, offsetof(CMP_CompressOptionsPy, options.useSRGBFrames), 0, ""},
    {"miplevels", T_INT, offsetof(CMP_CompressOptionsPy, options.miplevels), 0, ""},
    {nullptr}};

PyType_Slot CMP_CompressOptionsPy_slots[] = {
    {Py_tp_new, (void *)PyType_GenericNew},
    // {Py_tp_init, (void *)CMP_CompressOptionsPy_init},
    // {Py_tp_dealloc, (void *)CMP_CompressOptionsPy_dealloc},
    {Py_tp_doc, (void *)"CMP_CompressOptions"},
    //{Py_tp_repr, (void *)CMP_CompressOptionsPy_repr},
    // {Py_tp_getset, CMP_CompressOptionsPy_getsetters},
    {Py_tp_members, CMP_CompressOptionsPy_members},
    //{Py_tp_methods, CMP_CompressOptionsPy_methods},
    {0, NULL},
};

PyType_Spec CMP_CompressOptionsPy_Spec = {
    "Compressonator.CMP_CompressOptions",     // const char* name;
    sizeof(CMP_CompressOptionsPy),            // int basicsize;
    0,                                        // int itemsize;
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // unsigned int flags;
    CMP_CompressOptionsPy_slots,              // PyType_Slot *slots;
};