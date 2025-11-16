#!/usr/bin/env python3
"""
c_array_to_image.py

Convert a C-style array (numbers separated by commas, optionally wrapped in braces)
into an image file (PNG/JPEG).

Usage examples:
  python c_array_to_image.py --input my_array.txt --out out.png --width 128
  python c_array_to_image.py --input my_array.txt --mode auto
  python c_array_to_image.py --input my_array.txt --out out.png --width 32 --mode rgb
"""

import re
import math
import argparse
from PIL import Image

def parse_c_array(text):
    """
    Extract integers from a C-style array text.
    Accepts numbers separated by commas, newlines, braces, etc.
    Returns a list of ints (0..255).
    """
    nums = re.findall(r'-?\d+', text)
    return [int(n) for n in nums]

def infer_shape(n_pixels, width=None, channels=1):
    """
    Infer (width, height) for total samples n_pixels (which are raw values, not pixels).
    channels: 1 (grayscale) or 3 (RGB)
    If width is given, use it (must divide n_pixels/channels).
    Otherwise try perfect-square on pixel count, else return width=height if possible,
    else return width = n_pixels // channels, height = 1 (single-row).
    """
    if n_pixels % channels != 0:
        return None, None  # can't map to pixel grid with this channel count

    n_pix = n_pixels // channels

    if width:
        if n_pix % width != 0:
            return None, None
        return width, n_pix // width

    # try square
    s = int(math.isqrt(n_pix))
    if s * s == n_pix:
        return s, s

    # try common widths (power-of-two-ish)
    for w in (256, 128, 64, 32, 16, 8, 4, 2):
        if n_pix % w == 0:
            return w, n_pix // w

    # fallback to single-row
    return n_pix, 1

def create_image(values, out_path, width=None, mode='auto'):
    """
    values: list of ints (0..255)
    mode: 'auto', 'grayscale', 'rgb'
    """
    if not values:
        raise ValueError("No values parsed from input.")

    # clamp values to byte range
    values = [max(0, min(255, int(v))) for v in values]

    # decide mode
    if mode == 'auto':
        if len(values) % 3 == 0:
            # ambiguous: could be grayscale or RGB. We'll check distribution:
            # if many triples are equal -> likely grayscale stored as triples
            triples_equal = sum(1 for i in range(0, len(values), 3)
                                if values[i] == values[i+1] == values[i+2])
            if triples_equal / (len(values) / 3) > 0.9:
                # most triples equal -> treat as grayscale packed in RGB
                channels = 1
            else:
                channels = 3
        else:
            channels = 1
    elif mode.lower() in ('gray', 'grayscale'):
        channels = 1
    elif mode.lower() == 'rgb':
        channels = 3
    else:
        raise ValueError("Unknown mode: " + str(mode))

    w, h = infer_shape(len(values), width=width, channels=channels)
    if w is None:
        raise ValueError("Cannot infer a valid width/height with the provided data and parameters."
                         " Try specifying a --width or change --mode between 'grayscale' and 'rgb'.")

    # Build pixel data
    if channels == 1:
        # each value is a pixel
        n_pix = w * h
        if len(values) != n_pix:
            raise ValueError(f"Data count ({len(values)}) doesn't match expected pixels ({n_pix}) for grayscale.")
        img = Image.frombytes('L', (w, h), bytes(values))
    else:
        # RGB
        n_pix = w * h
        if len(values) != n_pix * 3:
            raise ValueError(f"Data count ({len(values)}) doesn't match expected pixels*3 ({n_pix*3}) for RGB.")
        img = Image.frombytes('RGB', (w, h), bytes(values))

    img.save(out_path)
    return w, h, channels

def main():
    p = argparse.ArgumentParser(description="Convert C array to image.")
    p.add_argument('--input', '-i', required=True, help="Path to file containing the C-style array (or paste).")
    p.add_argument('--out', '-o', default='out.png', help="Output image filename (png/jpg).")
    p.add_argument('--width', '-w', type=int, default=None, help="Image width in pixels (optional).")
    p.add_argument('--mode', choices=['auto','grayscale','rgb'], default='auto',
                   help="Interpretation mode: 'auto'/'grayscale'/'rgb'.")
    args = p.parse_args()

    with open(args.input, 'r', encoding='utf-8') as f:
        txt = f.read()

    vals = parse_c_array(txt)
    try:
        w, h, channels = create_image(vals, args.out, width=args.width, mode=args.mode)
    except ValueError as e:
        print("ERROR:", e)
        return

    print(f"Saved {args.out}  — size: {w}x{h}, channels: {channels}")

if __name__ == '__main__':
    main()
