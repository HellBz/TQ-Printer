#!/usr/bin/env python3
"""Build and upload Thermal-Printer.ino to an ESP32-S3.

Requirements:
    - Python 3
    - arduino-cli (https://arduino.github.io/arduino-cli/latest/installation/)
    - ESP32 board core (the script installs it automatically if missing)

Usage:
    python flash.py
    python flash.py --port COM3
    python flash.py --fqbn esp32:esp32:esp32s3
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

SKETCH = "Thermal-Printer.ino"
DEFAULT_FQBN = "esp32:esp32:esp32s3"
CORE = "esp32:esp32"
ESP32_INDEX = (
    "https://raw.githubusercontent.com/espressif/arduino-esp32/"
    "gh-pages/package_esp32_index.json"
)


def find_tool(name):
    """Return the absolute path of an executable or None."""
    path = shutil.which(name)
    if path:
        return path
    # Also check a few common Windows locations / names
    path = shutil.which(name + ".exe")
    if path:
        return path
    # Allow bundling arduino-cli next to this script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    local = os.path.join(script_dir, name + ".exe")
    if os.path.isfile(local):
        return local
    local_no_ext = os.path.join(script_dir, name)
    if os.path.isfile(local_no_ext):
        return local_no_ext
    return None


def run(cmd, check=False, capture_output=False):
    """Run a command and print it."""
    printable = " ".join(cmd) if isinstance(cmd, list) else cmd
    print(f"> {printable}")
    result = subprocess.run(
        cmd,
        check=check,
        capture_output=capture_output,
        text=True
    )
    return result


def ensure_core(arduino_cli):
    """Make sure the ESP32 core is installed."""
    result = run(
        [arduino_cli, "core", "list"],
        capture_output=True
    )

    if result.returncode != 0:
        print("Could not query installed Arduino cores.")
        print(result.stderr)
        return False

    if CORE in result.stdout:
        print(f"Core {CORE} is already installed.")
        return True

    print(f"Core {CORE} not found. Updating index and installing...")
    run(
        [
            arduino_cli,
            "core",
            "update-index",
            "--additional-urls",
            ESP32_INDEX,
        ],
        check=True,
    )
    run(
        [
            arduino_cli,
            "core",
            "install",
            CORE,
            "--additional-urls",
            ESP32_INDEX,
        ],
        check=True,
    )
    return True


def find_port(arduino_cli, fqbn):
    """Try to auto-detect the upload port from 'arduino-cli board list'."""
    result = run(
        [arduino_cli, "board", "list"],
        capture_output=True
    )

    if result.returncode != 0 or not result.stdout:
        return None

    # Prefer a port that reports the expected FQBN
    for line in result.stdout.splitlines():
        if fqbn in line:
            parts = line.split()
            if parts:
                return parts[0]

    # Fallback: first serial-like port
    for line in result.stdout.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("COM") or "/dev/" in stripped:
            parts = stripped.split()
            if parts:
                return parts[0]

    return None


def main():
    parser = argparse.ArgumentParser(
        description="Build and upload Thermal-Printer.ino to an ESP32-S3."
    )
    parser.add_argument(
        "--port",
        help="Serial port, e.g. COM3 or /dev/ttyUSB0. "
             "Auto-detected if omitted.",
    )
    parser.add_argument(
        "--fqbn",
        default=DEFAULT_FQBN,
        help=f"Fully Qualified Board Name (default: {DEFAULT_FQBN}).",
    )
    args = parser.parse_args()

    arduino_cli = find_tool("arduino-cli")
    if not arduino_cli:
        print("ERROR: arduino-cli not found in PATH.")
        print("Please install it from:")
        print("  https://arduino.github.io/arduino-cli/latest/installation/")
        print("and make sure it is added to your PATH.")
        return 1

    print(f"Using arduino-cli: {arduino_cli}")

    if not ensure_core(arduino_cli):
        return 1

    script_dir = os.path.dirname(os.path.abspath(__file__))

    # The sketch may live next to this script or in a subfolder named like it.
    project_dir = script_dir
    sketch_file = os.path.join(project_dir, SKETCH)

    if not os.path.exists(sketch_file):
        subfolder = os.path.join(
            script_dir,
            os.path.splitext(SKETCH)[0]
        )
        subfolder_sketch = os.path.join(subfolder, SKETCH)
        if os.path.exists(subfolder_sketch):
            project_dir = subfolder
            sketch_file = subfolder_sketch

    if not os.path.exists(sketch_file):
        print(f"ERROR: Sketch not found: {sketch_file}")
        return 1

    # Arduino CLI requires the sketch folder name to match the main .ino file name.
    # The project root may have a different name, so we copy the relevant files
    # into a temporary folder with the correct name before compiling.
    sketch_name = os.path.splitext(SKETCH)[0]
    tmp_root = tempfile.mkdtemp(prefix="thermal-printer-build-")
    build_dir = os.path.join(tmp_root, sketch_name)
    os.makedirs(build_dir, exist_ok=True)

    print(f"Preparing build folder: {build_dir}")

    for item in os.listdir(project_dir):
        # Skip the bundled arduino-cli binary/archive, VCS folders and the
        # stray 'nul' device file that cannot be copied on Windows.
        if item in (
            "arduino-cli.exe",
            "arduino-cli.zip",
            ".git",
            "nul",
        ):
            continue
        src = os.path.join(project_dir, item)
        dst = os.path.join(build_dir, item)
        if os.path.isdir(src):
            shutil.copytree(src, dst)
        else:
            shutil.copy2(src, dst)

    sketch_path = build_dir

    port = args.port
    if not port:
        port = find_port(arduino_cli, args.fqbn)
        if not port:
            print(
                "ERROR: Could not auto-detect the upload port. "
                "Please specify it with --port."
            )
            return 1
        print(f"Auto-detected port: {port}")
    else:
        print(f"Using port: {port}")

    print("Compiling and uploading...")
    try:
        run(
            [
                arduino_cli,
                "compile",
                "--fqbn",
                args.fqbn,
                "--upload",
                "--port",
                port,
                sketch_path,
            ],
            check=True,
        )
    finally:
        print(f"Cleaning up temporary build folder: {tmp_root}")
        shutil.rmtree(tmp_root, ignore_errors=True)

    print("Upload finished successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
