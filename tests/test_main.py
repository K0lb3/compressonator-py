import gc

import pytest

from compressonator_py import CMP_Format, CMP_Texture

IGNORED_FORMATS = set(
    [
        CMP_Format.Unknown,
        CMP_Format.MAX,
    ]
)

UNKNOWN_SIZE_FORMATS = set(
    [
        CMP_Format.BROTLIG,
        CMP_Format.APC,
        CMP_Format.GTC,
        CMP_Format.PVRTC,
        CMP_Format.BASIS,
    ]
)

PROBLEMATIC_FORMATS = set(
    # converting fails
    [CMP_Format.ARGB_2101010, CMP_Format.RGBA_1010102]
)

TEST_FORMATS = set(CMP_Format.__members__.values())
TEST_FORMATS -= IGNORED_FORMATS | UNKNOWN_SIZE_FORMATS | PROBLEMATIC_FORMATS


@pytest.mark.parametrize(
    "cmp_format",
    TEST_FORMATS,
    ids=lambda cmp_format: cmp_format.name,
)
def test_texture_creation(cmp_format):
    # empty creation
    tex = CMP_Texture(256, 256, 0, cmp_format)
    # with data
    data = bytes(tex.dwDataSize)
    tex2 = CMP_Texture(256, 256, 0, cmp_format, data)
    # cmp check
    assert tex == tex2
    # deletion
    del tex
    del tex2
    gc.collect()


@pytest.mark.parametrize(
    "cmp_format",
    TEST_FORMATS,
    ids=lambda cmp_format: cmp_format.name,
)
def test_compression(cmp_format):
    data = bytes(256 * 256 * 4)
    tex = CMP_Texture(256, 256, 0, CMP_Format.RGBA_8888, dwData=data)
    tex2 = CMP_Texture(256, 256, 0, format=cmp_format)
    print(f"Converting RGBA_8888 to {cmp_format.name}...")

    try:
        tex.convert(tex2)
    except ValueError as e:
        if "The source format is not a supported format." in str(e):
            pytest.skip(f"Skipping conversion to {cmp_format.name} due to unsupported source format.")


@pytest.mark.parametrize(
    "cmp_format",
    UNKNOWN_SIZE_FORMATS,
    ids=lambda cmp_format: cmp_format.name,
)
def test_format_size_check(cmp_format):
    try:
        CMP_Texture(256, 256, 0, cmp_format)
        pytest.fail(f"Created a texture of type {cmp_format} without data!")
    except ValueError:
        pass

    try:
        CMP_Texture(256, 256, 0, cmp_format, dwData=b"")
        pytest.fail(f"Created a texture of type {cmp_format} with empty data!")
    except ValueError:
        pass

    try:
        CMP_Texture(256, 256, 0, cmp_format, dwData=b"1")
    except Exception as e:
        pytest.fail(f"Failed creating a texture of type {cmp_format} with given data: {e}")
