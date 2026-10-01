import json
import sys
import zipfile

archive_path, image_path, hex_path = sys.argv[1:4]
has_save = len(sys.argv) == 4
with zipfile.ZipFile(archive_path) as archive:
    expected = {"info.json", "interp.hex", "fxdata.bin"}
    if has_save:
        expected.add("fxsave.bin")
    assert set(archive.namelist()) == expected
    assert archive.testzip() is None
    info = json.loads(archive.read("info.json"))
    assert info["schemaVersion"] == 3
    if has_save:
        assert info["title"] == 'A "title"'
        assert info["description"] == "Line one"
        assert info["author"] == "Tester"
        assert info["genre"] == "Demo"
        assert info["version"] == "2.0"
    else:
        assert info["title"] == "minimal"
        assert info["author"] == "Unknown"
        assert info["version"] == "1.0"
    binary = {
        "filename": "interp.hex",
        "flashdata": "fxdata.bin",
        "device": "ArduboyFX",
    }
    if has_save:
        binary["flashsave"] = "fxsave.bin"
    assert info["binaries"] == [binary]
    assert archive.read("interp.hex") == open(hex_path, "rb").read()
    assert archive.read("fxdata.bin") == open(image_path, "rb").read()
    if has_save:
        assert archive.read("fxsave.bin") == b"\xff" * 4096
