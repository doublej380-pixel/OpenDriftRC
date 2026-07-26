#!/usr/bin/env python3
"""Build merged OpenDrift factory images and web-installer manifests."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


BOARDS = (
    {
        "env": "waveshare_amoled_164",
        "name": "OpenDrift AMOLED 1.64",
        "label": "AMOLED",
        "manifest": "manifest-amoled.json",
    },
    {
        "env": "waveshare_128",
        "name": "OpenDrift Round 1.28",
        "label": "Round",
        "manifest": "manifest-round.json",
    },
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-dir", type=Path, default=Path("OpenDrift"))
    parser.add_argument("--site-dir", type=Path, default=Path("installer"))
    parser.add_argument("--release-dir", type=Path, default=Path("release-assets"))
    parser.add_argument("--version", required=True)
    parser.add_argument("--commit", required=True)
    return parser.parse_args()


def find_boot_app0() -> Path:
    core_dir = Path(
        os.environ.get(
            "PLATFORMIO_CORE_DIR",
            Path.home() / ".platformio",
        )
    )
    boot_app0 = (
        core_dir
        / "packages"
        / "framework-arduinoespressif32"
        / "tools"
        / "partitions"
        / "boot_app0.bin"
    )

    if not boot_app0.is_file():
        raise FileNotFoundError(f"boot_app0.bin not found at {boot_app0}")

    return boot_app0


def esptool_command() -> list[str]:
    if importlib.util.find_spec("esptool") is not None:
        return [sys.executable, "-m", "esptool"]

    core_dir = Path(
        os.environ.get(
            "PLATFORMIO_CORE_DIR",
            Path.home() / ".platformio",
        )
    )
    esptool_script = core_dir / "packages" / "tool-esptoolpy" / "esptool.py"

    if not esptool_script.is_file():
        raise FileNotFoundError(
            "esptool is not installed as a Python module and PlatformIO's "
            f"copy was not found at {esptool_script}"
        )

    return [sys.executable, str(esptool_script)]


def safe_version(value: str) -> str:
    allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_"
    cleaned = "".join(character for character in value if character in allowed)

    if not cleaned:
        raise ValueError("Version does not contain a filename-safe character")

    return cleaned


def sha256(path: Path) -> str:
    digest = hashlib.sha256()

    with path.open("rb") as firmware_file:
        for chunk in iter(lambda: firmware_file.read(1024 * 1024), b""):
            digest.update(chunk)

    return digest.hexdigest()


def main() -> int:
    args = parse_args()
    project_dir = args.project_dir.resolve()
    site_dir = args.site_dir.resolve()
    release_dir = args.release_dir.resolve()
    version = safe_version(args.version)
    short_commit = args.commit[:8]
    firmware_dir = site_dir / "firmware"

    firmware_dir.mkdir(parents=True, exist_ok=True)
    release_dir.mkdir(parents=True, exist_ok=True)

    for stale_factory in firmware_dir.glob("OpenDrift-*.factory.bin"):
        stale_factory.unlink()

    boot_app0 = find_boot_app0()
    esptool = esptool_command()
    checksums: list[str] = []

    for board in BOARDS:
        build_dir = project_dir / ".pio" / "build" / board["env"]
        bootloader = build_dir / "bootloader.bin"
        partitions = build_dir / "partitions.bin"
        application = build_dir / "firmware.bin"

        for required_file in (bootloader, partitions, application):
            if not required_file.is_file():
                raise FileNotFoundError(
                    f"Missing {required_file}; build {board['env']} first"
                )

        factory_name = f"OpenDrift-{board['label']}-{version}.factory.bin"
        update_name = f"OpenDrift-{board['label']}-{version}.update.bin"
        factory_path = firmware_dir / factory_name
        release_factory_path = release_dir / factory_name
        release_update_path = release_dir / update_name

        subprocess.run(
            esptool
            + [
                "--chip",
                "esp32s3",
                "merge_bin",
                "-o",
                str(factory_path),
                "--flash_mode",
                "dio",
                "--flash_freq",
                "80m",
                "--flash_size",
                "16MB",
                "0x0000",
                str(bootloader),
                "0x8000",
                str(partitions),
                "0xe000",
                str(boot_app0),
                "0x10000",
                str(application),
            ],
            check=True,
        )

        shutil.copy2(factory_path, release_factory_path)
        shutil.copy2(application, release_update_path)

        manifest = {
            "name": board["name"],
            "version": version,
            "new_install_prompt_erase": True,
            "new_install_improv_wait_time": 0,
            "builds": [
                {
                    "chipFamily": "ESP32-S3",
                    "improv": False,
                    "parts": [
                        {
                            "path": f"firmware/{factory_name}",
                            "offset": 0,
                        }
                    ],
                }
            ],
        }

        (site_dir / board["manifest"]).write_text(
            json.dumps(manifest, indent=2) + "\n",
            encoding="utf-8",
        )

        for release_path in (release_factory_path, release_update_path):
            checksums.append(f"{sha256(release_path)}  {release_path.name}")

    checksum_path = release_dir / "SHA256SUMS.txt"
    checksum_path.write_text("\n".join(checksums) + "\n", encoding="utf-8")

    release_info = {
        "version": version,
        "commit": args.commit,
        "shortCommit": short_commit,
    }
    (site_dir / "release.json").write_text(
        json.dumps(release_info, indent=2) + "\n",
        encoding="utf-8",
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
