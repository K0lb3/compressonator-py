from typing import Optional

from ._compressonator import (
    AMD_CMD_Set,
    CMP_ChannelFormat,
    CMP_CompressOptions,
    CMP_ComputeType,
    CMP_Format,
    CMP_GPUDecode,
    CMP_Speed,
    CMP_Texture,
    CMP_TextureDataType,
    CMP_TextureType,
    KernelDeviceInfo,
    KernelPerformanceStats,
)


def CMP_ConvertTexture(src: CMP_Texture, dest: CMP_Texture, options: Optional[CMP_CompressOptions]) -> None:
    # for backwards compatibility
    return src.convert(dest, options)


__version__ = "0.0.3"

__all__ = [
    "AMD_CMD_Set",
    "CMP_CompressOptions",
    "CMP_Texture",
    "KernelDeviceInfo",
    "KernelPerformanceStats",
    "CMP_ChannelFormat",
    "CMP_ComputeType",
    "CMP_Format",
    "CMP_GPUDecode",
    "CMP_Speed",
    "CMP_TextureDataType",
    "CMP_TextureType",
    "CMP_ConvertTexture__version__",
]
