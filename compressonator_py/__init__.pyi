from typing import Optional

from .classes import AMD_CMD_Set, CMP_CompressOptions, CMP_Texture, KernelDeviceInfo, KernelPerformanceStats
from .enums import (
    CMP_ChannelFormat,
    CMP_ComputeType,
    CMP_Format,
    CMP_GPUDecode,
    CMP_Speed,
    CMP_TextureDataType,
    CMP_TextureType,
)

def CMP_ConvertTexture(src: CMP_Texture, dest: CMP_Texture, options: Optional[CMP_CompressOptions]) -> None:
    """
    Converts the source texture to the destination texture
    This can be compression, decompression or converting between two uncompressed formats.

    Parameters
    ----------
    src : CMP_Texture
        The source texture.
    dest : CMP_Texture
        The destination texture.
    options : Optional[CMP_CompressOptions]
        Optional compression options; if None, default options are used.

    Raises
    ------
    RuntimeError
        If an error occurs during the conversion process.
    """
    ...

__version__: str

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
    "CMP_ConvertTexture",
    "__version__",
]
