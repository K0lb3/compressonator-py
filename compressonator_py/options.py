from ._compressonator import CMP_CompressOptions as CMP_CompressOptionsBase


class CMP_CompressOptions(CMP_CompressOptionsBase):
    def __init__(self):
        # v4.5 Brotli-G parameters
        self.doPreconditionBRLG = False
        self.doDeltaEncodeBRLG = False
        self.doSwizzleBRLG = False

        # v4.3 Page size
        self.dwPageSize = 0

        # v4.2 Refinement parameters
        self.bUseRefinementSteps = False
        self.nRefinementSteps = 1

        # Channel weighting
        self.bUseChannelWeighting = False
        self.fWeightingRed = 0.0
        self.fWeightingGreen = 0.0
        self.fWeightingBlue = 0.0
        self.bUseAdaptiveWeighting = False

        # DXT/BC parameters
        self.bDXT1UseAlpha = False
        self.nAlphaThreshold = 0
        self.bDisableMultiThreading = False

        # GPU parameters
        self.bUseGPUDecompress = False
        self.bUseCGCompress = False
        self.nGPUDecode = 0  # Assume OpenGL default
        self.nEncodeWith = 0  # Assume OpenCL default
        self.dwnumThreads = 0  # Auto
        self.genGPUMipMaps = False
        self.useSRGBFrames = False
        self.miplevels = 0

        # Quality parameters
        self.nCompressionSpeed = 0  # Assume enum value for "Normal"
        self.fquality = 0.05
        self.brestrictColour = False
        self.brestrictAlpha = False
        self.dwmodeMask = 0xFF

        # Command set
        # self.NumCmds = 0
        # self.CmdSet = []

        # Tone mapping
        self.fInputDefog = 0.0
        self.fInputExposure = 0.0
        self.fInputKneeLow = 0.0
        self.fInputKneeHigh = 0.0
        self.fInputGamma = 0.0
        self.fInputFilterGamma = 0.0

        # Draco compression
        self.iCmpLevel = 7
        self.iPosBits = 14
        self.iTexCBits = 12
        self.iNormalBits = 10
        self.iGenericBits = 8

        # Mesh optimization
        self.iVcacheSize = 16
        self.iVcacheFIFOSize = 0  # Disabled
        self.fOverdrawACMR = 1.05
        self.iSimplifyLOD = 0
        self.bVertexFetch = True

        # Format information
        self.SourceFormat = 0
        self.DestFormat = 0
        self.format_support_hostEncoder = False

        # Diagnostics
        self.getPerfStats = False
        self.perfStats = None
        self.getDeviceInfo = False
        self.deviceInfo = None


__all__ = ("CMP_CompressOptions",)
