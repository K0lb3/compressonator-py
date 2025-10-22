# =====================================================================
#  Copyright (c) 2007-2024    Advanced Micro Devices, Inc. All rights reserved.
#  Copyright (c) 2004-2006    ATI Technologies Inc.
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files(the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions :
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.
#
#  \file Compressonator.h
#
# =====================================================================
from typing import Optional

from .enums import CMP_Format

class AMD_CMD_Set:
    """
    TODO: list available commands

    Max command length is 32
    Max parameter length is 16
    """

    strCommand: str
    strParameter: str

class CMP_CompressOptions:
    """User options and setting used for processing

    Attributes
    ----------
    dwSize: int
        The size of this structure.
    doPreconditionBRLG: bool
    doDeltaEncodeBRLG: bool
    doSwizzleBRLG: bool
    dwPageSize: int
        Used by Brotli-G Codec for setting the page size used for compression
    bUseRefinementSteps: bool
        Used by BC1, BC2, and BC3 codecs to improve quality, this setting will increase encoding time for quality results
    nRefinementSteps:  int
        Currently only 1 step is implemented
    bUseChannelWeighting: bool
        Use channel weights. With swizzled formats the weighting applies to the data within the specified channel not the channel itself.
        Channel weigthing is not implemented for BC6H and BC7
    fWeightingRed: float
        The weighting of the Red or X Channel.
    fWeightingGreen: float
        The weighting of the Green or Y Channel.
    fWeightingBlue: float
        The weighting of the Blue or Z Channel.
     bUseAdaptiveWeighting: bool
        Adapt weighting on a per-block basis.
     bDXT1UseAlpha: bool
        Encode single-bit alpha data. Only valid when compressing to DXT1 & BC1.
     bUseGPUDecompress: bool
        Use GPU to decompress. Decode API can be changed by specified in DecodeWith parameter. Default is OpenGL.
     bUseCGCompress: bool
        Use SPMD/GPU to compress. Encode API can be changed by specified in EncodeWith parameter. Default is OpenCL.
     nAlphaThreshold: CMP_BYTE
        The alpha threshold to use when compressing to DXT1 & BC1 with bDXT1UseAlpha.
        Texels with an alpha value less than the threshold are treated as transparent.
        Note: When nCompressionSpeed is not set to Normal AphaThreshold is ignored for DXT1 & BC1
    bDisableMultiThreading: bool
        Disable multi-threading of the compression. This will slow the compression but can be useful if you're managing threads in your application.
        If set BC7 dwnumThreads will default to 1 during encoding and then return back to its original value when done.
    nCompressionSpeed: CMP_Speed
        The trade-off between compression speed & quality.
        Notes:
            1. This value is ignored for BC6H and BC7 (for BC7 the compression speed depends on fquaility value)
            2. For 64 bit DXT1 to DXT5 and BC1 to BC5 nCompressionSpeed is ignored and set to Noramal Speed
            3. To force the use of nCompressionSpeed setting regarless of Note 2 use fQuality at 0.05
    nGPUDecode: CMP_GPUDecode
        This value is set using DecodeWith argument (OpenGL, DirectX) default is OpenGL
    nEncodeWith: CMP_Compute_type
        This value is set using EncodeWith argument, currently only OpenCL is used dwnumThreads: int
        Number of threads to initialize for BC7 encoding (Max up to 128).
        Default set to auto,
    fquality: float
        Quality of encoding. This value ranges between 0.0 and 1.0. BC7 & BC6 default is 0.05, others codecs are set at 1.0.
        Setting fquality
            above 0.0 gives the fastest, lowest quality encoding,
            1.0 is the slowest, highest quality encoding.
        Default set to a low value of 0.05
    brestrictColour: bool
        This setting is a quality tuning setting for BC7 which may be necessary for convenience in some applications. Default set to false. If set and the block does not need alpha it instructs the code not to use modes that have combined colour + alpha - this avoids the possibility that the encoder might choose an alpha other than 1.0 (due to parity) and cause something to become accidentally slightly transparent (it's possible that when encoding 3-component texture applications will assume that the 4th component can safely be assumed to be 1.0 all the time.)
    brestrictAlpha: bool
        This setting is a quality tuning setting for BC7 which may be necessary for some textures. Default set to false, if set it will also apply restriction to blocks with alpha to avoid issues with punch-through or thresholded alpha encoding
    dwmodeMask: int
        Mode to set BC7 to encode blocks using any of 8 different block modes in order to obtain the highest quality. Default set to 0xFF) You can combine the bits to test for which modes produce the best image quality. The mode that produces the best image quality above a set quality level (fquality) is used and subsequent modes set in the mask are not tested, this optimizes the performance of the compression versus the required quality. If you prefer to check all modes regardless of the quality then set the fquality to a value of 0
    NumCmds: int
        Count of the number of command value pairs in CmdSet[].
        Max value that can be set is AMD_MAX_CMDS = 20 on this release
    CmdSet[AMD_MAX_CMDS]: AMD_CMD_SET
        Extended command options that can be set for the specified codec
        Example to set the number of threads and quality used for compression
        Options.CmdSet[0].strCommand = "NumThreads"
        Options.CmdSet[0].strParameter = "8"
        Options.CmdSet[1].strCommand   = "Quality"
        Options.CmdSet[1].strParameter = "1.0"
        Options.NumCmds = 2
    fInputDefog: float
        ToneMap properties for float type image send into non float compress algorithm.
    fInputExposure: float
    fInputKneeLow: float
    fInputKneeHigh: float
    fInputGamma: float
    fInputFilterGamma: float
        Gamma correction value applied for mipmap generation
    iCmpLevel:  int
        < draco setting: compression level (range 0-10: higher mean more compressed) - default 7
    iPosBits:  int
        quantization bits for position - default 14
    iTexCBits:  int
        quantization bits for texture coordinates - default 12
    iNormalBits:  int
        quantization bits for normal - default 10
    iGenericBits:  int
        quantization bits for generic - default 8
    SourceFormat: CMP_Format
    DestFormat: CMP_Format
    format_support_hostEncoder: bool
        Temp setting used while encoding with gpu or hpc plugins
    m_PrintInfoStr: CMP_PrintInfoStr
    getPerfStats: bool
        Set to true if you want to get Performance Stats
    perfStats: KernelPerformanceStats
        Data storage for the performance stats obtained from GPU or CPU while running encoder processing
                  getDeviceInfo: bool
        Set to true if you want to get target device info
          deviceInfo: KernelDeviceInfo
        Data storage for the performance stats obtained from GPU or CPU while running encoder processing
    genGPUMipMaps: bool
        When ecoding with GPU HW use it to generate MipMap images, valid only when miplevels is set else default is toplevel 1
    useSRGBFrames: bool
        when using GPU HW for encoding and mipmap generation use SRGB frames, default is RGB
     miplevels:  int
        miplevels to use when GPU is used to generate them
    """

    ...

class CMP_Texture:
    """
    A class representing a texture with various properties.

    Attributes
    ----------
    dwSize : int
        Size of this structure.
    dwWidth : int
        Width of the texture.
    dwHeight : int
        Height of the texture.
    dwPitch : int
        Distance to start of the next line, necessary only for uncompressed textures.
    format : CMP_Format
        Format of the texture.
    transcodeFormat : CMP_Format
        If the "format" is BASIS, an optional target format can be set here (default is BC1).
    nBlockHeight : int
        If the source is a compressed format, specifies its block height (default is 4).
    nBlockWidth : int
        Specifies the block width (default is 4).
    nBlockDepth : int
        Specifies the block depth (for ASTC, this is the z setting, default is 1).
    dwDataSize : int
        Size of the current texture data.
    pData : bytes or None
        Pointer to the texture data to process; this can be the image source or a specific MIP level.
    pMipSet : object or None
        Pointer to a MipSet structure, typically used by Load Texture and Save Texture.
    """

    dwSize: int
    dwWidth: int
    dwHeight: int
    dwPitch: int
    format: CMP_Format
    transcodeFormat: CMP_Format
    nBlockHeight: int
    nBlockWidth: int
    nBlockDepth: int
    dwDataSize: int
    pData: bytes

    def __buffer__(self, flags: int) -> memoryview[int]: ...
    def __init__(
        self,
        dwWidth: int,
        dwHeight: int,
        dwPitch: int,
        format: CMP_Format,
        transcodeFormat: CMP_Format = CMP_Format.MAX,
        nBlockHeight: int = 4,
        nBlockWidth: int = 4,
        nBlockDepth: int = 1,
        pData: Optional[bytes] = None,
    ) -> None: ...

def CMP_ConvertTexture(
    src: CMP_Texture, dest: CMP_Texture, options: Optional[CMP_CompressOptions]
) -> None:
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

__all__ = ("CMP_CompressOptions", "CMP_Texture", "CMP_ConvertTexture")
