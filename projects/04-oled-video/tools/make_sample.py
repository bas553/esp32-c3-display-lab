#!/usr/bin/env python3
"""Create a five-second original moving-ball/gray-ramp MP4 using FFmpeg."""
import argparse
import math
from pathlib import Path
import subprocess
import tempfile

from mp4_to_oled import find_ffmpeg


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path,
                        default=Path(__file__).resolve().parents[1] / "videos" / "sample.mp4")
    parser.add_argument("--ffmpeg")
    args = parser.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    width, height, fps, frames = 256, 128, 10, 50
    with tempfile.TemporaryDirectory() as temporary:
        raw = Path(temporary) / "sample.gray"
        with raw.open("wb") as stream:
            for frame in range(frames):
                pixels = bytearray(width * height)
                cx = 24 + int(frame * 208 / (frames - 1))
                cy = 70 - int(30 * abs(math.sin(frame * math.pi / 12)))
                for y in range(height):
                    for x in range(width):
                        if y < 20:
                            value = x  # Gray ramp demonstrates ordered dithering.
                        elif y in (23, 105):
                            value = 255
                        elif (x - cx) ** 2 + (y - cy) ** 2 <= 13 ** 2:
                            value = 255
                        elif 112 <= y <= 117 and (x + frame * 4) % 32 < 12:
                            value = 200
                        else:
                            value = 0
                        pixels[y * width + x] = value
                stream.write(pixels)
        subprocess.run([find_ffmpeg(args.ffmpeg), "-hide_banner", "-loglevel", "error", "-nostdin", "-y",
                        "-f", "rawvideo", "-pix_fmt", "gray", "-s", f"{width}x{height}", "-r", str(fps),
                        "-i", str(raw), "-an", "-c:v", "libx264", "-crf", "18", "-pix_fmt", "yuv420p",
                        "-movflags", "+faststart", str(output)], check=True)
    print(f"Created {output}: {frames} frames, {fps} FPS, 5 seconds")


if __name__ == "__main__":
    main()
