import sys
import os
import argparse
from pathlib import Path

def main():
    convert("cheeki_breeki", alpha_col=(0, 0, 255))
    convert("cheeki_breeki_open", alpha_col=(0, 0, 255))
    convert("compose", alpha_col=(0, 0, 255))

    convert("bg")


def convert(what,alpha_col=None):
    print(what)
    here = Path(__file__).parent

    in_path = here / f"{what}.ppm"
    out_path = here / f"../src/mode_displays/img_{what}.h"
    with open(in_path, "rb") as f:
        width, height, maxval = parse_p6_header(f)
        print(f"  Image : {width} x {height}")
        pixels = list(read_pixels(f, width, height, alpha_col=alpha_col))

    header_text = generate_header(pixels, width, height, what)

    with open(out_path, "w") as f:
        f.write(header_text)




def parse_p6_header(f):
    """Parse the P6 PPM header and return (width, height, maxval)."""

    def read_token():
        """Read one whitespace-delimited token, skipping # comments."""
        token = b""
        while True:
            ch = f.read(1)
            if not ch:
                raise ValueError("Unexpected end of file in PPM header.")
            if ch == b"#":
                while f.read(1) not in (b"\n", b""):
                    pass
            elif ch in (b" ", b"\t", b"\r", b"\n"):
                if token:
                    return token.decode()
            else:
                token += ch

    magic = read_token()
    if magic != "P6":
        raise ValueError(f"Not a P6 PPM file. Magic: {magic!r}")

    width  = int(read_token())
    height = int(read_token())
    maxval = int(read_token())

    if width == 0 or height == 0:
        raise ValueError("WIDTH or HEIGHT is zero.")
    if maxval != 255:
        raise ValueError(f"MAXVAL={maxval} is unsupported.")

    return width, height, maxval


def rgb_to_rgb565(r, g, b):
    """Convert 8-bit R, G, B to a 16-bit RGB565 value."""
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def read_pixels(f, width, height, alpha_col):
    """Read raw pixel data and return a list of RGB565 values."""
    bytes_per_pixel   = 3 
    total_bytes       = width * height * bytes_per_pixel

    raw = f.read(total_bytes)
    if len(raw) != total_bytes:
        raise ValueError(
            f"Expected {total_bytes} bytes of pixel data, got {len(raw)}."
        )

    for i in range(0, total_bytes, 3):
        col = raw[i], raw[i + 1], raw[i + 2]
        new_col = rgb_to_rgb565(*col)
        if alpha_col:
            if col == alpha_col:
                yield 0x0000
                continue
            elif new_col == 0x0000:
                yield 1
                continue
        yield new_col
  


def generate_header(pixels, width, height, what):
    values_per_line = 12

    res = f"""
#pragma once
#include "image.h"

constexpr Image<{width}, {height}> img_{what} = {{
"""
    


    for row in range(height):
        row_pixels = pixels[row * width : (row + 1) * width]
        for i in range(0, len(row_pixels), values_per_line):
            chunk = row_pixels[i : i + values_per_line]
            res += "    " + ", ".join(f"0x{v:04X}" for v in chunk) + ",\n"

    res += "};"
    return res




if __name__ == "__main__":
    main()
