import re
from pathlib import Path

def load_texture(file_path):
    print(f"Loading {file_path}...")
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            
        width_match = re.search(r'_WIDTH\s+(\d+)', content)
        height_match = re.search(r'_HEIGHT\s+(\d+)', content)
        
        if not width_match or not height_match:
            print("  Failed to find dimensions")
            return
            
        width = int(width_match.group(1))
        height = int(height_match.group(1))
        print(f"  Dimensions: {width}x{height}")
        
        data_match = re.search(r'\{(.*)\}', content, re.DOTALL)
        if not data_match:
            print("  Failed to find data block")
            return
            
        data_str = data_match.group(1)
        # Simple count check
        values = [x for x in data_str.split(',') if x.strip().isdigit()]
        print(f"  Found {len(values)} values")
        
        expected = width * height * 3
        if len(values) < expected:
            print(f"  WARNING: Expected {expected} values, found {len(values)}")
        else:
            print("  Data size looks correct")
            
    except Exception as e:
        print(f"  Error: {e}")

texture_dir = Path("d:/PROGRAM/VSPRO/DoomClone/tools/textures")
for f in texture_dir.glob("*.h"):
    load_texture(f)
