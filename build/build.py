import subprocess
from pathlib import Path

# Debug
subprocess.run(["gn",
                "gen",
                "--args=is_debug=true",
                "out/debug"],
               cwd=Path.cwd(), check=True)
subprocess.run(["ninja",
                "-C",
                "out/debug"],
               cwd=Path.cwd(), check=True)

# Release
subprocess.run(["gn",
                "gen",
                "out/release"],
               cwd=Path.cwd(), check=True)
subprocess.run(["ninja",
                "-C",
                "out/release"],
               cwd=Path.cwd(), check=True)
