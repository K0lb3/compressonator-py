from compressonator_py import CMP_ConvertTexture, CMP_Format, CMP_Texture


# def test_conversion():
#     data = bytes(128 * 128 * 4)
#     tex = CMP_Texture(128, 128, 0, CMP_Format.RGBA_8888, pData=data)
#     tex2 = CMP_Texture(128, 128, 0, format=CMP_Format.BC7)
#     tex3 = CMP_Texture(128, 128, 0, format=CMP_Format.RGBA_8888)

#     CMP_ConvertTexture(tex, tex2)
#     CMP_ConvertTexture(tex2, tex3)

#     assert tex.pData != tex2.pData
#     assert tex.pData == tex3.pData

def test_texture_creation():
    data = bytes(256 * 256 * 4)
    tex = CMP_Texture(256, 256, 0, CMP_Format.RGBA_8888, pData=data)
    tex2 = CMP_Texture(256, 256, 0, format=CMP_Format.BC7)

    CMP_ConvertTexture(tex, tex2)

    tex3 = CMP_Texture(256, 256, 0, format=CMP_Format.RGBA_8888)
    CMP_ConvertTexture(tex2, tex3)

    assert tex.pData != tex2.pData
    assert tex.pData == tex3.pData
    