from __future__ import annotations

from typing import Optional

from ._compressonator import CMP_ConvertTexture
from ._compressonator import CMP_Texture as CMP_TextureBase
from .enums import CMP_Format
from .options import CMP_CompressOptions


class CMP_Texture(CMP_TextureBase):
    @property
    def format(self) -> CMP_Format:  # type: ignore
        return CMP_Format(super().format)

    @property
    def transcodeFormat(self) -> CMP_Format:  # type: ignore
        return CMP_Format(super().transcodeFormat)

    def convert(
        self, format: CMP_Format, options: Optional[CMP_CompressOptions] = None
    ) -> CMP_Texture:
        dst = CMP_Texture(
            self.dwWidth,
            self.dwHeight,
            0,
            format,
        )
        CMP_ConvertTexture(self, dst, options)
        return dst
