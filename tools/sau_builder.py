#!/usr/bin/env python3
"""
SAU Builder for DoomClone
Converts level.h text format to binary SAU format
Can also convert SAU back to text format

SAU Format v2 (DoomClone's custom level format):
  - Header: magic, version, counts
  - Sectors: wall range, heights, textures
  - Walls: coordinates, texture, tiling, shade
  - Player: spawn position and angle
  - Enemies: position and type
  - Textures: embedded texture data (NEW in v2)
"""

import struct
import sys
import os
import re

# SAU Format Constants
SAU_MAGIC = 0x5541534F  # "OSAU" in little endian (Oracular SAU)
SAU_VERSION = 2  # Version 2 includes textures


def parse_texture_h_file(filepath):
    """Parse a texture .h file and extract RGB data"""
    if not os.path.exists(filepath):
        return None
    
    try:
        with open(filepath, 'r') as f:
            data = f.read()
        
        textures = []
        
        # Parse dimension arrays first
        dim_arrays = {}
        int_array_matches = list(re.finditer(r'(?:static\s+)?(?:const\s+)?int\s+([a-zA-Z0-9_]+)(?:\[\d*\])?\s*=\s*\{([^}]*)\}', data, re.DOTALL))
        for match in int_array_matches:
            name = match.group(1)
            content = match.group(2)
            vals = [int(num) for num in re.findall(r'\b\d+\b', content)]
            dim_arrays[name] = vals

        # Find all char array definitions (texture data)
        array_matches = list(re.finditer(r'(?:static\s+)?(?:const\s+)?(?:unsigned\s+)?char\s+([a-zA-Z0-9_]+)(?:\[\])?\s*=\s*\{([^}]*)\}', data, re.DOTALL))
        
        for match in array_matches:
            name = match.group(1)
            content = match.group(2)
            
            # Find dimensions
            width = 0
            height = 0
            
            # Try various dimension patterns
            w_match = re.search(f'#define\\s+{name}_WIDTH\\s+(\\d+)', data)
            h_match = re.search(f'#define\\s+{name}_HEIGHT\\s+(\\d+)', data)
            
            if not w_match:
                base_name = re.sub(r'_frame_\d+$', '', name)
                w_match = re.search(f'#define\\s+{base_name}_FRAME_WIDTH\\s+(\\d+)', data)
                if not w_match:
                    w_match = re.search(f'#define\\s+{base_name}_WIDTH\\s+(\\d+)', data)
            
            if not h_match:
                base_name = re.sub(r'_frame_\d+$', '', name)
                h_match = re.search(f'#define\\s+{base_name}_FRAME_HEIGHT\\s+(\\d+)', data)
                if not h_match:
                    h_match = re.search(f'#define\\s+{base_name}_HEIGHT\\s+(\\d+)', data)
            
            if w_match and h_match:
                width = int(w_match.group(1))
                height = int(h_match.group(1))
            else:
                # Check for array-based dimensions
                frame_match = re.match(r'([a-zA-Z0-9_]+)_frame_(\d+)', name)
                if frame_match:
                    prefix = frame_match.group(1)
                    idx = int(frame_match.group(2))
                    w_arr_name = f"{prefix}_frame_widths"
                    h_arr_name = f"{prefix}_frame_heights"
                    
                    if w_arr_name in dim_arrays and h_arr_name in dim_arrays:
                        if idx < len(dim_arrays[w_arr_name]) and idx < len(dim_arrays[h_arr_name]):
                            width = dim_arrays[w_arr_name][idx]
                            height = dim_arrays[h_arr_name][idx]

            if width == 0 or height == 0:
                continue
            
            # Parse pixel data
            pixel_vals = [int(num) for num in re.findall(r'\b\d+\b', content)]
            
            expected = width * height * 3
            if len(pixel_vals) < expected:
                pixel_vals += [0] * (expected - len(pixel_vals))
            
            textures.append({
                'name': name,
                'width': width,
                'height': height,
                'data': bytes(pixel_vals[:expected])
            })
        
        return textures
    except Exception as e:
        print(f"Error parsing {filepath}: {e}")
        return None


def load_all_textures(texture_dir):
    """Load all textures in the order used by the game"""
    all_textures = []
    
    # Static textures T_00 through T_06
    for i in range(7):
        filepath = os.path.join(texture_dir, f"T_{i:02d}.h")
        textures = parse_texture_h_file(filepath)
        if textures:
            all_textures.append(textures[0])  # Take first texture from file
        else:
            # Create placeholder
            all_textures.append({
                'name': f'T_{i:02d}',
                'width': 64,
                'height': 64,
                'data': bytes([255, 0, 255] * 64 * 64)  # Magenta placeholder
            })
    
    # WALL57 animated frames (index 7)
    wall57_frames = []
    for frame_file in ["WALL57_2.h", "WALL57_3.h", "WALL57_4.h"]:
        filepath = os.path.join(texture_dir, frame_file)
        textures = parse_texture_h_file(filepath)
        if textures:
            wall57_frames.append(textures[0])
    
    if wall57_frames:
        # Combine frames into one entry with multiple frames
        all_textures.append({
            'name': 'WALL57_anim',
            'width': wall57_frames[0]['width'],
            'height': wall57_frames[0]['height'],
            'frames': wall57_frames,
            'frame_count': len(wall57_frames),
            'data': wall57_frames[0]['data']  # First frame as default
        })
    else:
        all_textures.append({
            'name': 'WALL57_anim',
            'width': 64,
            'height': 64,
            'data': bytes([255, 255, 0] * 64 * 64)  # Yellow placeholder
        })
    
    # WALL58 animated frames (index 8)
    filepath = os.path.join(texture_dir, "WALL58.h")
    wall58_textures = parse_texture_h_file(filepath)
    wall58_frames = [t for t in (wall58_textures or []) if 'frame' in t['name'].lower()]
    
    if wall58_frames:
        all_textures.append({
            'name': 'WALL58_anim',
            'width': wall58_frames[0]['width'],
            'height': wall58_frames[0]['height'],
            'frames': wall58_frames,
            'frame_count': len(wall58_frames),
            'data': wall58_frames[0]['data']
        })
    else:
        all_textures.append({
            'name': 'WALL58_anim',
            'width': 64,
            'height': 64,
            'data': bytes([0, 255, 255] * 64 * 64)  # Cyan placeholder
        })
    
    return all_textures


def parse_level_h(filename):
    """Parse the text-based level.h format"""
    with open(filename, 'r') as f:
        lines = [line.strip() for line in f.readlines() if line.strip()]
    
    idx = 0
    
    # Number of sectors
    num_sectors = int(lines[idx])
    idx += 1
    
    # Read sectors
    sectors = []
    for i in range(num_sectors):
        parts = lines[idx].split()
        sector = {
            'ws': int(parts[0]),
            'we': int(parts[1]),
            'z1': int(parts[2]),
            'z2': int(parts[3]),
            'st': int(parts[4]),
            'ss': int(parts[5])
        }
        sectors.append(sector)
        idx += 1
    
    # Number of walls
    num_walls = int(lines[idx])
    idx += 1
    
    # Read walls
    walls = []
    for i in range(num_walls):
        parts = lines[idx].split()
        wall = {
            'x1': int(parts[0]),
            'y1': int(parts[1]),
            'x2': int(parts[2]),
            'y2': int(parts[3]),
            'wt': int(parts[4]),
            'u': int(parts[5]),
            'v': int(parts[6]),
            'shade': int(parts[7])
        }
        walls.append(wall)
        idx += 1
    
    # Player spawn
    parts = lines[idx].split()
    player = {
        'x': int(parts[0]),
        'y': int(parts[1]),
        'z': int(parts[2]),
        'a': int(parts[3]),
        'l': int(parts[4])
    }
    idx += 1
    
    # Enemies (optional)
    enemies = []
    if idx < len(lines):
        try:
            num_enemies = int(lines[idx])
            idx += 1
            for i in range(num_enemies):
                if idx < len(lines):
                    parts = lines[idx].split()
                    enemy = {
                        'x': int(parts[0]),
                        'y': int(parts[1]),
                        'z': int(parts[2]),
                        'type': int(parts[3])
                    }
                    enemies.append(enemy)
                    idx += 1
        except (ValueError, IndexError):
            pass
    
    return sectors, walls, player, enemies


def write_sau(filename, sectors, walls, player, enemies, textures=None):
    """Write binary SAU file with optional embedded textures"""
    with open(filename, 'wb') as f:
        # Determine texture count
        num_textures = len(textures) if textures else 0
        
        # Write header (16 bytes for v2)
        # magic(4) + version(2) + numSectors(2) + numWalls(2) + numEnemies(2) + numTextures(2) + reserved(2)
        header = struct.pack('<IHHHHHI',
            SAU_MAGIC,
            SAU_VERSION,
            len(sectors),
            len(walls),
            len(enemies),
            num_textures,
            0  # Reserved
        )
        f.write(header)
        
        # Write sectors (12 bytes each)
        for sec in sectors:
            data = struct.pack('<hhhhhh',
                sec['ws'], sec['we'],
                sec['z1'], sec['z2'],
                sec['st'], sec['ss']
            )
            f.write(data)
        
        # Write walls (16 bytes each)
        for wall in walls:
            data = struct.pack('<hhhhhhhh',
                wall['x1'], wall['y1'],
                wall['x2'], wall['y2'],
                wall['wt'],
                wall['u'], wall['v'],
                wall['shade']
            )
            f.write(data)
        
        # Write player spawn (10 bytes)
        data = struct.pack('<hhhhh',
            player['x'], player['y'], player['z'],
            player['a'], player['l']
        )
        f.write(data)
        
        # Write enemies (8 bytes each)
        for enemy in enemies:
            data = struct.pack('<hhhh',
                enemy['x'], enemy['y'], enemy['z'],
                enemy['type']
            )
            f.write(data)
        
        # Write textures (NEW in v2)
        if textures:
            for tex in textures:
                name_bytes = tex['name'].encode('utf-8')[:31]  # Max 31 chars + null
                name_bytes = name_bytes.ljust(32, b'\x00')
                
                width = tex['width']
                height = tex['height']
                frame_count = tex.get('frame_count', 1)
                
                # Texture header: name(32) + width(2) + height(2) + frame_count(2) + reserved(2)
                tex_header = struct.pack('<32sHHHH',
                    name_bytes,
                    width,
                    height,
                    frame_count,
                    0  # Reserved
                )
                f.write(tex_header)
                
                # Write texture data for each frame
                if 'frames' in tex and frame_count > 1:
                    for frame in tex['frames']:
                        f.write(frame['data'])
                else:
                    f.write(tex['data'])
    
    # Calculate and display file info
    size = os.path.getsize(filename)
    print(f"Created SAU: {filename}")
    print(f"  Version: {SAU_VERSION}")
    print(f"  Sectors: {len(sectors)}")
    print(f"  Walls: {len(walls)}")
    print(f"  Enemies: {len(enemies)}")
    print(f"  Textures: {num_textures}")
    print(f"  Size: {size} bytes ({size/1024:.1f} KB)")


def read_sau(filename, extract_textures=False):
    """Read binary SAU file"""
    with open(filename, 'rb') as f:
        # Read header (check for v1 or v2)
        header_data = f.read(12)
        magic, version, num_sectors, num_walls, num_enemies = struct.unpack('<IHHHH', header_data)
        
        if magic != SAU_MAGIC:
            raise ValueError(f"Invalid SAU magic: {hex(magic)} (expected {hex(SAU_MAGIC)})")
        
        num_textures = 0
        if version >= 2:
            # Read additional v2 header fields
            extra = f.read(6)
            num_textures, _ = struct.unpack('<HI', extra)
        
        print(f"SAU Version: {version}")
        print(f"Sectors: {num_sectors}, Walls: {num_walls}, Enemies: {num_enemies}, Textures: {num_textures}")
        
        # Read sectors
        sectors = []
        for i in range(num_sectors):
            data = f.read(12)
            values = struct.unpack('<hhhhhh', data)
            sectors.append({
                'ws': values[0], 'we': values[1],
                'z1': values[2], 'z2': values[3],
                'st': values[4], 'ss': values[5]
            })
        
        # Read walls
        walls = []
        for i in range(num_walls):
            data = f.read(16)
            values = struct.unpack('<hhhhhhhh', data)
            walls.append({
                'x1': values[0], 'y1': values[1],
                'x2': values[2], 'y2': values[3],
                'wt': values[4],
                'u': values[5], 'v': values[6],
                'shade': values[7]
            })
        
        # Read player
        data = f.read(10)
        values = struct.unpack('<hhhhh', data)
        player = {
            'x': values[0], 'y': values[1], 'z': values[2],
            'a': values[3], 'l': values[4]
        }
        
        # Read enemies
        enemies = []
        for i in range(num_enemies):
            data = f.read(8)
            values = struct.unpack('<hhhh', data)
            enemies.append({
                'x': values[0], 'y': values[1], 'z': values[2],
                'type': values[3]
            })
        
        # Read textures (v2+)
        textures = []
        if version >= 2 and num_textures > 0:
            for i in range(num_textures):
                tex_header = f.read(40)
                name_bytes, width, height, frame_count, _ = struct.unpack('<32sHHHH', tex_header)
                name = name_bytes.rstrip(b'\x00').decode('utf-8')
                
                frames = []
                data_size = width * height * 3
                for frame_idx in range(frame_count):
                    frame_data = f.read(data_size)
                    frames.append(frame_data)
                
                textures.append({
                    'name': name,
                    'width': width,
                    'height': height,
                    'frame_count': frame_count,
                    'frames': frames,
                    'data': frames[0] if frames else b''
                })
        
        return sectors, walls, player, enemies, textures


def sau_to_level_h(sau_filename, output_filename):
    """Convert SAU back to level.h text format"""
    result = read_sau(sau_filename)
    if len(result) == 5:
        sectors, walls, player, enemies, textures = result
    else:
        sectors, walls, player, enemies = result
        textures = []
    
    with open(output_filename, 'w') as f:
        # Sectors
        f.write(f"{len(sectors)}\n")
        for sec in sectors:
            f.write(f"{sec['ws']} {sec['we']} {sec['z1']} {sec['z2']} {sec['st']} {sec['ss']}\n")
        
        # Walls
        f.write(f"{len(walls)}\n")
        for wall in walls:
            f.write(f"{wall['x1']} {wall['y1']} {wall['x2']} {wall['y2']} {wall['wt']} {wall['u']} {wall['v']} {wall['shade']}\n")
        
        # Player
        f.write(f"{player['x']} {player['y']} {player['z']} {player['a']} {player['l']}\n")
        
        # Enemies
        if enemies:
            f.write(f"\n{len(enemies)}\n")
            for enemy in enemies:
                f.write(f"{enemy['x']} {enemy['y']} {enemy['z']} {enemy['type']}\n")
    
    print(f"Converted to: {output_filename}")


def main():
    if len(sys.argv) < 2:
        print("=" * 60)
        print("SAU Builder for DoomClone (v2 - with texture support)")
        print("=" * 60)
        print()
        print("Usage:")
        print("  sau_builder.py <level.h>                    - Convert to level.sau")
        print("  sau_builder.py <level.h> <output.sau>       - Convert to specified SAU")
        print("  sau_builder.py <level.h> --textures <dir>   - Include textures from dir")
        print("  sau_builder.py --extract <file.sau>         - Extract SAU to level.h")
        print("  sau_builder.py --info <file.sau>            - Show SAU info")
        print()
        print("Examples:")
        print("  sau_builder.py level.h")
        print("  sau_builder.py level.h --textures textures/")
        print("  sau_builder.py level.h mylevel.sau --textures textures/")
        print("  sau_builder.py --extract mylevel.sau extracted.h")
        return
    
    if sys.argv[1] == '--extract' and len(sys.argv) >= 3:
        sau_file = sys.argv[2]
        output = sys.argv[3] if len(sys.argv) > 3 else sau_file.replace('.sau', '_extracted.h')
        sau_to_level_h(sau_file, output)
    elif sys.argv[1] == '--info' and len(sys.argv) >= 3:
        read_sau(sys.argv[2])
    else:
        input_file = sys.argv[1]
        output_file = None
        texture_dir = None
        
        # Parse arguments
        i = 2
        while i < len(sys.argv):
            if sys.argv[i] == '--textures' and i + 1 < len(sys.argv):
                texture_dir = sys.argv[i + 1]
                i += 2
            elif not output_file and not sys.argv[i].startswith('--'):
                output_file = sys.argv[i]
                i += 1
            else:
                i += 1
        
        if not output_file:
            base = os.path.splitext(input_file)[0]
            output_file = base + '.sau'
        
        if not output_file.endswith('.sau'):
            output_file += '.sau'
        
        try:
            sectors, walls, player, enemies = parse_level_h(input_file)
            
            # Load textures if directory specified
            textures = None
            if texture_dir:
                print(f"Loading textures from: {texture_dir}")
                textures = load_all_textures(texture_dir)
                print(f"Loaded {len(textures)} textures")
            
            write_sau(output_file, sectors, walls, player, enemies, textures)
        except Exception as e:
            print(f"Error: {e}")
            import traceback
            traceback.print_exc()
            sys.exit(1)


if __name__ == '__main__':
    main()
