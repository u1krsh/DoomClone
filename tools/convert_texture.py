#!/usr/bin/env python3
"""
Convert image files to C header format for the DoomClone texture system.
Requires PIL/Pillow: pip install pillow

Usage:
  python convert_texture.py input_image.png output_name_or_path

Examples:
  python convert_texture.py "D:\...\604.png" T_00
  python convert_texture.py "D:\...\604.png" textures/T_00.h
  python convert_texture.py 604.png "C:\temp\mytex.h"
"""

import sys
import os
import re
from PIL import Image

def make_identifier(name: str) -> str:
    """Create a safe C identifier from a filename (no extension)."""
    # keep letters, digits, and underscores; replace others with underscore
    ident = re.sub(r'[^0-9A-Za-z_]', '_', name)
    # identifiers can't start with a digit: prefix if necessary
    if re.match(r'^[0-9]', ident):
        ident = '_' + ident
    return ident

def image_to_c_header(input_path, output_name):
    # Load image
    img = Image.open(input_path)
    if img.mode != 'RGB':
        img = img.convert('RGB')

    width, height = img.size
    pixels = list(img.getdata())

    # Determine output path and sanitized identifier
    # If output_name looks like a path (contains separator) or ends with .h we treat it as a path
    if os.path.sep in output_name or output_name.endswith('.h'):
        output_path = output_name
        if not output_path.lower().endswith('.h'):
            output_path += '.h'
    else:
        # default directory 'textures' relative to script
        output_dir = os.path.join(os.path.dirname(__file__), 'textures')
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, output_name + '.h')

    # ensure parent dir exists
    parent_dir = os.path.dirname(output_path)
    if parent_dir:
        os.makedirs(parent_dir, exist_ok=True)

    # Prepare identifier (use base name without extension)
    base = os.path.splitext(os.path.basename(output_path))[0]
    ident = make_identifier(base)
    ident_upper = ident.upper()

    # Build header contents
    header_lines = []
    header_lines.append(f"#ifndef {ident_upper}_H")
    header_lines.append(f"#define {ident_upper}_H")
    header_lines.append("")
    header_lines.append(f"#define {ident_upper}_WIDTH {width}")
    header_lines.append(f"#define {ident_upper}_HEIGHT {height}")
    header_lines.append("")
    header_lines.append(f"// array size is {width * height * 3}")
    header_lines.append(f"const unsigned char {ident}[] = {{")

    # Add pixel data (16 pixels per line -> 48 values per line)
    values_per_pixel = 3
    pixels_per_line = 16
    vals_per_line = pixels_per_line * values_per_pixel

    flat_vals = []
    for (r, g, b) in pixels:
        flat_vals.extend([str(r), str(g), str(b)])

    for i in range(0, len(flat_vals), vals_per_line):
        chunk = flat_vals[i:i+vals_per_line]
        header_lines.append("  " + ", ".join(chunk) + ("," if i + vals_per_line < len(flat_vals) else ""))

    header_lines.append("};")
    header_lines.append("")
    header_lines.append(f"#endif /* {ident_upper}_H */")
    header_content = "\n".join(header_lines) + "\n"

    # Write to file (utf-8 is fine for this plain ASCII content)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(header_content)

    print(f"Converted {input_path} ({width}x{height}) -> {output_path}")
    print(f"Array size: {width * height * 3} bytes")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python convert_texture.py <input_image> <output_name_or_path>")
        print("Example: python convert_texture.py wall.png T_01")
        print("Example (explicit path): python convert_texture.py wall.png textures/T_01.h")
        sys.exit(1)

    input_file = sys.argv[1]
    output_name = sys.argv[2]

    image_to_c_header(input_file, output_name)
