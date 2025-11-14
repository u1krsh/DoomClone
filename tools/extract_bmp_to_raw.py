#!/usr/bin/env python3
"""
Extract raw RGB data from a BMP file embedded in a C header file.
This reads the BMP data from T_00.h and extracts just the pixel data.
"""

import struct

def read_bmp_header(data):
    """Read BMP file header and info header"""
    # BMP File Header (14 bytes)
    if data[0:2] != b'BM':
        raise ValueError("Not a valid BMP file")
    
    file_size = struct.unpack('<I', data[2:6])[0]
    pixel_offset = struct.unpack('<I', data[10:14])[0]
    
    # DIB Header (BITMAPV4HEADER is 108 bytes, starting at offset 14)
    header_size = struct.unpack('<I', data[14:18])[0]
    width = struct.unpack('<i', data[18:22])[0]
    height = struct.unpack('<i', data[22:26])[0]
    bits_per_pixel = struct.unpack('<H', data[28:30])[0]
    compression = struct.unpack('<I', data[30:34])[0]
    
    print(f"BMP Info:")
    print(f"  File size: {file_size} bytes")
    print(f"  Width: {width}px")
    print(f"  Height: {height}px")
    print(f"  Bits per pixel: {bits_per_pixel}")
    print(f"  Compression: {compression} ({'RLE8' if compression == 1 else 'None' if compression == 0 else 'Other'})")
    print(f"  Pixel data offset: {pixel_offset}")
    print(f"  Header size: {header_size}")
    
    return width, height, bits_per_pixel, pixel_offset, compression

def decompress_rle8(data, width, height, pixel_offset):
    """Decompress RLE8 compressed BMP data"""
    compressed = data[pixel_offset:]
    pixels = []
    
    i = 0
    while i < len(compressed):
        count = compressed[i]
        value = compressed[i + 1] if i + 1 < len(compressed) else 0
        i += 2
        
        if count == 0:  # Escape code
            if value == 0:  # End of line
                # Pad to end of scanline if needed
                while len(pixels) % width != 0:
                    pixels.append(0)
            elif value == 1:  # End of bitmap
                break
            elif value == 2:  # Delta (skip pixels)
                if i + 1 < len(compressed):
                    dx = compressed[i]
                    dy = compressed[i + 1]
                    i += 2
                    # Skip pixels
                    pixels.extend([0] * (dx + dy * width))
            else:  # Absolute mode: value = number of pixels to read literally
                for _ in range(value):
                    if i < len(compressed):
                        pixels.append(compressed[i])
                        i += 1
                # Absolute mode data is padded to word boundary
                if value % 2 == 1:
                    i += 1
        else:  # Encoded mode: repeat 'value' 'count' times
            pixels.extend([value] * count)
    
    # Ensure we have exactly width * height pixels
    while len(pixels) < width * height:
        pixels.append(0)
    pixels = pixels[:width * height]
    
    return pixels

def convert_indexed_to_rgb(data, width, height, pixel_offset, palette_offset=54, compression=0):
    """Convert 8-bit indexed color to RGB using the color palette"""
    # Read the color palette (256 colors * 4 bytes each = 1024 bytes)
    palette = []
    for i in range(256):
        offset = palette_offset + (i * 4)
        if offset + 3 < len(data):
            b = data[offset]
            g = data[offset + 1]
            r = data[offset + 2]
            palette.append((r, g, b))
        else:
            palette.append((0, 0, 0))
    
    # Decompress or read pixel indices
    if compression == 1:  # RLE8
        print("Decompressing RLE8 data...")
        pixel_indices = decompress_rle8(data, width, height, pixel_offset)
    else:  # Uncompressed
        pixel_data = data[pixel_offset:]
        # BMP rows are stored bottom-to-top and padded to 4-byte boundaries
        row_size = width  # 8-bit = 1 byte per pixel
        padding = (4 - (row_size % 4)) % 4
        actual_row_size = row_size + padding
        
        pixel_indices = []
        for y in range(height):
            row_offset = y * actual_row_size
            pixel_indices.extend(pixel_data[row_offset:row_offset + width])
    
    # Convert to RGB, flipping vertically (BMP is bottom-up)
    rgb_data = []
    for y in range(height - 1, -1, -1):  # Start from bottom row
        for x in range(width):
            index = pixel_indices[y * width + x]
            r, g, b = palette[index]
            rgb_data.extend([r, g, b])
    
    return rgb_data

def generate_c_header(rgb_data, width, height, output_name):
    """Generate C header file with raw RGB data"""
    header = f"""#define {output_name}_WIDTH {width}
#define {output_name}_HEIGHT {height}

// Raw RGB data: {width} x {height} x 3 = {len(rgb_data)} bytes
const unsigned char {output_name}[] = {{
"""
    
    # Write pixel data in rows of 16 values (5-6 pixels per line)
    for i in range(0, len(rgb_data), 48):  # 48 bytes = 16 RGB triplets
        chunk = rgb_data[i:i+48]
        header += "  "
        header += ", ".join(str(b) for b in chunk)
        if i + 48 < len(rgb_data):
            header += ",\n"
        else:
            header += "\n"
    
    header += "};\n"
    
    return header

# Read the current T_00.h and extract BMP data
with open('textures/T_00.h', 'r') as f:
    content = f.read()

# Extract the byte array
import re
match = re.search(r'const unsigned char T_00\[\] = \{([^}]+)\}', content, re.DOTALL)
if not match:
    print("ERROR: Could not find T_00 array in file")
    exit(1)

byte_string = match.group(1)
bytes_list = [int(x.strip()) for x in byte_string.split(',') if x.strip()]
bmp_data = bytes(bytes_list)

print(f"Read {len(bmp_data)} bytes from T_00.h")

# Parse BMP
width, height, bpp, pixel_offset, compression = read_bmp_header(bmp_data)

# Convert indexed color to RGB
if bpp == 8:
    print("\nConverting 8-bit indexed color to RGB...")
    rgb_data = convert_indexed_to_rgb(bmp_data, width, height, pixel_offset, compression=compression)
else:
    print(f"\nERROR: Unsupported BMP format (bits per pixel: {bpp})")
    exit(1)

# Generate new header
print(f"\nGenerating new T_00.h with {len(rgb_data)} bytes of raw RGB data...")
header_content = generate_c_header(rgb_data, width, height, "T_00")

# Write to file
with open('textures/T_00.h', 'w') as f:
    f.write(header_content)

print(f"? Successfully wrote textures/T_00.h")
print(f"  Size: {width}x{height} = {len(rgb_data)} bytes")
