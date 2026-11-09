#!/usr/bin/env python3
"""Create an original three-second color animation for the TFT player."""
import argparse
import math
from pathlib import Path
import subprocess
import tempfile
from mp4_to_tft import find_ffmpeg


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] / "videos/sample.mp4")
    parser.add_argument("--ffmpeg")
    args = parser.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    width, height = 320, 160
    with tempfile.TemporaryDirectory(dir=output.parent) as temporary:
        raw = Path(temporary) / "sample.rgb"
        with raw.open("wb") as stream:
            for frame in range(30):
                pixels = bytearray(width * height * 3)
                cx = 25 + int(frame * 270 / 29)
                cy = 112 - int(45 * abs(math.sin(frame * math.pi / 10)))
                for y in range(height):
                    for x in range(width):
                        if x in (0, width - 1) or y in (0, height - 1):
                            color = (255, 255, 255)
                        elif y < 30:
                            color = ((255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 255))[x // 80]
                        elif (x - cx) ** 2 + (y - cy) ** 2 <= 17 ** 2:
                            color = (255, 210, 20)
                        elif y >= 138:
                            color = ((x + frame * 8) % 256, 80, (255 - x + frame * 8) % 256)
                        else:
                            color = (5, 12, 32)
                        index = (y * width + x) * 3
                        pixels[index:index + 3] = bytes(color)
                stream.write(pixels)
        subprocess.run([find_ffmpeg(args.ffmpeg), "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24",
                        "-s", "320x160", "-r", "10", "-i", str(raw), "-an", "-c:v", "libx264",
                        "-crf", "15", "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(output)], check=True)
    print(f"Created {output}: 30 frames, 10 FPS, 3 seconds")


if __name__ == "__main__":
    main()
