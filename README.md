# Compressonator Python

A Python wrapper/binding for AMD Compressonator texture compression library.

## Overview

Compressonator Python provides Python bindings for AMD's Compressonator texture compression library,
enabling efficient texture compression and decompression operations directly from Python code.

## Installation

```bash
pip install compressonator-py
```

## Usage

```python
from compressonator_py import CMP_Texture, CMP_Format, CMP_ConvertTexture

# simple roundtrip

# generate an empty 128x128 RGBA 8bit texture
width, height = (128, 128)
pitch = 0
data = bytes(width*height*4)
tex = CMP_Texture(width, height, pitch, CMP_Format.RGBA_8888, pData = data)
# compress to BC7
tex_bc7 = CMP_Texture(width, height, pitch, CMP_Format.BC7)
CMP_ConvertTexture(tex, tex_bc7)

# decompress back
tex_re = CMP_Texture(128, 128, pitch, CMP_Format.RGBA_8888)
CMP_ConvertTexture(tex_bc7, tex_re)

# check result
assert tex.data != tex_bc7
assert tex.data == tex_re.data
```

## Features

- Texture compression and decompression
- Support for multiple formats (BC1-BC7, ASTC, ETC, etc.)
- Quality control options
- Cross-platform compatibility
