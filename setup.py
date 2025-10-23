from __future__ import annotations

import os
from itertools import chain

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

LOCAL = ""
CMP_DIR = os.path.join(LOCAL, "compressonator")
CMP_CORE_DIR = os.path.join(CMP_DIR, "cmp_core")
CMP_COMPRESSONATORLIB_DIR = os.path.join(CMP_DIR, "cmp_compressonatorlib")


def glob(pattern: str) -> list[str]:
    # glob.glob only added root dir in 3.10
    dir, ext = os.path.split(pattern)
    assert ext.startswith("*"), "Only simple globbing supported"
    ext = ext[1:]
    return [os.path.join(dir, f) for f in os.listdir(dir) if f.endswith(ext)]


class BuildPart:
    sources: list[str]
    include_dirs: list[str]


class CompressonatorPy(BuildPart):
    sources = ["compressonator_pyc/pybind.cpp"]
    include_dirs = ["compressonator_pyc"]


class CompressonatorCore(BuildPart):
    sources = [
        f"{CMP_CORE_DIR}/shaders/bc1_encode_kernel.cpp",
        f"{CMP_CORE_DIR}/shaders/bc2_encode_kernel.cpp",
        f"{CMP_CORE_DIR}/shaders/bc3_encode_kernel.cpp",
        f"{CMP_CORE_DIR}/shaders/bc4_encode_kernel.cpp",
        f"{CMP_CORE_DIR}/shaders/bc5_encode_kernel.cpp",
        f"{CMP_CORE_DIR}/shaders/bc6_encode_kernel.cpp",
        f"{CMP_CORE_DIR}/shaders/bc7_encode_kernel.cpp",
        f"{CMP_CORE_DIR}/source/cmp_core.cpp",
        f"{CMP_DIR}/applications/_libs/cmp_math/cpu_extensions.cpp",
        f"{CMP_DIR}/applications/_libs/cmp_math/cmp_math_common.cpp",
    ]

    include_dirs = [
        f"{CMP_CORE_DIR}/shaders",
        f"{CMP_CORE_DIR}/source",
        f"{CMP_DIR}/applications/_libs/cmp_math",
    ]


class CompressonatorLib(BuildPart):
    sources = [
        f"{CMP_COMPRESSONATORLIB_DIR}/compress.cpp",
        f"{CMP_COMPRESSONATORLIB_DIR}/compressonator.cpp",
        *chain.from_iterable(
            glob(f"{CMP_COMPRESSONATORLIB_DIR}/{entry}")
            for entry in [
                # Lossy Compression
                "apc/*.cpp",
                "atc/*.cpp",
                "ati/*.cpp",
                "ati/*.c",
                "basis/*.cpp",
                "bc6h/*.cpp",
                "bc7/*.cpp",
                "block/*.cpp",
                "buffer/*.cpp",
                "dxt/*.cpp",
                "dxtc/*.cpp",
                "dxtc/*.c",
                "etc/*.cpp",
                "etc/etcpack/*.cpp",
                "etc/etcpack/*.cxx",
                "gt/*.cpp",
                # Astc
                "astc/*.cpp",
                "astc/arm/*.cpp",
            ]
        ),
        *glob(f"{CMP_COMPRESSONATORLIB_DIR}/common/*.cpp"),
        *glob(f"{CMP_DIR}/cmp_framework/common/*.cpp"),
        *glob(f"{CMP_DIR}/cmp_framework/common/half/*.cpp"),
        f"{CMP_DIR}/applications/_plugins/common/atiformats.cpp",
        f"{CMP_DIR}/applications/_plugins/common/format_conversion.cpp",
        f"{CMP_DIR}/applications/_plugins/common/codec_common.cpp",
        f"{CMP_DIR}/applications/_plugins/common/texture_utils.cpp",
    ]

    include_dirs = [
        CMP_COMPRESSONATORLIB_DIR,
        *[
            os.path.join(CMP_COMPRESSONATORLIB_DIR, d)
            for d in [
                # Lossy Compression
                "apc",
                "atc",
                "ati",
                "basis",
                "bc6h",
                "bc7",
                "block",
                "buffer",
                "dxt",
                "dxtc",
                "etc",
                "etc/etcpack",
                "gt",
                # Astc
                "astc",
                "astc/arm",
            ]
        ],
        # Common
        f"{CMP_COMPRESSONATORLIB_DIR}/common",
        f"{CMP_DIR}/cmp_framework/common",
        f"{CMP_DIR}/cmp_framework/common/half",
        f"{CMP_DIR}/applications/_plugins/common",
        f"{CMP_DIR}/applications/_libs/cmp_math",
    ]


class CustomBuildExt(build_ext):
    def build_simd_lib(self, ext: Exception) -> None:
        sse = f"{CMP_CORE_DIR}/source/core_simd_sse.cpp"
        avx = f"{CMP_CORE_DIR}/source/core_simd_avx.cpp"
        avx512 = f"{CMP_CORE_DIR}/source/core_simd_avx512.cpp"

        if self.compiler.compiler_type == "msvc":
            sse_arg = ""
            avx_arg = "/arch:AVX2"
            avx512_arg = "/arch:AVX512"
            extra_args = ["/std:c++14"]
        else:
            sse_arg = "-march=nehalem"  # unix
            avx_arg = "-march=haswell"
            avx512_arg = "-march=knl"
            extra_args = ["-std=c++14", "-fpermissive"]

        ext.extra_compile_args.extend(extra_args)
        macros = ext.define_macros[:]
        for undef in ext.undef_macros:
            macros.append((undef,))

        for src, arg in [
            (sse, sse_arg),
            (avx, avx_arg),
            (avx512, avx512_arg),
        ]:
            ext.extra_objects.extend(
                self.compiler.compile(
                    [src],
                    output_dir=self.build_temp,
                    macros=macros,
                    include_dirs=CompressonatorCore.include_dirs,
                    debug=self.debug,
                    extra_postargs=[*ext.extra_compile_args, arg],
                    depends=ext.depends,
                )
            )

    def build_extension(self, ext) -> None:
        self.build_simd_lib(ext)
        super().build_extension(ext)


setup(
    name="compressonator-py",
    packages=["compressonator_py"],
    package_data={"compressonator_py": ["*.py", "*.pyi", "py.typed"]},
    ext_modules=[
        Extension(
            name="compressonator_py._compressonator",
            sources=[
                *CompressonatorPy.sources,
                *CompressonatorCore.sources,
                *CompressonatorLib.sources,
            ],
            include_dirs=[
                *CompressonatorPy.include_dirs,
                *CompressonatorCore.include_dirs,
                *CompressonatorLib.include_dirs,
            ],
            language="c++",
            define_macros=[
                ("OPTION_BUILD_ASTC", "1"),
                # ("OPTION_BUILD_BROTLIG", "1")
            ],
        )
    ],
    cmdclass={"build_ext": CustomBuildExt},
)
