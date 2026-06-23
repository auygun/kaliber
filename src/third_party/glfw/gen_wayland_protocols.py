#!/usr/bin/env python3
import os
import subprocess
import sys

def main():
    if len(sys.argv) < 3:
        print("Usage: gen_wayland_protocols.py <output_dir> <xml_dir> <proto1> [proto2] ...")
        sys.exit(1)

    output_dir = sys.argv[1]
    xml_dir = sys.argv[2]
    protocols = sys.argv[3:]

    os.makedirs(output_dir, exist_ok=True)

    for proto in protocols:
        xml_path = os.path.join(xml_dir, proto + ".xml")
        header_path = os.path.join(output_dir, proto + "-client-protocol.h")
        code_path = os.path.join(output_dir, proto + "-client-protocol-code.h")

        subprocess.check_call([
            "wayland-scanner", "client-header", xml_path, header_path
        ])

        subprocess.check_call([
            "wayland-scanner", "private-code", xml_path, code_path
        ])

if __name__ == "__main__":
    main()
