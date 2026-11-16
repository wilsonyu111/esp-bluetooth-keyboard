#!/usr/bin/env python3
"""Download missing Helldivers stratagem icons as LittleFS-safe PNGs.

Wiki hosts SVG icon backgrounds; we rasterize to 256x256 PNG to match
existing assets. Basename length is capped at 27 (project convention /
older mklittlefs path safety with /hd-icons/ prefix).
"""

from __future__ import annotations

import json
import re
import tempfile
import urllib.request
from pathlib import Path

from PIL import Image
from reportlab.graphics import renderPM
from svglib.svglib import svg2rlg

ROOT = Path(__file__).resolve().parents[1]
ICON_DIR = ROOT / "data" / "hd-icons"
MANIFEST = ICON_DIR / "manifest.json"
WIKI_BASE = "https://helldivers.wiki.gg"
MAX_NAME_LEN = 27
TARGET_SIZE = 256

# local png name -> absolute svg url on helldivers.wiki.gg
ICONS: list[tuple[str, str, str]] = [
    # (local_name, wiki_label, svg_path)
    ("orb_confetti_icon.png", "Orbital Confetti (fallback)", "/images/Orbital_Strike_Stratagem_Fallback_Icon_Background.svg"),
    ("eagle_flyby_icon.png", "Eagle Fly By (fallback)", "/images/Eagle_Stratagem_Fallback_Icon_Background.svg"),
    ("eagle_gas_icon.png", "Eagle Gas Airstrike", "/images/Eagle_Gas_Airstrike_Stratagem_Icon_Background.svg"),
    ("apex_icon.png", "AMW-20 Apex", "/images/Apex_Stratagem_Icon_Background.svg"),
    ("breach_hammer_icon.png", "CQC-20 Breaching Hammer", "/images/Breaching_Hammer_Stratagem_Icon_Background.svg"),
    ("bullet_storm_icon.png", "MGX-42 Bullet Storm", "/images/Bullet_Storm_Stratagem_Icon_Background.svg"),
    ("epoch_icon.png", "PLAS-45 Epoch", "/images/Epoch_Stratagem_Icon_Background.svg"),
    ("speargun_icon.png", "S-11 Speargun", "/images/Speargun_Stratagem_Icon_Background.svg"),
    ("leveller_icon.png", "EAT-411 Leveller", "/images/Leveller_Stratagem_Icon_Background.svg"),
    ("exp_napalm_icon.png", "EAT-700 Expendable Napalm", "/images/Expendable_Napalm_Stratagem_Icon_Background.svg"),
    ("defoliation_icon.png", "CQC-9 Defoliation Tool", "/images/Defoliation_Tool_Stratagem_Icon_Background.svg"),
    ("c4_pack_icon.png", "B/MD C4 Pack", "/images/C4_Pack_Stratagem_Icon_Background.svg"),
    ("solo_silo_icon.png", "MS-11 Solo Silo", "/images/Solo_Silo_Stratagem_Icon_Background.svg"),
    ("cremator_icon.png", "B/FLAM-80 Cremator", "/images/Cremator_Stratagem_Icon_Background.svg"),
    ("belt_gl_icon.png", "GL-28 Belt-Fed Grenade Launcher", "/images/Belt-Fed_Grenade_Launcher_Stratagem_Icon_Background.svg"),
    ("maxigun_icon.png", "M-1000 Maxigun", "/images/Maxigun_Stratagem_Icon_Background.svg"),
    ("nova_icon.png", "LAS-44 Nova", "/images/Nova_Stratagem_Icon_Background.svg"),
    ("meltagun_icon.png", "40-K Meltagun", "/images/Meltagun_Stratagem_Icon_Background.svg"),
    ("hot_dog_icon.png", "AX/FLAM-75 Hot Dog", "/images/Hot_Dog_Stratagem_Icon_Background.svg"),
    ("warp_pack_icon.png", "LIFT-182 Warp Pack", "/images/Warp_Pack_Stratagem_Icon_Background.svg"),
    ("incin_frv_icon.png", "M-104 Incinerator FRV", "/images/Incinerator_FRV_Stratagem_Icon_Background.svg"),
    ("maelstrom_icon.png", "TD-110 Maelstrom", "/images/Maelstrom_Stratagem_Icon_Background.svg"),
    ("supply_frv_icon.png", "M-103 Supply FRV", "/images/Supply_FRV_Stratagem_Icon_Background.svg"),
    ("breakthrough_icon.png", "EXO-55 Breakthrough Exosuit", "/images/Breakthrough_Exosuit_Stratagem_Icon_Background.svg"),
    ("lumberer_icon.png", "EXO-51 Lumberer Exosuit", "/images/Lumberer_Exosuit_Stratagem_Icon_Background.svg"),
    ("laser_sentry_icon.png", "A/LAS-98 Laser Sentry", "/images/Laser_Sentry_Stratagem_Icon_Background.svg"),
    ("gas_mortar_icon.png", "A/GM-17 Gas Mortar Sentry", "/images/Gas_Mortar_Sentry_Stratagem_Icon_Background.svg"),
    ("super_destroyer_icon.png", "Call In Super Destroyer", "/images/Call_In_Super_Destroyer_Stratagem_Icon_Background.svg"),
    ("tectonic_drill_icon.png", "Tectonic Drill", "/images/Drill_Stratagem_Icon_Background.svg"),
    ("tactical_cam_icon.png", "Tactical Video Camera", "/images/Tactical_Video_Camera_Stratagem_Icon_Background.svg"),
    ("cargo_container_icon.png", "Cargo Container", "/images/Cargo_Container_Stratagem_Icon_Background.svg"),
    ("reinforce_pods_icon.png", "Reinforcement Pods", "/images/Reinforcement_Pods_TO_Icon.svg"),
    ("prospect_drill_icon.png", "Prospecting Drill", "/images/Drill_Stratagem_Icon_Background.svg"),
    ("comms_relay_icon.png", "Portable Comms Relay", "/images/Cargo_Container_Stratagem_Icon_Background.svg"),
    ("dark_fluid_icon.png", "Dark Fluid Vessel", "/images/Dark_Fluid_Vessel_Stratagem_Icon_Background.svg"),
    ("aquifer_drill_icon.png", "Aquifer Drill", "/images/Drill_Stratagem_Icon_Background.svg"),
    ("e711_drill_icon.png", "Activate E-711 Extraction Drill", "/images/Drill_Stratagem_Icon_Background.svg"),
    ("orb_illum_flare_icon.png", "Orbital Illumination Flare", "/images/Orbital_Illumination_Flare_Stratagem_Icon_Background.svg"),
]


def assert_name_ok(name: str) -> None:
    if len(name) > MAX_NAME_LEN:
        raise SystemExit(f"Name too long ({len(name)}>{MAX_NAME_LEN}): {name}")
    if not re.fullmatch(r"[a-z0-9._-]+\.png", name):
        raise SystemExit(f"Bad name chars: {name}")


def fetch(url: str) -> bytes:
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "esp32-bluetooth-keyboard-icon-sync/1.0"},
    )
    with urllib.request.urlopen(req, timeout=60) as resp:
        data = resp.read()
    if len(data) < 100:
        raise RuntimeError(f"Suspiciously small download ({len(data)} bytes): {url}")
    return data


def svg_to_png(svg_bytes: bytes, dest: Path) -> None:
    with tempfile.TemporaryDirectory() as tmp:
        svg_path = Path(tmp) / "icon.svg"
        png_path = Path(tmp) / "icon.png"
        # Strip XML/affinity quirks that confuse svglib when present.
        text = svg_bytes.decode("utf-8", errors="replace")
        if text.lstrip().startswith("<?xml"):
            # keep as-is; svglib handles xml
            svg_path.write_text(text, encoding="utf-8")
        else:
            svg_path.write_bytes(svg_bytes)

        drawing = svg2rlg(str(svg_path))
        if drawing is None:
            raise RuntimeError("svglib failed to parse SVG")

        scale = TARGET_SIZE / max(float(drawing.width), float(drawing.height), 1.0)
        drawing.width = TARGET_SIZE
        drawing.height = TARGET_SIZE
        drawing.scale(scale, scale)

        # Opaque render; wiki SVGs already include the colored plate.
        renderPM.drawToFile(drawing, str(png_path), fmt="PNG", dpi=72)
        im = Image.open(png_path).convert("RGBA")
        if im.size != (TARGET_SIZE, TARGET_SIZE):
            im = im.resize((TARGET_SIZE, TARGET_SIZE), Image.Resampling.LANCZOS)
        im.save(dest, format="PNG", optimize=True)


def main() -> None:
    for name, _, _ in ICONS:
        assert_name_ok(name)

    ICON_DIR.mkdir(parents=True, exist_ok=True)

    # Clean prior bad SVG-saved-as-png downloads for these names.
    for name, _, _ in ICONS:
        path = ICON_DIR / name
        if path.exists():
            head = path.read_bytes()[:200].lstrip()
            if head.startswith(b"<") or head.startswith(b"<?xml"):
                path.unlink()

    results = []
    ok = 0
    fail = 0
    for local_name, label, svg_path in ICONS:
        url = WIKI_BASE + svg_path
        dest = ICON_DIR / local_name
        try:
            svg_bytes = fetch(url)
            svg_to_png(svg_bytes, dest)
            size = dest.stat().st_size
            # Validate PNG
            with Image.open(dest) as im:
                assert im.format == "PNG"
                assert im.size == (TARGET_SIZE, TARGET_SIZE)
            print(f"OK  {local_name} ({len(local_name)} chars, {size} B) <- {label}")
            ok += 1
            results.append(
                {
                    "wiki": label,
                    "file": f"hd-icons/{local_name}",
                    "bytes": size,
                    "url": url,
                    "status": "ok",
                }
            )
        except Exception as e:
            print(f"FAIL {local_name}: {e}")
            fail += 1
            results.append(
                {
                    "wiki": label,
                    "file": f"hd-icons/{local_name}",
                    "url": url,
                    "status": f"error: {e}",
                }
            )

    manifest = {"source": "https://helldivers.wiki.gg/wiki/Stratagems", "images": []}
    if MANIFEST.exists():
        try:
            manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            pass
    if "images" not in manifest:
        manifest["images"] = []

    existing = {img.get("file"): i for i, img in enumerate(manifest["images"])}
    for item in results:
        if item.get("status") != "ok":
            continue
        entry = {
            "wiki": item["wiki"],
            "file": item["file"],
            "bytes": item["bytes"],
            "url": item["url"],
        }
        if item["file"] in existing:
            manifest["images"][existing[item["file"]]] = entry
        else:
            existing[item["file"]] = len(manifest["images"])
            manifest["images"].append(entry)

    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    # Cleanup temp artifacts
    for junk in ("apex_tmp.svg", "apex_test.png", "_wiki_icon_labels.txt"):
        p = ICON_DIR / junk
        if p.exists():
            p.unlink()

    print(f"\nDone: {ok} png icons, {fail} failed")
    print(f"Max filename length: {max(len(n) for n, _, _ in ICONS)} (limit {MAX_NAME_LEN})")


if __name__ == "__main__":
    main()
