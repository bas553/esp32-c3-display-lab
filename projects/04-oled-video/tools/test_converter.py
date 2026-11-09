"""Unit and real FFmpeg integration tests; run: python tools/test_converter.py"""
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

import mp4_to_oled as converter

ROOT = Path(__file__).resolve().parents[1]


def header_frames(path):
    text = path.read_text(encoding="utf-8")
    blocks = re.findall(r"\{ // Frame \d+\n(.*?)\n  \}", text, re.S)
    return [bytes(int(value, 16) for value in re.findall(r"0x([0-9A-F]{2})", block)) for block in blocks]


class PackingTests(unittest.TestCase):
    def test_golden_boundary_pixels(self):
        pixels = bytearray(8192)
        for x, y in ((0, 0), (0, 7), (127, 0), (0, 8), (127, 63)):
            pixels[y * 128 + x] = 1
        expected = bytearray(1024)
        expected[0], expected[127], expected[128], expected[1023] = 0x81, 1, 1, 0x80
        self.assertEqual(converter.pack_ssd1306(pixels), bytes(expected))

    def test_full_row_and_column(self):
        row = bytearray(8192)
        row[8 * 128:9 * 128] = bytes([1] * 128)
        self.assertEqual(converter.pack_ssd1306(row), bytes(128) + bytes([1] * 128) + bytes(768))
        column = bytearray(8192)
        for y in range(64):
            column[y * 128 + 17] = 1
        packed = converter.pack_ssd1306(column)
        self.assertEqual([i for i, value in enumerate(packed) if value], [17 + 128 * p for p in range(8)])
        self.assertEqual(sum(packed), 255 * 8)

    def test_threshold_and_invert(self):
        gray = bytes([0, 127, 128, 255]) * 2048
        self.assertEqual(converter.monochrome(gray, "threshold"), bytes([0, 0, 1, 1]) * 2048)
        normal = converter.pack_ssd1306(converter.monochrome(gray, "threshold"))
        inverse = converter.pack_ssd1306(converter.monochrome(gray, "threshold", invert=True))
        self.assertEqual(inverse, bytes(255 ^ byte for byte in normal))

    def test_ordered_midgray_checkerboard(self):
        pixels = converter.monochrome(bytes([128] * 8192), "ordered")
        # Bayer at 50% produces this known 4x4 checkerboard, with 4096 lit pixels.
        for y, expected in enumerate(([1, 0, 1, 0], [0, 1, 0, 1], [1, 0, 1, 0], [0, 1, 0, 1])):
            self.assertEqual(list(pixels[y * 128:y * 128 + 4]), expected)
        self.assertEqual(sum(pixels), 4096)
        self.assertFalse(any(converter.monochrome(bytes(8192))))
        self.assertTrue(all(converter.monochrome(bytes([255] * 8192))))

    def test_partial_pipe_reads_and_invalid_size(self):
        class ShortPipe(io.BytesIO):
            def read(self, size=-1):
                return super().read(min(size, 37))
        self.assertEqual(converter.read_frame(ShortPipe(bytes(8192))), bytes(8192))
        with self.assertRaises(ValueError):
            converter.read_frame(io.BytesIO(bytes(8191)))
        with self.assertRaises(ValueError):
            converter.pack_ssd1306(bytes(1023))


class IntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="oled-tests-")
        cls.folder = Path(cls.temporary.name)
        cls.ffmpeg = converter.find_ffmpeg()
        cls.sample = ROOT / "videos" / "sample.mp4"
        if not cls.sample.exists():
            raise RuntimeError("Run tools/make_sample.py first")

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def run_converter(self, source, name, *options, success=True):
        output = self.folder / name
        process = subprocess.run([sys.executable, str(ROOT / "tools" / "mp4_to_oled.py"),
                                  str(source), "--output", str(output), *options],
                                 capture_output=True, text=True, encoding="utf-8", errors="replace")
        if success:
            self.assertEqual(process.returncode, 0, process.stderr)
        else:
            self.assertNotEqual(process.returncode, 0)
        return output, process

    def test_sample_and_all_frame_sizes(self):
        output, _ = self.run_converter(self.sample, "sample.h", "--fps", "10", "--seconds", "5")
        frames = header_frames(output)
        self.assertEqual(len(frames), 50)
        self.assertTrue(all(len(frame) == 1024 for frame in frames))
        self.assertNotEqual(frames[0], frames[20])
        meta = json.loads(output.with_suffix(".json").read_text())
        self.assertEqual(meta["video_flash_bytes"], 51200)

    def test_crop_letterbox_and_paths_with_spaces(self):
        portrait = self.folder / "portrait clip.mp4"
        subprocess.run([self.ffmpeg, "-v", "error", "-y", "-f", "lavfi", "-i",
                        "color=white:size=64x128:rate=4:duration=1", "-c:v", "libx264",
                        "-pix_fmt", "yuv420p", str(portrait)], check=True)
        letterbox, _ = self.run_converter(portrait, "letterbox output.h", "--method", "threshold", "--fit", "letterbox", "--fps", "4")
        crop, _ = self.run_converter(portrait, "crop.h", "--method", "threshold", "--fit", "crop", "--fps", "4")
        for frame in header_frames(letterbox):
            self.assertEqual(frame, (bytes(48) + bytes([255] * 32) + bytes(48)) * 8)
        for frame in header_frames(crop):
            self.assertEqual(frame, bytes([255] * 1024))

    def test_gif_and_fractional_fps(self):
        gif = self.folder / "sample.gif"
        subprocess.run([self.ffmpeg, "-v", "error", "-y", "-i", str(self.sample),
                        "-t", "1", "-vf", "fps=5", str(gif)], check=True)
        output, _ = self.run_converter(gif, "gif.h", "--fps", "10", "--seconds", "1", "--invert")
        self.assertEqual(len(header_frames(output)), 10)
        output, _ = self.run_converter(self.sample, "fractional.h", "--fps", "2.5", "--seconds", "2")
        self.assertEqual(len(header_frames(output)), 5)
        meta = json.loads(output.with_suffix(".json").read_text())
        self.assertEqual((meta["fps_num"], meta["fps_den"]), (5, 2))

    def test_budget_warning_and_atomic_failures(self):
        output, process = self.run_converter(self.sample, "budget.h", "--seconds", "1", "--flash-budget", "2048", "--firmware-reserve", "0")
        self.assertIn("WARNING", process.stderr)
        self.assertEqual(len(header_frames(output)), 10)
        original = output.read_bytes()
        _, process = self.run_converter(self.sample, "budget.h", "--flash-budget", "2048", "--firmware-reserve", "0", "--strict-size", success=False)
        self.assertIn("strict-size", process.stderr)
        self.assertEqual(output.read_bytes(), original)
        bad = self.folder / "corrupt.mp4"
        bad.write_bytes(b"not a movie")
        self.run_converter(bad, "budget.h", success=False)
        self.assertEqual(output.read_bytes(), original)

    def test_invalid_cli_inputs(self):
        for options in (("--fps", "0"), ("--seconds", "-1"), ("--seconds", "nan"), ("--threshold", "256"), ("--ffmpeg", "definitely_missing_ffmpeg.exe")):
            self.run_converter(self.sample, "invalid.h", *options, success=False)


if __name__ == "__main__":
    unittest.main(verbosity=2)
