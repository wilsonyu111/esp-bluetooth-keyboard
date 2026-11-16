#!/usr/bin/env python3
"""Parse stratagem arrow codes from helldivers.wiki.gg Stratagems HTML."""

from __future__ import annotations

import json
import re
from pathlib import Path

HTML = Path.home() / "AppData" / "Local" / "Temp" / "hd_stratagems.html"
OUT = Path(__file__).resolve().parents[1] / "tools" / "missing_stratagem_codes.json"

ARROW = {
    "Stratagem Arrow Up.svg": "U",
    "Stratagem Arrow Down.svg": "D",
    "Stratagem Arrow Left.svg": "L",
    "Stratagem Arrow Right.svg": "R",
}

# Names we care about (wiki link title / text)
MISSING = [
    "Orbital Confetti",
    "Eagle Fly By",
    "Eagle Gas Airstrike",
    "AMW-20 Apex",
    "CQC-20 Breaching Hammer",
    "MGX-42 Bullet Storm",
    "PLAS-45 Epoch",
    "S-11 Speargun",
    "EAT-411 Leveller",
    "EAT-700 Expendable Napalm",
    "CQC-9 Defoliation Tool",
    "B/MD C4 Pack",
    "MS-11 Solo Silo",
    "B/FLAM-80 Cremator",
    "GL-28 Belt-Fed Grenade Launcher",
    "M-1000 Maxigun",
    "LAS-44 Nova",
    "40-K Meltagun",
    "AX/FLAM-75 Hot Dog",
    "LIFT-182 Warp Pack",
    "M-104 Incinerator FRV",
    "TD-110 Maelstrom",
    "M-103 Supply FRV",
    "EXO-55 Breakthrough Exosuit",
    "EXO-51 Lumberer Exosuit",
    "A/LAS-98 Laser Sentry",
    "A/GM-17 Gas Mortar Sentry",
    "Call In Super Destroyer",
    "Tectonic Drill",
    "Tactical Video Camera",
    "Cargo Container",
    "Reinforcement Pods",
    "Prospecting Drill",
    "Portable Comms Relay",
    "Dark Fluid Vessel",
    "Aquifer Drill",
    "Activate E-711 Extraction Drill",
    "Orbital Illumination Flare",
]

DIR_TO_CPP = {
    "U": "DIR_U",
    "D": "DIR_D",
    "L": "DIR_L",
    "R": "DIR_R",
}


def parse_rows(html: str) -> list[dict]:
    rows = []
    # Each data row with a stratcode-parent block
    for m in re.finditer(
        r'<tr>\s*<td>.*?</td>\s*<td>\s*<a[^>]*title="([^"]+)"[^>]*>.*?</a>\s*</td>\s*'
        r'<td>\s*<div class="stratcode-parent">(.*?)</div>\s*</td>',
        html,
        flags=re.I | re.S,
    ):
        name = m.group(1)
        code_html = m.group(2)
        dirs = []
        for alt in re.findall(r'alt="([^"]+)"', code_html):
            d = ARROW.get(alt)
            if d:
                dirs.append(d)
        rows.append({"name": name, "dirs": dirs, "code": "".join(dirs)})
    return rows


def to_cpp(name: str, category: str, icon: str, dirs: list[str]) -> str:
    dir_list = ", ".join(DIR_TO_CPP[d] for d in dirs)
    return (
        f'    {{"{name}", "{category}", "{icon}", {len(dirs)}, {{{dir_list}}}}},'
    )


# Suggested icon filenames from previous download step
ICON_MAP = {
    "Orbital Confetti": ("Offensive", "orb_confetti_icon.png"),
    "Eagle Fly By": ("Offensive", "eagle_flyby_icon.png"),
    "Eagle Gas Airstrike": ("Offensive", "eagle_gas_icon.png"),
    "AMW-20 Apex": ("Supply", "apex_icon.png"),
    "CQC-20 Breaching Hammer": ("Supply", "breach_hammer_icon.png"),
    "MGX-42 Bullet Storm": ("Supply", "bullet_storm_icon.png"),
    "PLAS-45 Epoch": ("Supply", "epoch_icon.png"),
    "S-11 Speargun": ("Supply", "speargun_icon.png"),
    "EAT-411 Leveller": ("Supply", "leveller_icon.png"),
    "EAT-700 Expendable Napalm": ("Supply", "exp_napalm_icon.png"),
    "CQC-9 Defoliation Tool": ("Supply", "defoliation_icon.png"),
    "B/MD C4 Pack": ("Supply", "c4_pack_icon.png"),
    "MS-11 Solo Silo": ("Supply", "solo_silo_icon.png"),
    "B/FLAM-80 Cremator": ("Supply", "cremator_icon.png"),
    "GL-28 Belt-Fed Grenade Launcher": ("Supply", "belt_gl_icon.png"),
    "M-1000 Maxigun": ("Supply", "maxigun_icon.png"),
    "LAS-44 Nova": ("Supply", "nova_icon.png"),
    "40-K Meltagun": ("Supply", "meltagun_icon.png"),
    "AX/FLAM-75 Hot Dog": ("Supply", "hot_dog_icon.png"),
    "LIFT-182 Warp Pack": ("Supply", "warp_pack_icon.png"),
    "M-104 Incinerator FRV": ("Supply", "incin_frv_icon.png"),
    "TD-110 Maelstrom": ("Supply", "maelstrom_icon.png"),
    "M-103 Supply FRV": ("Supply", "supply_frv_icon.png"),
    "EXO-55 Breakthrough Exosuit": ("Supply", "breakthrough_icon.png"),
    "EXO-51 Lumberer Exosuit": ("Supply", "lumberer_icon.png"),
    "A/LAS-98 Laser Sentry": ("Defensive", "laser_sentry_icon.png"),
    "A/GM-17 Gas Mortar Sentry": ("Defensive", "gas_mortar_icon.png"),
    "Call In Super Destroyer": ("Mission", "super_destroyer_icon.png"),
    "Tectonic Drill": ("Mission", "tectonic_drill_icon.png"),
    "Tactical Video Camera": ("Mission", "tactical_cam_icon.png"),
    "Cargo Container": ("Mission", "cargo_container_icon.png"),
    "Reinforcement Pods": ("Mission", "reinforce_pods_icon.png"),
    "Prospecting Drill": ("Mission", "prospect_drill_icon.png"),
    "Portable Comms Relay": ("Mission", "comms_relay_icon.png"),
    "Dark Fluid Vessel": ("Mission", "dark_fluid_icon.png"),
    "Aquifer Drill": ("Mission", "aquifer_drill_icon.png"),
    "Activate E-711 Extraction Drill": ("Mission", "e711_drill_icon.png"),
    "Orbital Illumination Flare": ("Mission", "orb_illum_flare_icon.png"),
}


def main() -> None:
    html = HTML.read_text(encoding="utf-8", errors="replace")
    rows = parse_rows(html)
    by_name = {r["name"]: r for r in rows}

    print(f"Parsed {len(rows)} stratagems with codes from wiki HTML\n")
    missing_out = []
    cpp_lines = []
    for name in MISSING:
        row = by_name.get(name)
        if not row:
            # fuzzy: title may differ slightly
            hits = [r for r in rows if name.lower() in r["name"].lower() or r["name"].lower() in name.lower()]
            if hits:
                row = hits[0]
            else:
                print(f"MISS  {name}: not found in HTML")
                missing_out.append({"name": name, "dirs": [], "status": "not_found"})
                continue
        dirs = row["dirs"]
        cat, icon = ICON_MAP.get(name, ("?", "?.png"))
        status = "ok" if dirs else "empty_code"
        arrows = " ".join(dirs) if dirs else "(none)"
        print(f"{status.upper():5} {name:40} {arrows}")
        entry = {
            "name": name,
            "wiki_name": row["name"],
            "dirs": dirs,
            "code": "".join(dirs),
            "category": cat,
            "icon": icon,
            "status": status,
        }
        missing_out.append(entry)
        if dirs and cat != "?":
            cpp_lines.append(to_cpp(name, cat, icon, dirs))

    OUT.write_text(json.dumps(missing_out, indent=2) + "\n", encoding="utf-8")
    cpp_path = OUT.with_suffix(".cpp.txt")
    cpp_path.write_text("\n".join(cpp_lines) + "\n", encoding="utf-8")
    print(f"\nWrote {OUT}")
    print(f"Wrote {cpp_path}")
    with_code = sum(1 for e in missing_out if e.get("dirs"))
    print(f"With codes: {with_code}/{len(MISSING)}")


if __name__ == "__main__":
    main()
