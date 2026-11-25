"""Verify Rev B CAD, rebuild CH552 firmware, and package the release."""
from pathlib import Path
import argparse
import hashlib
from io import BytesIO
import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
import xml.etree.ElementTree as ET

from firmware.verify_ws2812 import read_hex, symbols, trace, verify

sys.path.append(os.environ.get("FREECAD_LIB", r"E:\freecad\lib"))
import FreeCAD
import Part
import Mesh
from PIL import Image

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sdcc", type=Path, default=Path(r"E:\SDCC\bin\sdcc.exe"))
sdcc = parser.parse_args().sdcc.absolute()
powershell = shutil.which("pwsh") or shutil.which("powershell")
if powershell is None:
    raise SystemExit("PowerShell is required to run the firmware makefiles.")
pcb = root / "hardware/sl25_rev_b"
case = root / "enclosure/sl25_traffic_rev_b"
assert json.loads((pcb / "checks/verification.json").read_text())["passed"]
validation = json.loads((case / "checks/validation.json").read_text())
assert validation["passed"], validation["failures"]
assert validation["enclosure_size_mm"] == [30, 30, 40]
assert validation["hood_profile"]["top_projection_mm"] == 15
assert validation["clearances_mm"]["required_LED_tip_to_diffuser"] >= 13.5
assert validation["screw_alignment"].keys() == {"H1", "H2"}
result = {"stl_reopen": {}, "step_reopen": {}, "svg_cut_contours": {}, "previews": {}}
for path in sorted((case / "stl").glob("*.stl")):
    mesh = Mesh.Mesh(str(path))
    assert mesh.isSolid() and not mesh.hasNonManifolds(), path
    assert abs(mesh.BoundBox.ZMin) < 1e-5
    result["stl_reopen"][path.name] = "closed manifold; bed at Z=0"
for path in sorted((case / "cad").glob("*.step")) + [pcb / "mechanical/sl25.step"]:
    shape = Part.read(str(path))
    assert shape.isValid() and len(shape.Solids), path
    result["step_reopen"][path.relative_to(root).as_posix()] = {"valid": True, "solids": len(shape.Solids)}
source = Part.read(str(pcb / "mechanical/sl25.step"))
led = [s for s in source.Solids if s.BoundBox.ZMin >= 1.55]
assert len(led) == 1
center = led[0].BoundBox.Center
assert abs(center.x - 12.5) < 1e-6 and abs(center.y + 12.5) < 1e-6
result["LED_STEP_center_mm"] = [center.x, center.y]
with zipfile.ZipFile(case / "cad/SL25_Traffic_B.FCStd") as archive:
    gui = ET.fromstring(archive.read("GuiDocument.xml"))
    visible = [v.get("name") for v in gui.iter("ViewProvider") for p in v.findall("./Properties/Property")
               if p.get("name") == "Visibility" and p.find("Bool").get("value") == "true"]
    assert len(visible) == 10, visible
    result["fcstd_visible_objects"] = len(visible)
for path in (case / "cut_templates").glob("0*.svg"):
    svg = ET.parse(path).getroot()
    diameters = [2 * float(c.get("r")) for c in svg.findall("{http://www.w3.org/2000/svg}circle")]
    assert diameters == ([26, 24] if "foam" in path.name else [26]), path
    assert svg.get("width") == "28mm"
    result["svg_cut_contours"][path.name] = diameters
for path in list((case / "preview").glob("*.png")) + list((pcb / "preview").glob("*.png")):
    with Image.open(path) as image:
        assert len(image.getcolors(image.width * image.height)) > 20, path
        result["previews"][path.relative_to(root).as_posix()] = list(image.size)
result["passed"] = True
(case / "checks/export_verification.json").write_text(json.dumps(result, indent=2), encoding="utf-8")

gerbers = root / "hardware/SL25-RevB-gerbers.zip"
with zipfile.ZipFile(gerbers, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in (pcb / "fabrication").iterdir():
        if path.suffix in (".gtl", ".gbl", ".gts", ".gbs", ".gto", ".gbo", ".gtp", ".gbp", ".gm1", ".drl", ".gbrjob"):
            archive.write(path, path.name)
printable = root / "enclosure/SL25-Traffic-STL-RevB.zip"
with zipfile.ZipFile(printable, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in (case / "stl").glob("*.stl"):
        archive.write(path, "stl/" + path.name)
    for path in (case / "cut_templates").glob("*.svg"):
        archive.write(path, "cut_templates/" + path.name)
    archive.write(case / "README.md", "README.md")
    archive.write(case / "preview/dimensions.png", "dimensions.png")
    archive.write(case / "preview/hood_profile.png", "hood_profile.png")

files = [gerbers, printable, root / "README.md", root / "CURRENT_DESIGN.md",
         root / "package_rev_b.py", root / "启动状态灯.cmd"]
for folder in (pcb, case):
    files += [p for p in folder.rglob("*") if p.is_file()
              and p.suffix.lower() not in (".fcbak", ".pyc", ".kicad_prl", ".lck")
              and not any(part.startswith("package_staging_") for part in p.parts)
              and "__pycache__" not in p.parts and p.name not in ("SHA256SUMS.json", "stl.zip")
              and not p.name.startswith(("audit-", "connection-check"))]
files += [root / "hardware" / name for name in
          ("generate_board.py", "generate_rev_b.py", "route_board.py", "route_rev_b.py", "finish_board.py", "verify_rev_b.py")]
files += [root / "hardware/reference" / name for name in ("CH552.pdf", "USB4105.pdf", "WS2812B-V5.pdf")]
files += [root / "host" / name for name in
          ("README.md", "package.json", "package-lock.json", "codex_status_agent.mjs", "codex_status_agent.test.mjs")]
files += [root / "firmware" / name for name in
          ("FIRMWARE_DIAGNOSIS.md", "common/ws2812_ch552.h", "test_firmware_logic.py", "verify_ws2812.py")]

targets = [
    ("ch552_codex_status", "build", "sl25_codex_status", (), True),
    ("ch552_led_diagnostic", "build", "sl25_led_diagnostic", (), True),
    ("ch552_led_diagnostic", "build", "sl25_gpio_probe", ("-GpioProbe",), False),
    ("ch552_ws2812_demo", "build-white", "sl25_ws2812_white", (), False),
]
for project in {target[0] for target in targets}:
    files += [root / "firmware" / project / name for name in ("main.c", "Makefile.ps1", "README.md")]

complete = root / "SL25-RevB-complete.zip"
checksum = case / "SHA256SUMS.json"
with tempfile.TemporaryDirectory(prefix=".sl25-package-", dir=root) as directory:
    stage = Path(directory)
    entries = {path.relative_to(root).as_posix(): path for path in files}
    report = {"sdcc": subprocess.check_output([str(sdcc), "--version"], text=True).splitlines()[0],
              "firmware": {}}
    for project, folder, stem, switches, verify_timing in targets:
        source = root / "firmware" / project
        release_path = Path("firmware") / project / folder
        out = stage / release_path
        subprocess.run([powershell, "-NoProfile", "-File", str(source / "Makefile.ps1"),
                        "-Sdcc", str(sdcc), "-OutDir", str(out), *switches], check=True, cwd=source)
        binary = out / (stem + ".hex")
        memory = read_hex(binary)
        assert 0 in memory, f"Missing reset vector in {binary}"
        details = {"flash_bytes": len(memory), "highest_address": hex(max(memory)),
                   "intel_hex_checksums": "passed"}
        if verify_timing:
            timing = verify(binary)
            del timing["file"]
            details["ws2812_instruction_timing"] = timing
            listing = binary.with_suffix(".rst")
            entries[listing.relative_to(stage).as_posix()] = listing
        elif project == "ch552_ws2812_demo":
            listing = binary.with_suffix(".rst")
            edges, _, _ = trace(memory, symbols(listing)["_ws_white"])
            low_ns = {round((rise[0] - fall[0]) * 1000 / 24, 2)
                      for fall, rise in zip(edges[1::2], edges[2::2])}
            details["ws2812_instruction_timing"] = {
                "result": "Experimental full-white demo",
                "T1L_ns_at_24MHz": sorted(low_ns), "RGB_diagnostic_T1L_min_ns": 580,
            }
        name = binary.relative_to(stage).as_posix()
        report["firmware"][name] = details
        entries[name] = binary

    report_path = stage / "firmware_checks.json"
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    entries["firmware/firmware_checks.json"] = report_path
    hashes = {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in sorted(entries.items())}
    manifest = json.dumps(hashes, indent=2)
    bundle = BytesIO()
    with zipfile.ZipFile(bundle, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, path in sorted(entries.items()):
            archive.write(path, "SL25-RevB/" + name)
        archive.writestr("SL25-RevB/" + checksum.relative_to(root).as_posix(), manifest)
    with zipfile.ZipFile(bundle) as archive:
        assert archive.testzip() is None
        for name, expected in hashes.items():
            assert hashlib.sha256(archive.read("SL25-RevB/" + name)).hexdigest() == expected
    with complete.open("wb") as output:
        output.write(bundle.getbuffer())
    checksum.write_text(manifest, encoding="utf-8")

for path in (complete, gerbers, printable):
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None
    print(str(path), path.stat().st_size, "bytes")
print("All exported files reopened and validated.")
