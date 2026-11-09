import io
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import unittest
import mp4_to_tft as converter

ROOT = Path(__file__).resolve().parents[1]


def header_frames(path):
    blocks = re.findall(r"\{ // Frame \d+\n(.*?)\n  \}", path.read_text(encoding="utf-8"), re.S)
    return [[int(word, 16) for word in re.findall(r"0x([0-9A-F]{4})", block)] for block in blocks]


class PackingTests(unittest.TestCase):
    def test_rgb565_golden_colors(self):
        rgb = bytes((255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0))
        packed = converter.pack_rgb565(rgb * (12800 // 5))
        self.assertEqual(len(packed), 25600)
        self.assertEqual(packed[:10], bytes.fromhex("00f8 e007 1f00 ffff 0000"))

    def test_pixel_row_boundaries(self):
        rgb = bytearray(38400)
        for index in (0, 159, 160, 12799):
            rgb[index * 3] = 255
        words = struct.unpack("<12800H", converter.pack_rgb565(rgb))
        self.assertEqual([i for i, value in enumerate(words) if value], [0, 159, 160, 12799])
        self.assertTrue(all(words[i] == 0xF800 for i in (0, 159, 160, 12799)))

    def test_invert_swap_and_bad_size(self):
        red = bytes((255, 0, 0)) * 12800
        self.assertEqual(converter.pack_rgb565(red, swap_rb=True)[:2], bytes.fromhex("1f00"))
        self.assertEqual(converter.pack_rgb565(red, invert=True)[:2], bytes.fromhex("ff07"))
        with self.assertRaises(ValueError):
            converter.pack_rgb565(bytes(38399))

    def test_partial_pipe(self):
        class ShortPipe(io.BytesIO):
            def read(self, size=-1):
                return super().read(min(size, 59))
        self.assertEqual(converter.read_frame(ShortPipe(bytes(38400))), bytes(38400))
        with self.assertRaises(ValueError):
            converter.read_frame(io.BytesIO(bytes(38399)))


class IntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="tft-test-")
        cls.folder = Path(cls.temp.name)
        cls.sample = ROOT / "videos/sample.mp4"
        cls.ffmpeg = converter.find_ffmpeg()

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def convert(self, source, name, *options, success=True):
        output = self.folder / name
        process = subprocess.run([sys.executable, str(ROOT / "tools/mp4_to_tft.py"), str(source),
                                  "--output", str(output), *options], capture_output=True, text=True,
                                 encoding="utf-8", errors="replace")
        if success:
            self.assertEqual(process.returncode, 0, process.stderr)
        else:
            self.assertNotEqual(process.returncode, 0)
        return output, process

    def test_sample_every_frame(self):
        output, _ = self.convert(self.sample, "sample.h", "--fps", "10", "--seconds", "3")
        frames = header_frames(output)
        self.assertEqual(len(frames), 30)
        self.assertTrue(all(len(frame) * 2 == 25600 for frame in frames))
        self.assertNotEqual(frames[0], frames[15])
        meta = json.loads(output.with_suffix(".json").read_text())
        self.assertEqual(meta["video_flash_bytes"], 768000)
        self.assertFalse(meta["over_budget"])

    def test_crop_letterbox(self):
        portrait = self.folder / "portrait clip.mp4"
        subprocess.run([self.ffmpeg, "-v", "error", "-y", "-f", "lavfi", "-i",
                        "color=white:size=80x160:rate=2:duration=1", "-c:v", "libx264", str(portrait)], check=True)
        letterbox, _ = self.convert(portrait, "letterbox.h", "--fps", "2", "--fit", "letterbox")
        crop, _ = self.convert(portrait, "crop.h", "--fps", "2", "--fit", "crop")
        expected_row = [0] * 60 + [65535] * 40 + [0] * 60
        for frame in header_frames(letterbox):
            self.assertEqual(frame, expected_row * 80)
        for frame in header_frames(crop):
            self.assertEqual(frame, [65535] * 12800)

    def test_gif_fractional_fps(self):
        gif = self.folder / "sample.gif"
        subprocess.run([self.ffmpeg, "-v", "error", "-y", "-i", str(self.sample), "-t", "1", "-vf", "fps=5", str(gif)], check=True)
        output, _ = self.convert(gif, "gif.h", "--fps", "10", "--seconds", "1")
        self.assertEqual(len(header_frames(output)), 10)
        output, _ = self.convert(self.sample, "fraction.h", "--fps", "2.5", "--seconds", "2")
        self.assertEqual(len(header_frames(output)), 5)

    def test_flash_and_failure_preserve_output(self):
        output, result = self.convert(self.sample, "size.h", "--fps", "2", "--seconds", "1", "--flash-budget", "25600", "--firmware-reserve", "0")
        self.assertIn("WARNING", result.stderr)
        original = output.read_bytes()
        self.convert(self.sample, "size.h", "--flash-budget", "25600", "--firmware-reserve", "0", "--strict-size", success=False)
        self.assertEqual(output.read_bytes(), original)
        bad = self.folder / "bad.mp4"
        bad.write_bytes(b"invalid video")
        self.convert(bad, "size.h", success=False)
        self.assertEqual(output.read_bytes(), original)


if __name__ == "__main__":
    unittest.main(verbosity=2)
