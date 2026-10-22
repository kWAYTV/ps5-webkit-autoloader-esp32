#!/usr/bin/env python3
"""Stage srbraboo/ps5-webkit-autoloader pages into host/packed.

Keeps the ESP32 split-AppCache installer shells (the chip cannot inject
manifest= from ?exploit= like the PC host). Replaces the chain HTML and
externalizes kexp/elfldr/autoloader.elf so both exploits fit in 4 MB flash.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_WEB = ROOT / "webkit-autoloader"
DEFAULT_OUT = ROOT / "host" / "packed"


def run(cmd: list[str], cwd: Path) -> None:
    print("+", " ".join(cmd), flush=True)
    subprocess.run(cmd, cwd=cwd, check=True)


def read_version(web: Path) -> tuple[str, str]:
    printed = subprocess.run(
        [sys.executable, str(web / "tools" / "gen_version.py"), "--print"],
        cwd=web,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    header = (web / "include" / "wkali.h").read_text(encoding="utf-8")
    match = re.search(r'#define\s+WKAL_VERSION\s+"([^"]+)"', header)
    base = match.group(1) if match else printed.split("-")[0]
    return printed, base


def rewrite(text: str, version: str, app_dir: str, build_time: str) -> str:
    text = text.replace("github.com/itsPLK/ps5-webkit-autoloader",
                        "github.com/srbraboo/ps5-webkit-autoloader")
    text = re.sub(r"v0\.6\.1 by PLK \(built [^)]+\)",
                  f"v{version} (built {build_time})", text)
    text = re.sub(r"var version = '[^']+';", f"var version = '{version}';", text)
    text = re.sub(r"var base = '/app/[^']+/';", f"var base = '/app/{app_dir}/';", text)
    text = re.sub(r"/app/0\.6\.1/", f"/app/{app_dir}/", text)
    return text


def write_manifest(path: Path, version: str, build_time: str, app_dir: str,
                   chain: str, cache_page: str) -> None:
    html = f"{chain}.html"
    path.write_text(
        "CACHE MANIFEST\n"
        f"# WebKit Autoloader v{version} (built {build_time}) - srbraboo pack\n"
        "\n"
        "CACHE:\n"
        f"/app/{app_dir}/{html}\n"
        f"/{cache_page}\n"
        "/logo.svg\n"
        "/app/index.html\n"
        f"/app/index.html?force={chain}\n"
        f"/app/{app_dir}/payloads/autoloader.elf\n"
        f"/app/{app_dir}/shared/elfldr-ps5.elf\n"
        f"/app/{app_dir}/shared/kexp-ps5.bin\n"
        f"/app/{app_dir}/__complete__\n"
        "\n"
        "FALLBACK:\n"
        "/app/ /app/index.html\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--web", type=Path, default=DEFAULT_WEB)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()
    web = args.web.resolve()
    out = args.out.resolve()
    if not (web / "Makefile").is_file():
        raise SystemExit(f"srbraboo tree not found: {web}")

    if not args.skip_build:
        run(["git", "submodule", "update", "--init",
             "third_party/ps5-elfldr",
             "third_party/ps5-unified-autoloader",
             "third_party/ps5-webkit-remote-loader"], cwd=web)
        # download_deps.sh uses `git describe --tags`. Actions submodule
        # checkouts omit tags, so describe falls back to 915a65e and 404s.
        run(["git", "fetch", "--tags", "--force", "--no-recurse-submodules"],
            cwd=web / "third_party" / "ps5-unified-autoloader")
        if os.name == "nt":
            bash = Path(r"C:\Program Files\Git\bin\bash.exe")
            posix = "/" + web.drive[0].lower() + web.as_posix()[2:]
            run([
                str(bash), "-lc",
                "export PATH='/c/Users/kWAY/AppData/Local/esp32-tools:/usr/bin:/bin:/mingw64/bin:$PATH'; "
                f"export HOME=/c/Users/kWAY; cd '{posix}' && "
                "bash ./tools/download_deps.sh && make page",
            ], cwd=web)
        else:
            run(["bash", "tools/download_deps.sh"], cwd=web)
            run(["make", "page"], cwd=web)

    version, base = read_version(web)
    app_dir = base
    build_time = subprocess.run(
        ["git", "-C", str(web), "log", "-1", "--format=%cI"],
        check=True, capture_output=True, text=True,
    ).stdout.strip() or "unknown"
    pages = {
        "poops": web / "build" / "autoloader" / "poops.html",
        "relapse": web / "build" / "autoloader" / "relapse.html",
    }
    for name, path in pages.items():
        if not path.is_file():
            raise SystemExit(f"missing {path} (run make page)")

    if out.exists():
        shutil.rmtree(out)
    version_dir = out / "app" / app_dir
    version_dir.mkdir(parents=True)

    for name, src in pages.items():
        shutil.copy2(src, version_dir / f"{name}.html")
    (version_dir / "__complete__").write_text(app_dir + "\n", encoding="utf-8")

    logo = web / "frontend" / "installer-page" / "logo.svg"
    if not logo.is_file():
        logo = web / "assets" / "icon.svg"
    shutil.copy2(logo, out / "logo.svg")

    shells = ROOT / "host" / "installer"
    if not shells.is_dir():
        shells = ROOT / "host" / "packed"
    for name in ("index.html", "slopkit_cache.html", "relapse_cache.html"):
        text = (shells / name).read_text(encoding="utf-8")
        (out / name).write_text(
            rewrite(text, app_dir, app_dir, build_time), encoding="utf-8")
    pointer = web / "frontend" / "pointer" / "index.html"
    pointer_text = pointer.read_text(encoding="utf-8")
    pointer_text = pointer_text.replace("[[VERSION_PLACEHOLDER]]", app_dir)
    pointer_text = pointer_text.replace("[[APP_DIR_PLACEHOLDER]]", app_dir)
    pointer_text = pointer_text.replace("[[BUILD_TIME_PLACEHOLDER]]", build_time)
    # ESP32 installer records the choice and jumps with ?force=
    pointer_text = pointer_text.replace(
        "  var chain = null;\n"
        "  try { chain = window.localStorage.getItem('wkal_exploit'); } catch (error) { chain = null; }\n",
        "  var forced = new URLSearchParams(location.search).get('force');\n"
        "  var chain = forced;\n"
        "  if (chain !== 'poops' && chain !== 'relapse') {\n"
        "    try { chain = window.localStorage.getItem('wkal_exploit'); } catch (error) { chain = null; }\n"
        "  }\n",
    )
    (out / "app" / "index.html").write_text(pointer_text, encoding="utf-8")

    write_manifest(out / "slopkit.appcache", version, build_time, app_dir,
                   "poops", "slopkit_cache.html")
    write_manifest(out / "relapse.appcache", version, build_time, app_dir,
                   "relapse", "relapse_cache.html")
    (out / "cache.appcache").write_text(
        "CACHE MANIFEST\n# retire previous cache\nNETWORK:\n*\n",
        encoding="utf-8",
    )

    run([sys.executable, str(ROOT / "pack" / "externalize_autoloader.py"), str(out)], cwd=ROOT)
    print(f"Packed {out} version={version} app_dir={app_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
