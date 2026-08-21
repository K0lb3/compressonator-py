#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

    inline void __cpuidex(int cpuInfo[4], int function_id, int subfunction_id)
    {
        (void)function_id;
        (void)subfunction_id;
        cpuInfo[0] = 0;
        cpuInfo[1] = 0;
        cpuInfo[2] = 0;
        cpuInfo[3] = 0;
    }

#ifdef __cplusplus
}
#endif
