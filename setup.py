from __future__ import annotations

import os
import sys
from itertools import chain
from typing import TYPE_CHECKING, ClassVar, Sequence

import nanobind as nb
from setuptools import Extension, setup
from setuptools.command.bdist_wheel import bdist_wheel
from setuptools.command.build_ext import build_ext

if TYPE_CHECKING:
    from distutils.ccompiler import CCompiler

    _Macro = tuple[str] | tuple[str, str | None]

LOCAL = ""
CMP_DIR = os.path.join(LOCAL, "compressonator")
CMP_CORE_DIR = os.path.join(CMP_DIR, "cmp_core")
CMP_COMPRESSONATORLIB_DIR = os.path.join(CMP_DIR, "cmp_compressonatorlib")

USE_LIMITED_API = sys.version_info >= (3, 12) and os.getenv("CIBUILDWHEEL") is not None


def glob(pattern: str) -> list[str]:
    # glob.glob only added root dir in 3.10
    dir, ext = os.path.split(pattern)
    assert ext.startswith("*"), "Only simple globbing supported"
    ext = ext[1:]
    return [os.path.join(dir, f) for f in os.listdir(dir) if f.endswith(ext)]


class BuildPart:
    sources: ClassVar[list[str]]
    include_dirs: ClassVar[list[str]]


class CompressonatorPy(BuildPart):
    sources = [
        "compressonator_pyc/nano.cpp",
        os.path.join(nb.source_dir(), "nb_combined.cpp"),
    ]
    include_dirs = [
        nb.include_dir(),
        os.path.join(
            os.path.dirname(nb.source_dir()),
            "ext",
            "robin_map",
            "include",
        ),
    ]


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


class CompressonatorCoreSIMD(BuildPart):
    stub = "compressonator_pyc/core_simd_stub.cpp"
    sse = f"{CMP_CORE_DIR}/source/core_simd_sse.cpp"
    avx = f"{CMP_CORE_DIR}/source/core_simd_avx.cpp"
    avx512 = f"{CMP_CORE_DIR}/source/core_simd_avx512.cpp"

    sources = [stub, sse, avx, avx512]
    include_dirs = [f"{CMP_CORE_DIR}/source"]


def wrap_compile(compiler: "CCompiler", cpp_flags: list[str]):
    old_compile = compiler.compile

    def compile_by_language(
        sources: Sequence[str | os.PathLike[str]],
        output_dir: str | None = None,
        macros: list[_Macro] | None = None,
        include_dirs: list[str] | tuple[str, ...] | None = None,
        debug: bool = False,
        extra_preargs: list[str] | None = None,
        extra_postargs: list[str] | None = None,
        depends: list[str] | tuple[str, ...] | None = None,
    ):
        c_sources = [s for s in sources if str(s).endswith(".c")]
        cpp_sources = [s for s in sources if str(s).endswith((".cpp", ".cxx", ".cc"))]

        objects = []
        # 1. Compile C files (without -std=c++17)
        if c_sources:
            objects.extend(
                old_compile(
                    c_sources,
                    output_dir=output_dir,
                    macros=macros,
                    include_dirs=include_dirs,
                    debug=debug,
                    extra_preargs=extra_preargs,
                    extra_postargs=extra_postargs,
                    depends=depends,
                )
            )

        # 2. Compile C++ files (with -std=c++17)
        if cpp_sources:
            cpp_postargs = (extra_postargs or []) + cpp_flags
            objects.extend(
                old_compile(
                    cpp_sources,
                    output_dir=output_dir,
                    macros=macros,
                    include_dirs=include_dirs,
                    debug=debug,
                    extra_preargs=extra_preargs,
                    extra_postargs=cpp_postargs,
                    depends=depends,
                )
            )
        return objects

    compiler.compile = compile_by_language


class CustomBuildExt(build_ext):
    def build_simd_lib(self, ext: Extension) -> None:
        if self.compiler.compiler_type == "msvc":
            sse_args = ["/arch:SSE4.1"]
            avx_args = ["/arch:AVX2"]
            avx512_args = ["/arch:AVX512"]
        else:
            sse_args = ["-msse4.1"]
            avx_args = ["-mavx2"]
            avx512_args = ["-mavx512f"]  # -mevex512"

        macros = ext.define_macros[:]
        for undef in ext.undef_macros:
            macros.append((undef,))

        for src, args in [
            (CompressonatorCoreSIMD.sse, sse_args),
            (CompressonatorCoreSIMD.avx, avx_args),
            (CompressonatorCoreSIMD.avx512, avx512_args),
        ]:
            ext.extra_objects.extend(
                self.compiler.compile(
                    [src],
                    output_dir=self.build_temp,
                    macros=macros,
                    include_dirs=CompressonatorCore.include_dirs,
                    debug=self.debug,
                    extra_postargs=[*ext.extra_compile_args, *args],
                    depends=ext.depends,
                )
            )

    def build_extension(self, ext: Extension) -> None:
        # remove simd sources, we will build them conditionally below
        # only added directly so they get included in sdist
        for src in CompressonatorCoreSIMD.sources:
            ext.sources.remove(src)

        if self.compiler.compiler_type == "msvc":
            ext.extra_compile_args.extend(["/w", "-D_WIN32"])
            ext.extra_link_args.extend(["/INCREMENTAL:NO"])

            if self.plat_name.lower().endswith("arm64"):
                # no __cpuindex on arm64 msvc
                ext.extra_compile_args.append(
                    "/FIcompressonator_pyc/fixes/msvc_arm64.hpp",
                )

            cpp_flags = ["/std:c++17"]
        else:
            ext.extra_compile_args.extend(
                [
                    "-fpermissive",
                    "-Wno-narrowing",
                    "--no-warnings",
                    "-fPIC",
                    "-Wno-write-strings",
                ]
            )

            cpp_flags = [
                "-std=c++17",
                # global define fix
                "-include",
                "locale",
                "-include",
                "vector",
                "-include",
                "algorithm",
                "-Uglobal",
                "-U__global",
                # musl fix
                "-include",
                "compressonator_pyc/fixes/musl_null.hpp",
            ]

        if sys.platform == "darwin":
            ext.extra_compile_args.append("-mmacosx-version-min=10.15")

        wrap_compile(self.compiler, cpp_flags)

        if self.plat_name.endswith(("amd64", "x86_64")):
            # build simd lib
            self.build_simd_lib(ext)
        else:
            ext.sources.append(CompressonatorCoreSIMD.stub)

        super().build_extension(ext)


class bdist_wheel_abi3(bdist_wheel):
    def get_tag(self):
        python, abi, plat = super().get_tag()

        if python.startswith("cp") and USE_LIMITED_API:
            # on CPython, our wheels are abi3 and compatible back to 3.12
            return "cp312", "abi3", plat

        return python, abi, plat


optional_macros = []
if USE_LIMITED_API:
    optional_macros.append(("Py_LIMITED_API", "0x030C0000"))

setup(
    name="compressonator-py",
    packages=["compressonator_py"],
    package_data={"compressonator_py": ["*.py", "*.pyi", "py.typed"]},
    include_package_data=True,
    ext_modules=[
        Extension(
            name="compressonator_py._compressonator",
            sources=[
                *CompressonatorPy.sources,
                *CompressonatorCore.sources,
                *CompressonatorLib.sources,
                # including all to goat sdist into including them
                *CompressonatorCoreSIMD.sources,
            ],
            include_dirs=[
                *CompressonatorPy.include_dirs,
                *CompressonatorCore.include_dirs,
                *CompressonatorLib.include_dirs,
            ],
            language="c++",
            define_macros=[
                ("OPTION_BUILD_ASTC", "1"),
                # ("USE_LOSSLESS_COMPRESSION", "1"), # brotli
                # ("USE_APC", "1"), # encoder only, decoder is external, Windows only
                # ("USE_GTC", "1"), # external, R&D, Windows only
                # ("USE_BASIS", "1"), # external, Windows only
                # limited api
                *optional_macros,
            ],
            py_limited_api=USE_LIMITED_API,
        )
    ],
    cmdclass={"build_ext": CustomBuildExt, "bdist_wheel": bdist_wheel_abi3},
)
