#!/usr/bin/env python3
import pygame
import sys
import math
import os
import re  # ADDED: for texture header parsing
import copy # ADDED: for undo/redo
import colorsys # ADDED: for color wheel
from enum import Enum
from typing import List, Tuple, Optional
from tkinter import Tk, filedialog

# Initialize Pygame
pygame.init()

# === CONSTANTS ===
WINDOW_WIDTH = 1600
WINDOW_HEIGHT = 900
FPS = 60

# Default level file path (relative to script's parent directory)
DEFAULT_LEVEL_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "level.h")
# FIXED: Resolve paths relative to the script/executable location
def get_base_path():
    """Get the base path for resources, works with PyInstaller and normal Python"""
    if getattr(sys, 'frozen', False):
        # Running as compiled executable (PyInstaller)
        return os.path.dirname(sys.executable)
    else:
        # Running as script
        return os.path.dirname(os.path.abspath(__file__))

BASE_PATH = get_base_path()
TEXTURE_DIR = os.path.join(BASE_PATH, "..", "textures")

# Colors (Dark Theme - Oracular Style)
class Colors:
    BG_DARK = (25, 25, 28)
    BG_MEDIUM = (45, 45, 48)
    BG_LIGHT = (63, 63, 70)
    GRID_DARK = (35, 35, 38)
    GRID_LIGHT = (55, 55, 58)
    GRID_AXIS_X = (180, 0, 0)
    GRID_AXIS_Y = (0, 180, 0)
    TEXT = (220, 220, 225)
    TEXT_DIM = (150, 150, 155)
    ACCENT = (0, 122, 204)
    ACCENT_BRIGHT = (0, 150, 255)
    SELECT = (255, 200, 0)
    SELECT_HOVER = (255, 230, 100)
    WALL = (180, 180, 190)
    WALL_SELECTED = (255, 255, 0)
    VERTEX = (255, 255, 255)
    VERTEX_SELECTED = (255, 200, 0)
    PLAYER = (0, 255, 0)
    PANEL_BG = (37, 37, 40)
    BUTTON = (0, 122, 204)
    BUTTON_HOVER = (28, 151, 234)
    BUTTON_ACTIVE = (0, 100, 180)
    SEPARATOR = (100, 100, 100)
    EDIT_BG = (50, 50, 55)  # ADDED: edit field background
    EDIT_BORDER = (0, 180, 255)  # ADDED: edit field border
    FLOOR = (50, 50, 50)
    CEILING = (30, 30, 30)

# Tool types
class Tool(Enum):
    SELECT = 0
    CREATE_SECTOR = 1
    VERTEX_EDIT = 2
    ENTITY = 3
    LIGHT = 4  # ADDED: Light editing tool
    GATE = 5   # ADDED: Gate/Switch editing tool

# Viewport types
class ViewType(Enum):
    TOP = 0
    FRONT = 1
    SIDE = 2
    CAMERA_3D = 3

# === DATA STRUCTURES ===
# Material types
MATERIAL_SOLID = 0
MATERIAL_GLASS = 1
MATERIAL_FENCE = 2

# Glass tint presets
GLASS_TINT_PRESETS = {
    "Clear": (220, 230, 240),
    "Blue": (150, 180, 220),
    "Green": (150, 200, 160),
    "Amber": (220, 180, 120),
    "Red": (220, 140, 140),
}

class Wall:
    def __init__(self, x1=0, y1=0, x2=0, y2=0, wt=0, u=1, v=1, shade=0,
                 material=0, tint_r=220, tint_g=230, tint_b=240, opacity=80):
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2
        self.wt = wt  # wall texture index
        self.u = u  # u tile / scale
        self.v = v  # v tile / scale
        self.shade = shade
        self.material = material  # 0=solid, 1=glass, 2=fence
        self.tint_r = tint_r  # Glass tint red
        self.tint_g = tint_g  # Glass tint green
        self.tint_b = tint_b  # Glass tint blue
        self.opacity = opacity  # Glass opacity (0-255)

class Sector:
    def __init__(self, ws=0, we=0, z1=0, z2=40, st=0, ss=4, tag=0):
        self.ws = ws  # wall start
        self.we = we  # wall end
        self.z1 = z1  # bottom height
        self.z2 = z2  # top height
        self.st = st  # surface texture index
        self.ss = ss  # surface scale
        self.tag = tag # 0=Normal, 1=Stair

class Player:
    def __init__(self, x=0, y=0, z=20, a=0, l=0):
        self.x = x
        self.y = y
        self.z = z
        self.a = a  # angle
        self.l = l  # look up/down

# ADDED: Enemy class
class Enemy:
    def __init__(self, x=0, y=0, z=0, enemy_type=0):
        self.x = x
        self.y = y
        self.z = z
        self.enemy_type = enemy_type # 0=BOSSA1, 1=BOSSA2, 2=BOSSA3
        self.active = 1

# ADDED: Pickup class
class Pickup:
    def __init__(self, x=0, y=0, z=0, pickup_type=0, respawns=1):
        self.x = x
        self.y = y
        self.z = z
        self.pickup_type = pickup_type
        self.respawns = respawns
        self.active = 1

# ADDED: Light class for dynamic lighting
class Light:
    def __init__(self, x=0, y=0, z=30, radius=150, intensity=200,
                 r=255, g=255, b=200, light_type=0, flicker=0):
        self.x = x
        self.y = y
        self.z = z
        self.radius = radius        # World units (reach of light)
        self.intensity = intensity  # Brightness 0-255
        self.r = r                  # Red component 0-255
        self.g = g                  # Green component 0-255
        self.b = b                  # Blue component 0-255
        self.light_type = light_type  # 0=Point, 1=Spot
        self.flicker = flicker      # 0=None, 1=Candle, 2=Strobe, 3=Pulse, 4=Random
        self.active = 1

# ADDED: Gate class for vertical doors
class Gate:
    def __init__(self, x=0, y=0, z_closed=0, z_open=40, gate_id=0,
                 trigger_radius=50, texture=0, speed=2, width=64):
        self.x = x
        self.y = y
        self.z_closed = z_closed    # Floor height when closed
        self.z_open = z_open        # Height when fully open
        self.gate_id = gate_id      # Unique ID for switch linking
        self.trigger_radius = trigger_radius  # Radius for R-key activation
        self.texture = texture      # Wall texture index
        self.speed = speed          # Movement speed
        self.width = width          # Gate width in world units
        self.active = 1

# ADDED: Switch class for gate triggers
class Switch:
    def __init__(self, x=0, y=0, z=20, linked_gate_id=0, texture=0):
        self.x = x
        self.y = y
        self.z = z
        self.linked_gate_id = linked_gate_id  # ID of gate to toggle
        self.texture = texture
        self.active = 1


# ADDED: Texture class

class Texture:
    def __init__(self, name: str, frames: List[pygame.Surface], frame_duration: int = 150):
        self.name = name
        self.frames = frames
        self.frame_duration = frame_duration
    
    def get_current_frame(self) -> pygame.Surface:
        if not self.frames:
            return pygame.Surface((64, 64)) # Fallback
        if len(self.frames) == 1:
            return self.frames[0]
        
        # Calculate frame based on time
        current_time = pygame.time.get_ticks()
        frame_idx = (current_time // self.frame_duration) % len(self.frames)
        return self.frames[frame_idx]

# === VIEWPORT CLASS ===
class Viewport:
    def __init__(self, rect, view_type, name):
        self.rect = rect
        self.view_type = view_type
        self.name = name
        self.offset_x = 0
        self.offset_y = 0
        self.zoom = 4.0  # pixels per unit
        self.grid_size = 8
        self.is_active = False
        
    def world_to_screen(self, wx, wy):
        """Convert world coordinates to screen coordinates"""
        sx = self.rect.x + self.rect.width // 2 + (wx - self.offset_x) * self.zoom
        sy = self.rect.y + self.rect.height // 2 - (wy - self.offset_y) * self.zoom
        return int(sx), int(sy)
    
    def screen_to_world(self, sx, sy):
        """Convert screen coordinates to world coordinates"""
        wx = (sx - self.rect.x - self.rect.width // 2) / self.zoom + self.offset_x
        wy = -(sy - self.rect.y - self.rect.height // 2) / self.zoom + self.offset_y
        return wx, wy
    
    def snap_to_grid(self, wx, wy):
        """Snap world coordinates to grid"""
        return (round(wx / self.grid_size) * self.grid_size,
                round(wy / self.grid_size) * self.grid_size)

# === MAIN EDITOR CLASS ===
class OracularEditor:
    def __init__(self):
        self.width = WINDOW_WIDTH
        self.height = WINDOW_HEIGHT
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        pygame.display.set_caption("Oracular Editor")
        self.clock = pygame.time.Clock()
        self.running = True
        
        # Notification state
        self.notification_message = ""
        self.notification_timer = 0.0
        self.notification_duration = 2.0 # seconds
        
        # Fonts
        self.font_small = pygame.font.SysFont('Segoe UI', 12)
        self.font_medium = pygame.font.SysFont('Segoe UI', 14)
        self.font_large = pygame.font.SysFont('Segoe UI', 16, bold=True)
        self.font_title = pygame.font.SysFont('Segoe UI', 20, bold=True)
        
        # UI Layout Constants
        self.TOOLBAR_WIDTH = 60
        self.STATUS_BAR_HEIGHT = 25
        
        # Load Tool Icons
        self.tool_icons = {}
        icon_dir = os.path.join(BASE_PATH, "icons")
        for tool, filename in [
            (Tool.SELECT, "select.png"),
            (Tool.CREATE_SECTOR, "sector.png"),
            (Tool.VERTEX_EDIT, "vertex.png"),
            (Tool.ENTITY, "entity.png"),
            (Tool.LIGHT, "light.png")  # ADDED: Light tool icon
        ]:
            try:
                path = os.path.join(icon_dir, filename)
                if os.path.exists(path):
                    icon = pygame.image.load(path).convert_alpha()
                    self.tool_icons[tool] = pygame.transform.smoothscale(icon, (32, 32))
            except Exception as e:
                print(f"Failed to load icon {filename}: {e}")
                
        # Status Bar State
        self.status_message = ""
        self.status_timer = 0
        
        # Load Logo
        self.logo_img = None
        self.logo_text_img = None
        try:
            logo_path = os.path.join(BASE_PATH, "oracular_logo.png")
            if os.path.exists(logo_path):
                self.logo_img = pygame.image.load(logo_path).convert_alpha()
                # Auto-remove background (assume top-left pixel is bg)
                bg_color = self.logo_img.get_at((0, 0))
                self.logo_img.set_colorkey(bg_color)
            
            text_path = os.path.join(BASE_PATH, "oracular_text.png")
            if os.path.exists(text_path):
                self.logo_text_img = pygame.image.load(text_path).convert_alpha()
                # Auto-remove background for text too
                bg_color_text = self.logo_text_img.get_at((0, 0))
                self.logo_text_img.set_colorkey(bg_color_text)
        except Exception as e:
            print(f"Warning: Could not load logo: {e}")
        # 3D View State
        self.textured_view = False

        self.current_level_path = DEFAULT_LEVEL_PATH
        
        self.current_tool = Tool.SELECT
        self.sectors = []
        self.walls = []
        self.player = Player()
        self.selected_sector = None
        self.selected_wall = None
        self.selected_vertices = []
        
        # ADDED: Enemy list and selection
        self.enemies = []
        self.selected_enemy = None
        
        # ADDED: Pickup list and selection
        self.pickups = []
        self.selected_pickup = None
        
        self.show_grid = True
        self.snap_to_grid = True
        
        self.active_menu = None
        self.menu_items = {
            "File": ["New (Ctrl+N)", "Open (Ctrl+O)", "Save (Ctrl+S)", "---", "Exit"],
            "Edit": ["Undo (Ctrl+Z)", "Redo (Ctrl+Y)", "---", "Delete (Del)", "---", "Select All"],
            "View": ["Toggle Grid (G)", "Toggle Snap (Shift+S)"],
            "Tools": ["Select (1)", "Create Sector (2)", "Vertex Edit (3)", "Entity (4)", "Light (5)", "Gate (6)"],
            "Help": ["About"]
        }
        
        self.creating_sector = False
        self.sector_vertices = []
        
        self.prop_wall_texture = 0
        self.prop_wall_u = 1
        self.prop_wall_v = 1
        self.prop_wall_material = 0  # 0=solid, 1=glass, 2=fence
        self.prop_wall_tint_preset = "Clear"  # Tint preset name
        self.prop_wall_tint_r = 220
        self.prop_wall_tint_g = 230
        self.prop_wall_tint_b = 240
        self.prop_wall_opacity = 80  # 0-255
        self.prop_sector_z1 = 0
        self.prop_sector_z2 = 40
        self.prop_sector_texture = 0
        self.prop_sector_scale = 4
        self.prop_sector_tag = 0
        
        # ADDED: Enemy defaults
        self.prop_enemy_type = 0
        
        # ADDED: Pickup defaults
        self.prop_pickup_type = 0
        self.entity_mode = "ENEMY" # "ENEMY" or "PICKUP"
        
        # ADDED: Light list and selection
        self.lights: List[Light] = []
        self.selected_light = None
        
        # ADDED: Light property defaults
        self.prop_light_r = 255
        self.prop_light_g = 255
        self.prop_light_b = 200  # Warm white default
        self.prop_light_radius = 150
        self.prop_light_intensity = 200
        self.prop_light_type = 0  # 0=Point, 1=Spot
        self.prop_light_flicker = 0  # 0=None, 1=Candle, 2=Strobe, 3=Pulse
        
        # ADDED: Gate list and selection
        self.gates: List[Gate] = []
        self.selected_gate = None
        
        # ADDED: Switch list and selection
        self.switches: List[Switch] = []
        self.selected_switch = None
        
        # ADDED: Gate property defaults
        self.prop_gate_id = 0
        self.prop_gate_z_closed = 0
        self.prop_gate_z_open = 40
        self.prop_gate_speed = 2
        self.prop_gate_trigger_radius = 50
        self.prop_gate_texture = 0
        self.prop_gate_width = 64
        
        # ADDED: Switch property defaults
        self.prop_switch_linked_gate_id = 0
        self.prop_switch_texture = 0
        
        # ADDED: Gate/Switch mode toggle
        self.gate_mode = "GATE"  # "GATE" or "SWITCH"
        
        # Light type and flicker names for UI
        self.light_type_names = ["Point", "Spot"]
        self.flicker_names = ["None", "Candle", "Strobe", "Pulse", "Random"]
        
        # Color presets for quick selection
        self.light_presets = [
            ("Warm White", 255, 255, 200),
            ("Cool White", 220, 240, 255),
            ("Red Alarm", 255, 50, 50),
            ("Blue", 50, 100, 255),
            ("Green", 50, 255, 100),
            ("Orange Fire", 255, 150, 50),
            ("Purple", 200, 50, 255),
        ]
        
        # ADDED: textures
        self.textures: List[Texture] = []
        self.enemy_textures: List[Texture] = [] # ADDED: Enemy textures
        self.pickup_textures: List[Texture] = [] # ADDED: Pickup textures
        self.load_textures()
        
        # Setup viewports (4-way layout)
        self.setup_viewports()
        self.active_viewport = self.viewports[0]
        
        # Mouse state
        self.mouse_down = False
        self.last_mouse_pos = (0, 0)
        self.hover_vertex = None
        self.hover_wall = None
        
        # ADDED: property edit state
        self.property_items = []  # list of dicts with rect & metadata
        self.edit_field = None  # currently editing item dict
        self.edit_buffer = ""
        
        # ADDED: Undo/Redo stacks
        self.undo_stack = []
        self.redo_stack = []
        
        # ADDED: Box Selection state
        self.selecting_box = False
        self.box_start = (0, 0)
        self.box_end = (0, 0)
        
        # ADDED: buttons for texture navigation
        self.texture_nav_buttons = []  # list of (rect, delta)
        
        # ADDED: Color Picker State
        self.picker_active_drag = None # 'hue' or 'sv' or None
        self.picker_hue_surf = None
        self.picker_sv_sat_mask = None
        self.picker_sv_val_mask = None
        
        self.init_color_picker_resources()

        


    # ADDED: texture loader
    def parse_texture_file(self, filename, var_names=None):
        """Parses a texture .h file and returns a list of surfaces found."""
        path = os.path.join(TEXTURE_DIR, filename)
        if not os.path.exists(path):
            print(f"Texture file not found: {path}")
            return []
            
        try:
            with open(path, 'r') as f:
                data = f.read()
                
            surfaces = []
            
            # Parse dimension arrays first (int arrays)
            dim_arrays = {}
            int_array_matches = list(re.finditer(r'(?:static\s+)?(?:const\s+)?int\s+([a-zA-Z0-9_]+)(?:\[\d*\])?\s*=\s*\{([^}]*)\}', data, re.DOTALL))
            for match in int_array_matches:
                name = match.group(1)
                content = match.group(2)
                vals = []
                for num in re.findall(r'\b\d+\b', content):
                    vals.append(int(num))
                dim_arrays[name] = vals

            # Find all char array definitions (texture data)
            array_matches = list(re.finditer(r'(?:static\s+)?(?:const\s+)?(?:unsigned\s+)?char\s+([a-zA-Z0-9_]+)(?:\[\])?\s*=\s*\{([^}]*)\}', data, re.DOTALL))
            
            for match in array_matches:
                name = match.group(1)
                content = match.group(2)
                
                if var_names and name not in var_names:
                    continue
                    
                # Find dimensions for this array
                width = 0
                height = 0
                
                # 1. Look for #define NAME_WIDTH 64
                w_match = re.search(f'#define\\s+{name}_WIDTH\\s+(\\d+)', data)
                h_match = re.search(f'#define\\s+{name}_HEIGHT\\s+(\\d+)', data)
                
                # 1b. Look for #define NAME_FRAME_WIDTH (common for animated textures)
                if not w_match:
                    # Try stripping _frame_X suffix
                    base_name = re.sub(r'_frame_\d+$', '', name)
                    print(f"DEBUG: name={name}, base_name={base_name}")
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
                    # 2. Fallback: try finding generic WIDTH/HEIGHT
                    if not w_match:
                        w_match = re.search(r'#define\\s+[A-Z0-9_]+_WIDTH\\s+(\\d+)', data)
                    if not h_match:
                        h_match = re.search(r'#define\\s+[A-Z0-9_]+_HEIGHT\\s+(\\d+)', data)
                        
                    if w_match and h_match:
                        width = int(w_match.group(1))
                        height = int(h_match.group(1))
                    else:
                        # 3. Fallback: Check for array-based dimensions
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
                    print(f"Could not find dimensions for {name} in {filename}")
                    continue
                    
                # Parse pixel data
                pixel_vals = []
                for num in re.findall(r'\b\d+\b', content):
                    pixel_vals.append(int(num))
                    
                expected = width * height * 3
                if len(pixel_vals) < expected:
                    pixel_vals += [0] * (expected - len(pixel_vals))
                    
                surface = pygame.Surface((width, height))
                idx = 0
                for y in range(height):
                    for x in range(width):
                        if idx + 2 < len(pixel_vals):
                            r = pixel_vals[idx]
                            g = pixel_vals[idx+1]
                            b = pixel_vals[idx+2]
                            surface.set_at((x, y), (r, g, b))
                        idx += 3
                
                surfaces.append((name, surface))
                
            # Sort surfaces if var_names provided
            if var_names:
                ordered_surfaces = []
                surface_map = {name: surf for name, surf in surfaces}
                for name in var_names:
                    if name in surface_map:
                        ordered_surfaces.append(surface_map[name])
                return ordered_surfaces
            
            return [s[1] for s in surfaces]
            
        except Exception as e:
            print(f"Error parsing {filename}: {e}")
            return []

    def load_textures(self):
        """Load textures in the specific order used by the game."""
        self.textures = []
        
        # 1. Load static textures (0-6)
        static_textures = ["T_00.h", "T_01.h", "T_02.h", "T_03.h", "T_04.h", "T_05.h", "T_06.h"]
        for i, tex_file in enumerate(static_textures):
            surfaces = self.parse_texture_file(tex_file)
            if surfaces:
                self.textures.append(Texture(f"Texture {i}", [surfaces[0]]))
            else:
                # Placeholder
                s = pygame.Surface((64, 64))
                s.fill((255, 0, 255))
                self.textures.append(Texture(f"Texture {i} (MISSING)", [s]))
                
        # 2. Load WALL57 (Animated, index 7)
        # Frames: WALL57_2, WALL57_3, WALL57_4, WALL57_3
        # These are in separate files
        wall57_frames = []
        
        f2 = self.parse_texture_file("WALL57_2.h", ["WALL57_2"])
        f3 = self.parse_texture_file("WALL57_3.h", ["WALL57_3"])
        f4 = self.parse_texture_file("WALL57_4.h", ["WALL57_4"])
        
        if f2 and f3 and f4:
            wall57_frames = [f2[0], f3[0], f4[0], f3[0]]
            self.textures.append(Texture("WALL57 (Anim)", wall57_frames, 150))
        else:
            s = pygame.Surface((64, 64))
            s.fill((255, 255, 0))
            self.textures.append(Texture("WALL57 (MISSING)", [s]))

        # 3. Load WALL58 (Animated, index 8)
        # Frames: WALL58_frame_0, WALL58_frame_1, WALL58_frame_2 in WALL58.h
        wall58_frames = self.parse_texture_file("WALL58.h", ["WALL58_frame_0", "WALL58_frame_1", "WALL58_frame_2"])
        if wall58_frames:
            self.textures.append(Texture("WALL58 (Anim)", wall58_frames, 150))
        else:
            s = pygame.Surface((64, 64))
            s.fill((0, 255, 255))
            self.textures.append(Texture("WALL58 (MISSING)", [s]))
            
        print(f"Loaded {len(self.textures)} textures in game order.")

        # 4. Load Enemy Textures
        # BOSSA1
        bossa1_frames = self.parse_texture_file("BOSSA1.h", ["BOSSA1"])
        if bossa1_frames:
            self.enemy_textures.append(Texture("BOSSA1", bossa1_frames))
        else:
            s = pygame.Surface((64, 64)); s.fill((255, 0, 0))
            self.enemy_textures.append(Texture("BOSSA1 (MISSING)", [s]))
            
        # BOSSA2 (using walk frames as default visualization)
        bossa2_frames = self.parse_texture_file("BOSSA2_walk.h", ["BOSSA2_frame_0", "BOSSA2_frame_1"])
        if bossa2_frames:
            self.enemy_textures.append(Texture("BOSSA2", bossa2_frames, 200))
        else:
            s = pygame.Surface((64, 64)); s.fill((0, 255, 255))
            self.enemy_textures.append(Texture("BOSSA2 (MISSING)", [s]))
            
        # BOSSA3 (using walk frames)
        bossa3_frames = self.parse_texture_file("BOSSA3_walk.h", ["BOSSA3_frame_0", "BOSSA3_frame_1"])
        if bossa3_frames:
            self.enemy_textures.append(Texture("BOSSA3", bossa3_frames, 200))
        else:
            s = pygame.Surface((64, 64)); s.fill((255, 0, 255))
            self.enemy_textures.append(Texture("BOSSA3 (MISSING)", [s]))
            
        # Cacodemon (Index 3)
        cace_frames = self.parse_texture_file("cace_stat.h", ["CACE_STAT"])
        if cace_frames:
            self.enemy_textures.append(Texture("Cacodemon", cace_frames))
        else:
            s = pygame.Surface((64, 64)); s.fill((255, 50, 0)) # Orange/Red
            self.enemy_textures.append(Texture("Cacodemon (MISSING)", [s]))
            
        print(f"Loaded {len(self.enemy_textures)} enemy textures.")

        # 5. Load Pickup Textures
        # Load specific sprites
        health_frames = self.parse_texture_file("health.h", ["HEALTH_frame_0", "HEALTH_frame_1", "HEALTH_frame_2"])
        armour_frames = self.parse_texture_file("armour.h", ["ARMOUR_frame_0", "ARMOUR_frame_1", "ARMOUR_frame_2", "ARMOUR_frame_3"])
        bl_key_frames = self.parse_texture_file("bl_key.h", ["BL_KEY_frame_0", "BL_KEY_frame_1"])
        
        # Helper to get surface or fallback
        def get_surf(frames, idx=0, fallback_color=(255,255,255)):
            if frames and idx < len(frames): return frames[idx]
            s = pygame.Surface((32, 32)); s.fill(fallback_color); return s

        # Pickup Textures (Mapped to ID)
        # 0: PICKUP_HEALTH_SMALL (Red) -> Use frame 0 of health
        # 1: PICKUP_HEALTH_LARGE (Red) -> Use frame 0 of health
        # 2: PICKUP_ARMOR_SMALL (Blue) -> Use frame 0 of armour
        # 3: PICKUP_ARMOR_LARGE (Blue) -> Use frame 0 of armour
        # 4: PICKUP_AMMO_CLIP (Yellow)
        # 5: PICKUP_AMMO_SHELLS (Yellow)
        # 6: PICKUP_AMMO_BULLETS (Yellow)
        # 7: PICKUP_BERSERK (Magenta)
        # 8: PICKUP_INVULN (Cyan)
        # 9: PICKUP_SPEED (Green)
        # 10: PICKUP_KEY_BLUE (Blue Key)
        
        # 0: Health Small
        self.pickup_textures.append(Texture("Health Small", [get_surf(health_frames, 0, (255, 50, 50))], 300))
        # 1: Health Large
        self.pickup_textures.append(Texture("Health Large", health_frames if health_frames else [get_surf(health_frames, 0, (255, 50, 50))], 300))

        # 2: Armor Small
        self.pickup_textures.append(Texture("Armor Small", [get_surf(armour_frames, 0, (50, 100, 255))], 300))
        # 3: Armor Large
        self.pickup_textures.append(Texture("Armor Large", armour_frames if armour_frames else [get_surf(armour_frames, 0, (50, 100, 255))], 300))

        # Placeholders for Ammo/Powerups
        pickup_names_rest = [
            "Ammo Clip", "Ammo Shells", "Ammo Bullets",
            "Berserk", "Invuln", "Speed"
        ]
        
        for i, name in enumerate(pickup_names_rest):
            idx = i + 4 # Start at index 4
            s = pygame.Surface((32, 32))
            # Color coding
            if idx in [4, 5, 6]: s.fill((255, 200, 50)) # Yellow (Ammo)
            elif idx == 7: s.fill((255, 0, 128))          # Magenta (Berserk)
            elif idx == 8: s.fill((0, 255, 200))          # Cyan (Invuln)
            elif idx == 9: s.fill((0, 255, 0))            # Green (Speed)
            else: s.fill((200, 200, 200))
            
            # Draw a simple shape to differentiate
            pygame.draw.rect(s, (0, 0, 0), (0, 0, 32, 32), 1)
            self.pickup_textures.append(Texture(name, [s]))
            
        # 10: Blue Key
        self.pickup_textures.append(Texture("Blue Key", bl_key_frames if bl_key_frames else [get_surf(bl_key_frames, 0, (0, 0, 255))], 500))

    def setup_viewports(self):
        """Setup 4-way viewport layout"""
        # Calculate viewport sizes
        toolbar_width = self.TOOLBAR_WIDTH
        properties_width = 280
        menu_height = 30
        status_height = self.STATUS_BAR_HEIGHT
        separator = 2
        
        available_width = WINDOW_WIDTH - toolbar_width - properties_width - 3 * separator
        available_height = WINDOW_HEIGHT - menu_height - status_height - 3 * separator
        
        vp_width = available_width // 2
        vp_height = available_height // 2
        
        x_start = toolbar_width + separator
        y_start = menu_height + separator
        
        # Create 4 viewports: Top, Front, Side, 3D
        self.viewports = [
            Viewport(pygame.Rect(x_start, y_start, vp_width, vp_height),
                    ViewType.TOP, "Top (XY)"),
            Viewport(pygame.Rect(x_start + vp_width + separator, y_start, vp_width, vp_height),
                    ViewType.FRONT, "Front (XZ)"),
            Viewport(pygame.Rect(x_start, y_start + vp_height + separator, vp_width, vp_height),
                    ViewType.SIDE, "Side (YZ)"),
            Viewport(pygame.Rect(x_start + vp_width + separator, y_start + vp_height + separator,
                               vp_width, vp_height),
                    ViewType.CAMERA_3D, "Camera (3D)")
        ]
        
        # Set default zoom and offsets for different views
        self.viewports[0].zoom = 2.0  # Top view - zoomed out more to see full level
        self.viewports[1].zoom = 2.0  # Front view
        self.viewports[2].zoom = 2.0  # Side view
        self.viewports[3].zoom = 2.0  # 3D view
    
    def center_viewports_on_level(self):
        """Center all viewports on the loaded level geometry"""
        if len(self.walls) == 0:
            # No geometry, center on player
            for vp in self.viewports[:3]:  # Only 2D views
                vp.offset_x = self.player.x
                vp.offset_y = self.player.y
            return
        
        # Calculate bounding box of all walls
        min_x = min(min(w.x1, w.x2) for w in self.walls)
        max_x = max(max(w.x1, w.x2) for w in self.walls)
        min_y = min(min(w.y1, w.y2) for w in self.walls)
        max_y = max(max(w.y1, w.y2) for w in self.walls)
        
        # Calculate center
        center_x = (min_x + max_x) / 2
        center_y = (min_y + max_y) / 2
        
        # Calculate level size
        level_width = max_x - min_x
        level_height = max_y - min_y
        
        # Set offsets for 2D views
        self.viewports[0].offset_x = center_x  # Top view
        self.viewports[0].offset_y = center_y
        
        self.viewports[1].offset_x = center_x  # Front view
        self.viewports[1].offset_y = 0  # Center vertically on Z=0
        
        self.viewports[2].offset_x = center_y  # Side view (uses Y as horizontal)
        self.viewports[2].offset_y = 0  # Center vertically on Z=0
        
        # Adjust zoom to fit the level
        if level_width > 0 and level_height > 0:
            # Calculate zoom to fit level in viewport with some padding
            zoom_x = (self.viewports[0].rect.width * 0.8) / level_width
            zoom_y = (self.viewports[0].rect.height * 0.8) / level_height
            zoom = min(zoom_x, zoom_y, 4.0)  # Cap at 4.0 for reasonable detail
            zoom = max(zoom, 0.5)  # Minimum zoom
            
            # Apply zoom to all 2D views
            for vp in self.viewports[:3]:
                vp.zoom = zoom
        
    def show_about_dialog(self):
        """Show the About dialog with logo"""
        dialog_width = 500
        dialog_height = 400
        x = (WINDOW_WIDTH - dialog_width) // 2
        y = (WINDOW_HEIGHT - dialog_height) // 2
        
        # Create a surface for the dialog to allow for a modal loop or just overlay
        # For simplicity, we'll run a mini-loop here to block interaction
        running = True
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE or event.key == pygame.K_RETURN:
                        running = False
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    mx, my = pygame.mouse.get_pos()
                    # Close if clicked outside or on a close button (not implemented)
                    # For now, just click anywhere to close
                    running = False
            
            # Draw overlay
            overlay = pygame.Surface((WINDOW_WIDTH, WINDOW_HEIGHT))
            overlay.fill((0, 0, 0))
            overlay.set_alpha(150)
            self.screen.blit(overlay, (0, 0))
            
            # Draw Dialog
            rect = pygame.Rect(x, y, dialog_width, dialog_height)
            pygame.draw.rect(self.screen, Colors.PANEL_BG, rect)
            pygame.draw.rect(self.screen, Colors.ACCENT, rect, 2)
            
            # Draw Logo
            current_y = y + 20
            if self.logo_img:
                logo_x = x + (dialog_width - self.logo_img.get_width()) // 2
                self.screen.blit(self.logo_img, (logo_x, current_y))
                current_y += self.logo_img.get_height() + 20
            else:
                current_y += 50
            
            # Draw Text
            title = self.font_title.render("Oracular Editor", True, Colors.ACCENT_BRIGHT)
            subtitle = self.font_medium.render("Open Source Doom-Style Level Editor", True, Colors.TEXT)
            
            self.screen.blit(title, (x + (dialog_width - title.get_width()) // 2, current_y))
            current_y += 30
            self.screen.blit(subtitle, (x + (dialog_width - subtitle.get_width()) // 2, current_y))
            
            pygame.display.flip()
            self.clock.tick(60)

    def run(self):
        """Main editor loop"""
        while self.running:
            self.handle_events()
            self.handle_continuous_input()  # Add continuous input handling
            self.update()
            self.render()
            self.clock.tick(FPS)
        
        pygame.quit()
        sys.exit()
    
    def handle_continuous_input(self):
        """Handle continuous keyboard input for smooth camera movement"""
        if not self.active_viewport or self.active_viewport.view_type != ViewType.CAMERA_3D:
            return
            
        # Ignore if Ctrl is pressed (to avoid conflict with shortcuts like Ctrl+S)
        if pygame.key.get_mods() & pygame.KMOD_CTRL:
            return
        
        keys = pygame.key.get_pressed()
        move_speed = 10
        turn_speed = 5
        
        # Movement
        if keys[pygame.K_w]:
            # Move forward
            self.player.x += int(math.sin(math.radians(self.player.a)) * move_speed)
            self.player.y += int(math.cos(math.radians(self.player.a)) * move_speed)
        if keys[pygame.K_s]:
            # Move backward
            self.player.x -= int(math.sin(math.radians(self.player.a)) * move_speed)
            self.player.y -= int(math.cos(math.radians(self.player.a)) * move_speed)
        if keys[pygame.K_a]:
            # Strafe left
            self.player.x -= int(math.cos(math.radians(self.player.a)) * move_speed)
            self.player.y += int(math.sin(math.radians(self.player.a)) * move_speed)
        if keys[pygame.K_d]:
            # Strafe right
            self.player.x += int(math.cos(math.radians(self.player.a)) * move_speed)
            self.player.y -= int(math.sin(math.radians(self.player.a)) * move_speed)
        if keys[pygame.K_q]:
            # Move up
            self.player.z -= move_speed
        if keys[pygame.K_e]:
            # Move down
            self.player.z += move_speed
        
        # Camera rotation
        if keys[pygame.K_LEFT]:
            # Turn left
            self.player.a = (self.player.a - turn_speed) % 360
        if keys[pygame.K_RIGHT]:
            # Turn right
            self.player.a = (self.player.a + turn_speed) % 360
        if keys[pygame.K_UP]:
            # Look up
            self.player.l -= turn_speed
        if keys[pygame.K_DOWN]:
            # Look down
            self.player.l += turn_speed
    
    def handle_events(self):
        """Handle input events"""
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            
            elif event.type == pygame.KEYDOWN:
                self.handle_keypress(event)
            
            elif event.type == pygame.MOUSEBUTTONDOWN:
                self.handle_mouse_down(event)
            
            elif event.type == pygame.MOUSEBUTTONUP:
                self.handle_mouse_up(event)
            
            elif event.type == pygame.MOUSEMOTION:
                self.handle_mouse_motion(event)
            
            elif event.type == pygame.MOUSEWHEEL:
                self.handle_mouse_wheel(event)
    
    # ADDED: Undo/Redo methods
        if len(self.undo_stack) > 50:
            self.undo_stack.pop(0)

    def save_undo_snapshot(self):
        """Save current state to undo stack"""
        state = {
            'sectors': copy.deepcopy(self.sectors),
            'walls': copy.deepcopy(self.walls),
            'player': copy.deepcopy(self.player),
            'enemies': copy.deepcopy(self.enemies),
            'pickups': copy.deepcopy(self.pickups),
            'lights': copy.deepcopy(self.lights)
        }
        self.undo_stack.append(state)
        self.redo_stack.clear()
        # Limit stack size
        if len(self.undo_stack) > 50:
            self.undo_stack.pop(0)


    def undo(self):
        """Undo last action"""
        if not self.undo_stack:
            print("Nothing to undo")
            return
            
        # Save current state to redo stack
        current_state = {
            'sectors': copy.deepcopy(self.sectors),
            'walls': copy.deepcopy(self.walls),
            'player': copy.deepcopy(self.player),
            'enemies': copy.deepcopy(self.enemies),
            'pickups': copy.deepcopy(self.pickups),
            'lights': copy.deepcopy(self.lights)
        }
        self.redo_stack.append(current_state)
        
        # Restore state
        state = self.undo_stack.pop()
        self.sectors = state['sectors']
        self.walls = state['walls']
        self.player = state['player']
        self.enemies = state.get('enemies', [])
        self.pickups = state.get('pickups', [])
        self.lights = state.get('lights', [])
        
        # Reset selection if invalid
        if self.selected_sector is not None and self.selected_sector >= len(self.sectors):
            self.selected_sector = None
        if self.selected_wall is not None and self.selected_wall >= len(self.walls):
            self.selected_wall = None
        if self.selected_enemy is not None and self.selected_enemy >= len(self.enemies):
            self.selected_enemy = None
        if self.selected_pickup is not None and self.selected_pickup >= len(self.pickups):
            self.selected_pickup = None
        if self.selected_light is not None and self.selected_light >= len(self.lights):
            self.selected_light = None
        self.selected_vertices = [] # Clear vertex selection to be safe
        
        print("Undo performed")

    def redo(self):
        """Redo last undone action"""
        if not self.redo_stack:
            print("Nothing to redo")
            return
            
        # Save current state to undo stack
        current_state = {
            'sectors': copy.deepcopy(self.sectors),
            'walls': copy.deepcopy(self.walls),
            'player': copy.deepcopy(self.player),
            'enemies': copy.deepcopy(self.enemies),
            'pickups': copy.deepcopy(self.pickups),
            'lights': copy.deepcopy(self.lights)
        }
        self.undo_stack.append(current_state)
        
        # Restore state
        state = self.redo_stack.pop()
        self.sectors = state['sectors']
        self.walls = state['walls']
        self.player = state['player']
        self.enemies = state.get('enemies', [])
        self.pickups = state.get('pickups', [])
        self.lights = state.get('lights', [])
        
        print("Redo performed")

    # ADDED: commit property edit
    def commit_property_edit(self):
        if not self.edit_field:
            return
        try:
            val = int(self.edit_buffer) if self.edit_buffer.strip() != '' else None
        except ValueError:
            val = None
        target = self.edit_field.get('target')
        attr = self.edit_field.get('attr')
        
        # Check if value actually changed
        current_val = getattr(target, attr) if target and attr else None
        if val is not None and target and attr and val != current_val:
            self.save_undo_snapshot() # Save before changing
            setattr(target, attr, val)
            # Keep props in sync
            if isinstance(target, Sector):
                self.prop_sector_z1 = target.z1
                self.prop_sector_z2 = target.z2
                self.prop_sector_texture = target.st
                self.prop_sector_scale = target.ss
            elif isinstance(target, Wall):
                self.prop_wall_texture = target.wt
                self.prop_wall_u = target.u
                self.prop_wall_v = target.v
            elif isinstance(target, Enemy):
                self.prop_enemy_type = target.enemy_type
            elif isinstance(target, Light):
                self.prop_light_r = target.r
                self.prop_light_g = target.g
                self.prop_light_b = target.b
                self.prop_light_radius = target.radius
                self.prop_light_intensity = target.intensity
                self.prop_light_flicker = target.flicker
        self.edit_field = None
        self.edit_buffer = ""

    def handle_keypress(self, event):
        # Undo/Redo shortcuts
        if event.key == pygame.K_z and pygame.key.get_mods() & pygame.KMOD_CTRL:
            self.undo()
            return
        if event.key == pygame.K_y and pygame.key.get_mods() & pygame.KMOD_CTRL:
            self.redo()
            return

        # If editing a field, capture numeric input
        if self.edit_field:
            if event.key == pygame.K_RETURN:
                self.commit_property_edit(); return
            elif event.key == pygame.K_ESCAPE:
                self.edit_field = None; self.edit_buffer = ""; return
            elif event.key == pygame.K_BACKSPACE:
                self.edit_buffer = self.edit_buffer[:-1]
                # self.commit_property_edit() # REMOVED: Don't commit on every keystroke for text entry
                return
            else:
                if event.unicode.isdigit() or (event.unicode == '-' and not self.edit_buffer):
                    self.edit_buffer += event.unicode
                    # self.commit_property_edit() # REMOVED: Don't commit on every keystroke for text entry
                return
        if event.key == pygame.K_1:
            self.current_tool = Tool.SELECT
        elif event.key == pygame.K_2:
            self.current_tool = Tool.CREATE_SECTOR
            self.selected_sector = None
            self.selected_wall = None
            self.selected_vertices = []
            self.selected_enemy = None
        elif event.key == pygame.K_3:
            self.current_tool = Tool.VERTEX_EDIT
        elif event.key == pygame.K_4:
            self.current_tool = Tool.ENTITY
        elif event.key == pygame.K_5:
            self.current_tool = Tool.LIGHT
        elif event.key == pygame.K_6:
            self.current_tool = Tool.GATE
        elif event.key == pygame.K_g:

            self.show_grid = not self.show_grid
        elif event.key == pygame.K_s and pygame.key.get_mods() & pygame.KMOD_SHIFT:
            self.snap_to_grid = not self.snap_to_grid
        elif event.key == pygame.K_s and pygame.key.get_mods() & pygame.KMOD_CTRL:
            self.save_level()
        elif event.key == pygame.K_o and pygame.key.get_mods() & pygame.KMOD_CTRL:
            self.open_level_dialog()
        elif event.key == pygame.K_n and pygame.key.get_mods() & pygame.KMOD_CTRL:
            self.new_level()
        elif event.key == pygame.K_DELETE:
            self.save_undo_snapshot() # Save before delete
            self.delete_selected()
        elif event.key == pygame.K_ESCAPE:
            if self.creating_sector:
                self.cancel_sector_creation()
        elif event.key == pygame.K_TAB:
            self.textured_view = not self.textured_view
            self.status_message = f"3D View: {'Textured' if self.textured_view else 'Wireframe'}"
            self.status_timer = 120
            
        # ADDED: Height adjustment shortcuts (4 units)
        elif event.key == pygame.K_LEFTBRACKET:
            # Decrease height
            self.save_undo_snapshot()
            if pygame.key.get_mods() & pygame.KMOD_SHIFT:
                # Ceiling
                if self.selected_sector is not None:
                    self.sectors[self.selected_sector].z2 -= 4
                    self.prop_sector_z2 = self.sectors[self.selected_sector].z2
                else:
                    self.prop_sector_z2 -= 4
                self.status_message = "Ceiling Lowered (-4)"
            else:
                # Floor
                if self.selected_sector is not None:
                    self.sectors[self.selected_sector].z1 -= 4
                    self.prop_sector_z1 = self.sectors[self.selected_sector].z1
                else:
                    self.prop_sector_z1 -= 4
                self.status_message = "Floor Lowered (-4)"
            self.notification_message = self.status_message
            self.notification_timer = 1.0

        elif event.key == pygame.K_RIGHTBRACKET:
            # Increase height
            self.save_undo_snapshot()
            if pygame.key.get_mods() & pygame.KMOD_SHIFT:
                # Ceiling
                if self.selected_sector is not None:
                    self.sectors[self.selected_sector].z2 += 4
                    self.prop_sector_z2 = self.sectors[self.selected_sector].z2
                else:
                    self.prop_sector_z2 += 4
                self.status_message = "Ceiling Raised (+4)"
            else:
                # Floor
                if self.selected_sector is not None:
                    self.sectors[self.selected_sector].z1 += 4
                    self.prop_sector_z1 = self.sectors[self.selected_sector].z1
                else:
                    self.prop_sector_z1 += 4
                self.status_message = "Floor Raised (+4)"
            self.notification_message = self.status_message
            self.notification_timer = 1.0

    def handle_mouse_down(self, event):
        if event.button == 1:
            self.mouse_down = True
            self.last_mouse_pos = event.pos
            
            # Check UI clicks FIRST (Menu, Toolbar, Properties)
            # This prevents clicks from falling through to viewports
            
            # 1. Menu Bar & Dropdowns
            if self.check_ui_click(event.pos):
                return

            # 2. Property panel
            if self.handle_property_click(event.pos):
                return
            
            # 3. Toolbar
            if self.handle_toolbar_click(event.pos):
                return
                
            # 4. Viewports
            for vp in self.viewports:
                if vp.rect.collidepoint(event.pos):
                    self.active_viewport = vp
                    vp.is_active = True
                    if self.current_tool == Tool.SELECT:
                        if not self.handle_select_click(vp, event.pos):
                            # Nothing clicked, start box selection
                            if vp.view_type == ViewType.TOP:
                                self.selecting_box = True
                                self.box_start = event.pos
                                self.box_end = event.pos
                        else:
                            # Start dragging if we selected something
                            if self.selected_sector is not None or self.selected_wall is not None:
                                self.save_undo_snapshot() # Save before drag starts
                                self.dragging_geometry = True
                    elif self.current_tool == Tool.CREATE_SECTOR:
                        self.handle_create_sector_click(vp, event.pos)
                    elif self.current_tool == Tool.VERTEX_EDIT:
                        self.handle_vertex_edit_click(vp, event.pos)
                        # Start dragging if we have vertices selected
                        if self.selected_vertices:
                            self.save_undo_snapshot() # Save before drag starts
                            self.dragging_geometry = True
                    elif self.current_tool == Tool.ENTITY:
                        self.handle_entity_click(vp, event.pos)
                        if self.selected_enemy is not None:
                            self.save_undo_snapshot()
                            self.dragging_geometry = True
                    elif self.current_tool == Tool.LIGHT:
                        self.handle_light_click(vp, event.pos)
                        if self.selected_light is not None:
                            self.save_undo_snapshot()
                            self.dragging_geometry = True
                    elif self.current_tool == Tool.GATE:
                        self.handle_gate_click(vp, event.pos)
                        if self.selected_gate is not None or self.selected_switch is not None:
                            self.save_undo_snapshot()
                            self.dragging_geometry = True
                else:

                    vp.is_active = False
            
            # self.check_ui_click(event.pos) # MOVED TO TOP
        elif event.button == 3:
            self.last_mouse_pos = event.pos

    # ADDED: Geometry dragging methods
    def drag_sector(self, pos):
        vp = self.active_viewport
        if not vp or vp.view_type != ViewType.TOP:
            return
            
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        prev_wx, prev_wy = vp.screen_to_world(self.last_mouse_pos[0], self.last_mouse_pos[1])
        
        dx = wx - prev_wx
        dy = wy - prev_wy
        
        # ... (rest of drag logic same as before) ...
        
        sector = self.sectors[self.selected_sector]
        for i in range(sector.ws, sector.we):
            if i < len(self.walls):
                wall = self.walls[i]
                wall.x1 += dx
                wall.y1 += dy
                wall.x2 += dx
                wall.y2 += dy
                
                if self.snap_to_grid:
                    wall.x1 = round(wall.x1 / vp.grid_size) * vp.grid_size
                    wall.y1 = round(wall.y1 / vp.grid_size) * vp.grid_size
                    wall.x2 = round(wall.x2 / vp.grid_size) * vp.grid_size
                    wall.y2 = round(wall.y2 / vp.grid_size) * vp.grid_size

    def drag_vertices(self, pos):
        vp = self.active_viewport
        if not vp or vp.view_type != ViewType.TOP:
            return
            
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        prev_wx, prev_wy = vp.screen_to_world(self.last_mouse_pos[0], self.last_mouse_pos[1])
        
        dx = wx - prev_wx
        dy = wy - prev_wy
        
        for wall_idx, vert_idx in self.selected_vertices:
            if wall_idx < len(self.walls):
                wall = self.walls[wall_idx]
                if vert_idx == 0:
                    wall.x1 += dx
                    wall.y1 += dy
                    if self.snap_to_grid:
                        wall.x1 = round(wall.x1 / vp.grid_size) * vp.grid_size
                        wall.y1 = round(wall.y1 / vp.grid_size) * vp.grid_size
                else:
                    wall.x2 += dx
                    wall.y2 += dy
                    if self.snap_to_grid:
                        wall.x2 = round(wall.x2 / vp.grid_size) * vp.grid_size
                        wall.y2 = round(wall.y2 / vp.grid_size) * vp.grid_size

    def drag_entity(self, pos):
        vp = self.active_viewport
        if not vp or vp.view_type != ViewType.TOP:
            return
            
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        prev_wx, prev_wy = vp.screen_to_world(self.last_mouse_pos[0], self.last_mouse_pos[1])
        
        dx = wx - prev_wx
        dy = wy - prev_wy
        
        if self.selected_enemy is not None and self.selected_enemy < len(self.enemies):
            enemy = self.enemies[self.selected_enemy]
            enemy.x += dx
            enemy.y += dy
            
            if self.snap_to_grid:
                enemy.x = round(enemy.x / vp.grid_size) * vp.grid_size
                enemy.y = round(enemy.y / vp.grid_size) * vp.grid_size

        elif self.selected_pickup is not None and self.selected_pickup < len(self.pickups):
            pickup = self.pickups[self.selected_pickup]
            pickup.x += dx
            pickup.y += dy
            
            if self.snap_to_grid:
                pickup.x = round(pickup.x / vp.grid_size) * vp.grid_size
                pickup.y = round(pickup.y / vp.grid_size) * vp.grid_size

    def drag_light(self, pos):
        """Drag selected light to new position"""
        vp = self.active_viewport
        if not vp or vp.view_type == ViewType.CAMERA_3D:
            return
        
        if self.selected_light is None or self.selected_light >= len(self.lights):
            return
            
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        
        if self.snap_to_grid:
            wx, wy = vp.snap_to_grid(wx, wy)
        
        light = self.lights[self.selected_light]
        light.x = int(wx)
        light.y = int(wy)

    # ADDED: property click handler
    def handle_property_click(self, pos):
        panel_width = 280
        panel_x = WINDOW_WIDTH - panel_width
        panel_y = 30
        panel_rect = pygame.Rect(panel_x, panel_y, panel_width, WINDOW_HEIGHT - 30 - 25)
        if not panel_rect.collidepoint(pos):
            return False
        # Texture nav buttons
        # Texture nav buttons
        for rect, delta in self.texture_nav_buttons:
            if rect.collidepoint(pos):
                if self.selected_wall is not None:
                    self.save_undo_snapshot() # Save before texture change
                    wall = self.walls[self.selected_wall]
                    wall.wt = (wall.wt + delta) % max(1, len(self.textures))
                    self.prop_wall_texture = wall.wt
                    return True
                elif self.current_tool == Tool.CREATE_SECTOR:
                    self.prop_wall_texture = (self.prop_wall_texture + delta) % max(1, len(self.textures))
                    return True
                elif self.current_tool == Tool.ENTITY:
                     # If enemy selected, change its type
                    if self.selected_enemy is not None:
                        enemy = self.enemies[self.selected_enemy]
                        enemy.enemy_type = (enemy.enemy_type + delta) % max(1, len(self.enemy_textures))
                        self.prop_enemy_type = enemy.enemy_type
                    else:
                        # Change default type
                        self.prop_enemy_type = (self.prop_enemy_type + delta) % max(1, len(self.enemy_textures))
                    return True
        # UI Buttons (Mode switch etc)
        if hasattr(self, 'ui_buttons'):
            for btn in self.ui_buttons:
                if btn['rect'].collidepoint(pos):
                    action = btn.get('action')
                    if action:
                        action()
                    return True

        # Property items
        for item in self.property_items:
            if item['rect'].collidepoint(pos):
                # Handle glass toggle checkbox specially
                if item.get('toggle_glass'):
                    if self.prop_wall_material == MATERIAL_GLASS:
                        self.prop_wall_material = MATERIAL_SOLID
                    else:
                        self.prop_wall_material = MATERIAL_GLASS
                    return True
                
                # Handle toggle for existing wall glass property
                if item.get('toggle_wall_glass'):
                    wall = item['target']
                    if getattr(wall, 'material', 0) == MATERIAL_GLASS:
                        wall.material = MATERIAL_SOLID
                    else:
                        wall.material = MATERIAL_GLASS
                        # Initialize glass properties if needed
                        if not hasattr(wall, 'tint_r'): wall.tint_r = 220
                        if not hasattr(wall, 'tint_g'): wall.tint_g = 230
                        if not hasattr(wall, 'tint_b'): wall.tint_b = 240
                        if not hasattr(wall, 'opacity'): wall.opacity = 80
                    return True
                
                # Handle tint preset button click
                if item.get('tint_preset'):
                    preset = item['tint_preset']
                    self.prop_wall_tint_preset = preset
                    r, g, b = GLASS_TINT_PRESETS[preset]
                    self.prop_wall_tint_r = r
                    self.prop_wall_tint_g = g
                    self.prop_wall_tint_b = b
                    self.prop_wall_tint_b = b
                    return True
                


                self.edit_field = item
                current_val = getattr(item['target'], item['attr'])
                self.edit_buffer = str(current_val)
                # REMOVED: Dragging logic
                return True
        return True  # Click inside panel but not on item still consumes

    def handle_mouse_up(self, event):
        if event.button == 1:
            self.mouse_down = False
            self.dragging_geometry = False # Stop geometry dragging
            
            if self.selecting_box:
                self.selecting_box = False
                # Perform box selection
                vp = self.active_viewport
                if vp and vp.view_type == ViewType.TOP:
                    # Calculate box in world coords
                    start_x, start_y = vp.screen_to_world(self.box_start[0], self.box_start[1])
                    end_x, end_y = vp.screen_to_world(self.box_end[0], self.box_end[1])
                    
                    min_x = min(start_x, end_x)
                    max_x = max(start_x, end_x)
                    min_y = min(start_y, end_y)
                    max_y = max(start_y, end_y)
                    
                    # Find sectors inside box
                    # A sector is selected if ANY of its walls are inside (or maybe center?)
                    # Let's say if at least one wall point is inside
                    
                    found_sector = False
                    found_sector = False
                    for i, sector in enumerate(self.sectors):
                        is_inside = False
                        found_wall_idx = None
                        for w_idx in range(sector.ws, sector.we):
                            if w_idx < len(self.walls):
                                w = self.walls[w_idx]
                                # Check if either endpoint is in box
                                if (min_x <= w.x1 <= max_x and min_y <= w.y1 <= max_y) or \
                                   (min_x <= w.x2 <= max_x and min_y <= w.y2 <= max_y):
                                    is_inside = True
                                    found_wall_idx = w_idx
                                    break
                        if is_inside:
                            self.selected_sector = i
                            # Select the specific wall found
                            if found_wall_idx is not None:
                                self.selected_wall = found_wall_idx
                            elif sector.ws < sector.we:
                                self.selected_wall = sector.ws
                            found_sector = True
                            break # Just select one for now, or last one found
                    
                    if not found_sector:
                        self.selected_sector = None
                        self.selected_wall = None

    def handle_mouse_motion(self, event):
        # Handle box selection
        if self.selecting_box:
            self.box_end = event.pos
            return

        # Handle geometry dragging
        if self.mouse_down and getattr(self, 'dragging_geometry', False) and self.active_viewport:
             if self.current_tool == Tool.SELECT and self.selected_sector is not None:
                 self.drag_sector(event.pos)
             elif self.current_tool == Tool.VERTEX_EDIT and self.selected_vertices:
                 self.drag_vertices(event.pos)
             elif self.current_tool == Tool.ENTITY and (self.selected_enemy is not None or self.selected_pickup is not None):
                 self.drag_entity(event.pos)
             elif self.current_tool == Tool.LIGHT and self.selected_light is not None:
                 self.drag_light(event.pos)
             self.last_mouse_pos = event.pos
             return

        if event.buttons[1] or (event.buttons[2] and self.current_tool == Tool.SELECT):
            if self.active_viewport:
                dx = event.pos[0] - self.last_mouse_pos[0]
                dy = event.pos[1] - self.last_mouse_pos[1]
                self.active_viewport.offset_x -= dx / self.active_viewport.zoom
                self.active_viewport.offset_y += dy / self.active_viewport.zoom
        for vp in self.viewports:
            if vp.rect.collidepoint(event.pos):
                self.update_hover_state(vp, event.pos)
        self.last_mouse_pos = event.pos

    def handle_mouse_wheel(self, event):
        if self.active_viewport:
            zoom_factor = 1.1 if event.y > 0 else 0.9
            self.active_viewport.zoom *= zoom_factor
            self.active_viewport.zoom = max(0.1, min(20.0, self.active_viewport.zoom))

    def handle_select_click(self, vp, pos):
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        selected_something = False
        for i, w in enumerate(self.walls):
            if self.is_near_wall(wx, wy, w, vp.view_type):
                self.selected_wall = i
                for j, s in enumerate(self.sectors):
                    if s.ws <= i < s.we:
                        self.selected_sector = j
                        break
                selected_something = True
                break
        if not selected_something:
            self.selected_wall = None
            self.selected_sector = None
            return False
        return True

    def handle_create_sector_click(self, vp, pos):
        if vp.view_type != ViewType.TOP:
            return
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        if self.snap_to_grid:
            wx, wy = vp.snap_to_grid(wx, wy)
        wx, wy = int(wx), int(wy)
        if not self.creating_sector:
            self.creating_sector = True
            self.sector_vertices = [(wx, wy)]
        else:
            first_x, first_y = self.sector_vertices[0]
            dist = math.sqrt((wx - first_x)**2 + (wy - first_y)**2)
            if dist < 16 / vp.zoom and len(self.sector_vertices) >= 3:
                self.finish_sector_creation()
            else:
                if len(self.sector_vertices) >= 1:
                    last_x, last_y = self.sector_vertices[-1]
                    if len(self.sector_vertices) == 1:
                        self.sector_vertices.append((wx, wy))
                    else:
                        prev_x, prev_y = self.sector_vertices[-2]
                        cross = (last_x - prev_x) * (wy - last_y) - (last_y - prev_y) * (wx - last_x)
                        if cross >= 0:
                            self.sector_vertices.append((wx, wy))
                        else:
                            print("Vertices must be counter-clockwise!")

    def handle_vertex_edit_click(self, vp, pos):
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        min_dist = float('inf')
        nearest_wall_idx = None
        nearest_vertex = None
        for i, w in enumerate(self.walls):
            for vertex_idx, (vx, vy) in enumerate([(w.x1, w.y1), (w.x2, w.y2)]):
                if vp.view_type == ViewType.TOP:
                    dist = math.sqrt((wx - vx)**2 + (wy - vy)**2)
                    if dist < min_dist and dist < 16 / vp.zoom:
                        min_dist = dist
                        nearest_wall_idx = i
                        nearest_vertex = vertex_idx
        if nearest_wall_idx is not None:
            if (nearest_wall_idx, nearest_vertex) in self.selected_vertices:
                self.selected_vertices.remove((nearest_wall_idx, nearest_vertex))
            else:
                self.selected_vertices.append((nearest_wall_idx, nearest_vertex))

    def handle_entity_click(self, vp, pos):
        if vp.view_type != ViewType.TOP:
            return
            
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        
        # Check if clicked on existing entity (Enemy OR Pickup)
        clicked_enemy = None
        clicked_pickup = None
        min_dist = float('inf')
        
        # Check enemies
        for i, enemy in enumerate(self.enemies):
            dist = math.sqrt((wx - enemy.x)**2 + (wy - enemy.y)**2)
            if dist < 10 / vp.zoom: # Hitbox
                if dist < min_dist:
                    min_dist = dist
                    clicked_enemy = i
                    clicked_pickup = None
        
        # Check pickups
        for i, pickup in enumerate(self.pickups):
            dist = math.sqrt((wx - pickup.x)**2 + (wy - pickup.y)**2)
            if dist < 8 / vp.zoom: # Smaller hitbox
                if dist < min_dist:
                    min_dist = dist
                    clicked_pickup = i
                    clicked_enemy = None
        
        if clicked_enemy is not None:
            self.selected_enemy = clicked_enemy
            self.selected_pickup = None
            self.selected_sector = None
            self.selected_wall = None
            print(f"Selected Enemy {clicked_enemy}")
            return
            
        if clicked_pickup is not None:
            self.selected_pickup = clicked_pickup
            self.selected_enemy = None
            self.selected_sector = None
            self.selected_wall = None
            print(f"Selected Pickup {clicked_pickup}")
            return

        # If we clicked on nothing, check if we should deselect
        if self.selected_enemy is not None or self.selected_pickup is not None:
            self.selected_enemy = None
            self.selected_pickup = None
            print("Deselected entity")
            return
            
        # If no entity clicked AND nothing selected, create new one based on mode
        if self.snap_to_grid:
            wx, wy = vp.snap_to_grid(wx, wy)
            
        self.save_undo_snapshot()
        
        if self.entity_mode == "ENEMY":
            self.enemies.append(Enemy(int(wx), int(wy), 0, self.prop_enemy_type))
            self.selected_enemy = len(self.enemies) - 1
            self.selected_pickup = None
            print(f"Created enemy at {int(wx)}, {int(wy)}")
        else: # PICKUP
            self.pickups.append(Pickup(int(wx), int(wy), 0, self.prop_pickup_type))
            self.selected_pickup = len(self.pickups) - 1
            self.selected_enemy = None
            print(f"Created pickup at {int(wx)}, {int(wy)}")

    def handle_light_click(self, vp, pos):
        """Handle mouse click in LIGHT tool mode"""
        if vp.view_type == ViewType.CAMERA_3D:
            return
            
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        
        # Check if clicking on existing light
        clicked_light = None
        min_dist = float('inf')
        
        for i, light in enumerate(self.lights):
            if light.active:
                dist = math.sqrt((wx - light.x)**2 + (wy - light.y)**2)
                click_radius = max(15, light.radius * 0.1) / vp.zoom
                if dist < click_radius:
                    if dist < min_dist:
                        min_dist = dist
                        clicked_light = i
        
        if clicked_light is not None:
            self.selected_light = clicked_light
            self.selected_sector = None
            self.selected_wall = None
            self.selected_enemy = None
            self.selected_pickup = None
            # Load light properties into defaults for editing
            light = self.lights[clicked_light]
            self.prop_light_r = light.r
            self.prop_light_g = light.g
            self.prop_light_b = light.b
            self.prop_light_radius = light.radius
            self.prop_light_intensity = light.intensity
            self.prop_light_type = light.light_type
            self.prop_light_flicker = light.flicker
            print(f"Selected Light {clicked_light}")
            return
        
        # If clicking on nothing and have selection, deselect
        if self.selected_light is not None:
            self.selected_light = None
            print("Deselected light")
            return
        
        # Create new light
        if self.snap_to_grid:
            wx, wy = vp.snap_to_grid(wx, wy)
        
        self.save_undo_snapshot()
        
        new_light = Light(
            x=int(wx), y=int(wy), z=30,
            radius=self.prop_light_radius,
            intensity=self.prop_light_intensity,
            r=self.prop_light_r,
            g=self.prop_light_g,
            b=self.prop_light_b,
            light_type=self.prop_light_type,
            flicker=self.prop_light_flicker
        )
        self.lights.append(new_light)
        self.selected_light = len(self.lights) - 1
        print(f"Created light at {int(wx)}, {int(wy)} with color ({self.prop_light_r}, {self.prop_light_g}, {self.prop_light_b})")

    def handle_gate_click(self, vp, pos):
        """Handle mouse click in GATE tool mode"""
        if vp.view_type == ViewType.CAMERA_3D:
            return
            
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        
        # Check if clicking on existing gate or switch
        clicked_gate = None
        clicked_switch = None
        min_dist = float('inf')
        
        # Check gates first
        for i, gate in enumerate(self.gates):
            if gate.active:
                dist = math.sqrt((wx - gate.x)**2 + (wy - gate.y)**2)
                click_radius = max(20, gate.width * 0.3) / vp.zoom
                if dist < click_radius:
                    if dist < min_dist:
                        min_dist = dist
                        clicked_gate = i
                        clicked_switch = None
        
        # Check switches
        for i, sw in enumerate(self.switches):
            if sw.active:
                dist = math.sqrt((wx - sw.x)**2 + (wy - sw.y)**2)
                click_radius = 15 / vp.zoom
                if dist < click_radius:
                    if dist < min_dist:
                        min_dist = dist
                        clicked_switch = i
                        clicked_gate = None
        
        # Select gate if clicked
        if clicked_gate is not None:
            self.selected_gate = clicked_gate
            self.selected_switch = None
            self.selected_sector = None
            self.selected_wall = None
            self.selected_enemy = None
            self.selected_pickup = None
            self.selected_light = None
            # Load gate properties
            gate = self.gates[clicked_gate]
            self.prop_gate_id = gate.gate_id
            self.prop_gate_z_closed = gate.z_closed
            self.prop_gate_z_open = gate.z_open
            self.prop_gate_speed = gate.speed
            self.prop_gate_trigger_radius = gate.trigger_radius
            self.prop_gate_texture = gate.texture
            self.prop_gate_width = gate.width
            print(f"Selected Gate {clicked_gate} (ID={gate.gate_id})")
            return
        
        # Select switch if clicked
        if clicked_switch is not None:
            self.selected_switch = clicked_switch
            self.selected_gate = None
            self.selected_sector = None
            self.selected_wall = None
            self.selected_enemy = None
            self.selected_pickup = None
            self.selected_light = None
            # Load switch properties
            sw = self.switches[clicked_switch]
            self.prop_switch_linked_gate_id = sw.linked_gate_id
            self.prop_switch_texture = sw.texture
            print(f"Selected Switch {clicked_switch} (linked to Gate {sw.linked_gate_id})")
            return
        
        # If clicking on nothing and have selection, deselect
        if self.selected_gate is not None or self.selected_switch is not None:
            self.selected_gate = None
            self.selected_switch = None
            print("Deselected gate/switch")
            return
        
        # Create new gate or switch based on mode
        if self.snap_to_grid:
            wx, wy = vp.snap_to_grid(wx, wy)
        
        self.save_undo_snapshot()
        
        if self.gate_mode == "GATE":
            # Auto-assign gate ID
            max_id = -1
            for g in self.gates:
                if g.gate_id > max_id:
                    max_id = g.gate_id
            new_id = max_id + 1
            
            new_gate = Gate(
                x=int(wx), y=int(wy),
                z_closed=self.prop_gate_z_closed,
                z_open=self.prop_gate_z_open,
                gate_id=new_id,
                trigger_radius=self.prop_gate_trigger_radius,
                texture=self.prop_gate_texture,
                speed=self.prop_gate_speed,
                width=self.prop_gate_width
            )
            self.gates.append(new_gate)
            self.selected_gate = len(self.gates) - 1
            self.selected_switch = None
            print(f"Created Gate {new_id} at {int(wx)}, {int(wy)}")
        else:
            # Create switch
            new_switch = Switch(
                x=int(wx), y=int(wy), z=20,
                linked_gate_id=self.prop_switch_linked_gate_id,
                texture=self.prop_switch_texture
            )
            self.switches.append(new_switch)
            self.selected_switch = len(self.switches) - 1
            self.selected_gate = None
            print(f"Created Switch at {int(wx)}, {int(wy)} linked to Gate {self.prop_switch_linked_gate_id}")


    def finish_sector_creation(self):

        if len(self.sector_vertices) < 3:
            print("Need at least 3 vertices to create a sector")
            self.creating_sector = False
            self.sector_vertices = []
            return
            
        self.save_undo_snapshot() # Save before creating sector
        
        wall_start = len(self.walls)
        for i in range(len(self.sector_vertices)):
            x1, y1 = self.sector_vertices[i]
            x2, y2 = self.sector_vertices[(i + 1) % len(self.sector_vertices)]
            dx = x2 - x1; dy = y2 - y1
            angle = math.atan2(dy, dx) * 180 / math.pi
            if angle < 0: angle += 360
            shade = int(angle % 180)
            if shade > 90: shade = 90 - (shade - 90)
            wall = Wall(x1, y1, x2, y2, self.prop_wall_texture,
                        self.prop_wall_u, self.prop_wall_v, shade,
                        self.prop_wall_material, self.prop_wall_tint_r,
                        self.prop_wall_tint_g, self.prop_wall_tint_b,
                        self.prop_wall_opacity)
            self.walls.append(wall)
        wall_end = len(self.walls)
        sector = Sector(wall_start, wall_end, self.prop_sector_z1, self.prop_sector_z2,
                        self.prop_sector_texture, self.prop_sector_scale, self.prop_sector_tag)
        self.sectors.append(sector)
        print(f"Created sector with {len(self.sector_vertices)} walls")
        self.creating_sector = False
        self.sector_vertices = []
        self.selected_sector = len(self.sectors) - 1

    def cancel_sector_creation(self):
        self.creating_sector = False
        self.sector_vertices = []

    def is_near_wall(self, wx, wy, wall, view_type):
        if view_type == ViewType.TOP:
            x1, y1, x2, y2 = wall.x1, wall.y1, wall.x2, wall.y2
            line_len_sq = (x2 - x1)**2 + (y2 - y1)**2
            if line_len_sq == 0:
                return math.sqrt((wx - x1)**2 + (wy - y1)**2) < 5
            t = max(0, min(1, ((wx - x1) * (x2 - x1) + (wy - y1) * (y2 - y1)) / line_len_sq))
            proj_x = x1 + t * (x2 - x1)
            proj_y = y1 + t * (y2 - y1)
            dist = math.sqrt((wx - proj_x)**2 + (wy - proj_y)**2)
            return dist < 5
        return False

    def update_hover_state(self, vp, pos):
        wx, wy = vp.screen_to_world(pos[0], pos[1])
        self.hover_vertex = None
        for i, w in enumerate(self.walls):
            for vertex_idx, (vx, vy) in enumerate([(w.x1, w.y1), (w.x2, w.y2)]):
                if vp.view_type == ViewType.TOP:
                    dist = math.sqrt((wx - vx)**2 + (wy - vy)**2)
                    if dist < 8 / vp.zoom:
                        self.hover_vertex = (i, vertex_idx)
                        return
        self.hover_wall = None
        for i, w in enumerate(self.walls):
            if self.is_near_wall(wx, wy, w, vp.view_type):
                self.hover_wall = i
                return

    def check_ui_click(self, pos):
        # Check menu if active OR if clicking in menu bar area
        if self.active_menu or pos[1] < 30:
            menu_clicked = self.check_menu_click(pos)
            if menu_clicked:
                return True
        
        # If we clicked outside an active menu, close it but don't consume click immediately?
        # Actually, standard behavior is to close menu and consume click if it was outside.
        # But if we want to allow clicking toolbar while menu is open, we should be careful.
        # For now, let's just ensure we don't fall through to viewports if menu was open.
        if self.active_menu:
            self.active_menu = None
            return True

        toolbar_x = 10
        button_y = 40
        button_height = 50
        button_width = 50
        for i, tool in enumerate(Tool):
            button_rect = pygame.Rect(toolbar_x, button_y + i * (button_height + 5),
                                      button_width, button_height)
            if button_rect.collidepoint(pos):
                self.current_tool = tool
                return True
        return False

    def show_about_dialog(self):
        """Show a modal About dialog"""
        dialog_width = 360
        dialog_height = 340
        dialog_x = (self.width - dialog_width) // 2
        dialog_y = (self.height - dialog_height) // 2
        
        # Create a transparent overlay
        overlay = pygame.Surface((self.width, self.height), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 180)) # Dark semi-transparent background
        
        # Prepare logo for this dialog size
        dialog_logo = self.logo_img
        if dialog_logo and dialog_logo.get_width() > dialog_width - 60:
            scale = (dialog_width - 60) / dialog_logo.get_width()
            new_h = int(dialog_logo.get_height() * scale)
            dialog_logo = pygame.transform.smoothscale(dialog_logo, (dialog_width - 60, new_h))

        running_dialog = True
        while running_dialog:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running_dialog = False
                    self.running = False
                elif event.type == pygame.KEYDOWN or event.type == pygame.MOUSEBUTTONDOWN:
                    running_dialog = False
            
            self.screen.blit(overlay, (0, 0))
            
            # Draw Dialog Box
            dialog_rect = pygame.Rect(dialog_x, dialog_y, dialog_width, dialog_height)
            
            # Shadow
            shadow_rect = dialog_rect.copy()
            shadow_rect.move_ip(4, 4)
            pygame.draw.rect(self.screen, (0, 0, 0), shadow_rect, border_radius=12)
            
            # Background
            pygame.draw.rect(self.screen, Colors.BG_DARK, dialog_rect, border_radius=12)
            pygame.draw.rect(self.screen, Colors.ACCENT, dialog_rect, 2, border_radius=12)
            
            # Content
            content_y = dialog_y + 40
            
            # Logo & Text Layout
            # We want [Logo] [Text] side by side
            # Logo should be small, e.g., 64px height
            
            target_h = 64
            
            # Prepare scaled surfaces
            surf_logo = None
            if self.logo_img:
                scale = target_h / self.logo_img.get_height()
                new_w = int(self.logo_img.get_width() * scale)
                surf_logo = pygame.transform.smoothscale(self.logo_img, (new_w, target_h))
            
            surf_text = None
            if self.logo_text_img:
                # Scale text to be smaller than logo, e.g. 40px height
                text_target_h = 40
                scale = text_target_h / self.logo_text_img.get_height()
                new_w = int(self.logo_text_img.get_width() * scale)
                
                # Check if it fits in remaining width (dialog width - logo width - padding)
                max_w = dialog_width - (new_w if surf_logo is None else surf_logo.get_width()) - 60
                if new_w > max_w:
                    scale = max_w / self.logo_text_img.get_width()
                    new_w = max_w
                    text_target_h = int(self.logo_text_img.get_height() * scale)
                
                surf_text = pygame.transform.smoothscale(self.logo_text_img, (new_w, text_target_h))
            
            # Calculate total width to center them
            total_w = 0
            gap = 15
            if surf_logo: total_w += surf_logo.get_width()
            if surf_text: total_w += surf_text.get_width()
            if surf_logo and surf_text: total_w += gap
            
            start_x = dialog_x + (dialog_width - total_w) // 2
            
            # Draw them
            curr_x = start_x
            if surf_logo:
                self.screen.blit(surf_logo, (curr_x, content_y))
                curr_x += surf_logo.get_width() + gap
            
            if surf_text:
                self.screen.blit(surf_text, (curr_x, content_y))
            elif not surf_text:
                # Fallback if text image missing
                title_surf = self.font_title.render("Oracular Editor", True, Colors.TEXT)
                self.screen.blit(title_surf, (curr_x, content_y + (target_h - title_surf.get_height())//2))

            content_y += target_h + 30
            
            # Version
            ver_surf = self.font_small.render("v1.0.0", True, Colors.ACCENT_BRIGHT)
            ver_rect = ver_surf.get_rect(center=(dialog_x + dialog_width // 2, content_y))
            self.screen.blit(ver_surf, ver_rect)
            content_y += 30
            
            # Description (Concise)
            desc_surf = self.font_medium.render("Oracular Editor", True, Colors.TEXT_DIM)
            desc_rect = desc_surf.get_rect(center=(dialog_x + dialog_width // 2, content_y))
            self.screen.blit(desc_surf, desc_rect)
            content_y += 35
            
            # Credits
            cred_surf = self.font_small.render("By Uttkarsh", True, Colors.TEXT)
            cred_rect = cred_surf.get_rect(center=(dialog_x + dialog_width // 2, content_y))
            self.screen.blit(cred_surf, cred_rect)
            
            # Footer
            footer_surf = self.font_small.render("Press any key to close", True, (100, 100, 100))
            footer_rect = footer_surf.get_rect(center=(dialog_x + dialog_width // 2, dialog_y + dialog_height - 20))
            self.screen.blit(footer_surf, footer_rect)
            
            pygame.display.flip()
            self.clock.tick(60)
        
        # If we clicked outside an active menu, close it but don't consume click immediately?
        # Actually, standard behavior is to close menu and consume click if it was outside.
        # But if we want to allow clicking toolbar while menu is open, we should be careful.
        # For now, let's just ensure we don't fall through to viewports if menu was open.
        if self.active_menu:
            self.active_menu = None
            return True

        toolbar_x = 10
        button_y = 40
        button_height = 50
        button_width = 50
        for i, tool in enumerate(Tool):
            button_rect = pygame.Rect(toolbar_x, button_y + i * (button_height + 5),
                                      button_width, button_height)
            if button_rect.collidepoint(pos):
                self.current_tool = tool
                return True
        return False
        return False

    def check_menu_click(self, pos):
        menus = ["File", "Edit", "View", "Tools", "Help"]
        
        # Check if clicking inside an open dropdown
        if self.active_menu:
            dropdown_rect, item = self.get_dropdown_rect_and_item(pos)
            if dropdown_rect and dropdown_rect.collidepoint(pos):
                if item:
                    # Handle menu action
                    if "---" not in item:
                        self.execute_menu_action(self.active_menu, item)
                    self.active_menu = None
                return True
                
        x = 10
        menu_height = 30
        for menu in menus:
            text = self.font_medium.render(menu, True, Colors.TEXT)
            menu_width = text.get_width() + 20
            
            # Check if clicked on this menu header
            if x <= pos[0] <= x + menu_width and pos[1] < menu_height:
                if menu == self.active_menu:
                    self.active_menu = None # Toggle off
                else:
                    self.active_menu = menu # Switch to this menu
                return True
            x += menu_width
            
        return False

    def get_dropdown_rect_and_item(self, pos):
        if not self.active_menu:
            return None, None
        menus = ["File", "Edit", "View", "Tools", "Help"]
        x = 10
        for menu in menus:
            text = self.font_medium.render(menu, True, Colors.TEXT)
            menu_width = text.get_width() + 20
            if menu == self.active_menu:
                items = self.menu_items[menu]
                dropdown_width = 200
                dropdown_height = len(items) * 25 + 10
                dropdown_x = x
                dropdown_y = 30
                dropdown_rect = pygame.Rect(dropdown_x, dropdown_y, dropdown_width, dropdown_height)
                if dropdown_rect.collidepoint(pos):
                    item_y = dropdown_y + 5
                    for item in items:
                        if item != "---":
                            item_rect = pygame.Rect(dropdown_x, item_y, dropdown_width, 25)
                            if item_rect.collidepoint(pos):
                                return dropdown_rect, item
                        item_y += 25
                return dropdown_rect, None
            x += menu_width
        return None, None

    def execute_menu_action(self, menu, item):
        if menu == "File":
            if "New" in item:
                self.new_level()
            elif "Open" in item:
                self.open_level_dialog()
            elif "Save" in item:
                self.save_level()
            elif "Exit" in item:
                self.running = False
        elif menu == "Edit":
            if "Undo" in item:
                self.undo()
            elif "Redo" in item:
                self.redo()
            elif "Delete" in item:
                self.save_undo_snapshot() # Save before delete
                self.delete_selected()
            elif "Select All" in item:
                self.selected_vertices = []
                for i, wall in enumerate(self.walls):
                    self.selected_vertices.append((i, 0))
                    self.selected_vertices.append((i, 1))
        elif menu == "View":
            if "Grid" in item:
                self.show_grid = not self.show_grid
            elif "Snap" in item:
                self.snap_to_grid = not self.snap_to_grid
        elif menu == "Tools":
            if "Select" in item:
                self.current_tool = Tool.SELECT
            elif "Create Sector" in item:
                self.current_tool = Tool.CREATE_SECTOR
                self.selected_sector = None
                self.selected_wall = None
                self.selected_vertices = []
            elif "Vertex Edit" in item:
                self.current_tool = Tool.VERTEX_EDIT
            elif "Entity" in item:
                self.current_tool = Tool.ENTITY
                self.selected_enemy = None
        elif menu == "Help":
            if "About" in item:
                self.show_about_dialog()

    def open_level_dialog(self):
        """Open file dialog to select a level file"""
        import subprocess
        
        # Get the project root directory (parent of tools/)
        project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        initial_dir = project_dir if os.path.exists(project_dir) else os.getcwd()
        
        filename = None
        
        # Try zenity first (native Linux dialog)
        try:
            result = subprocess.run([
                'zenity', '--file-selection',
                '--title=Open Level File',
                f'--filename={initial_dir}/',
                '--file-filter=Level files (*.h) | *.h',
                '--file-filter=All files | *'
            ], capture_output=True, text=True, timeout=120)
            
            if result.returncode == 0 and result.stdout.strip():
                filename = result.stdout.strip()
        except (FileNotFoundError, subprocess.TimeoutExpired):
            # Fallback to tkinter if zenity not available
            try:
                root = Tk()
                root.withdraw()
                root.attributes('-topmost', True)
                filename = filedialog.askopenfilename(
                    title="Open Level File",
                    initialdir=initial_dir,
                    filetypes=[("Level files", "*.h"), ("All files", "*.*")]
                )
                root.destroy()
            except Exception as e:
                print(f"File dialog error: {e}")
        
        # Load the selected file
        if filename:
            self.current_level_path = filename
            self.load_level()
            print(f"Opened: {filename}")
    
    def delete_selected(self):
        """Delete selected sector/wall"""
        if self.selected_sector is not None:
            sector = self.sectors[self.selected_sector]
            
            # Remove walls
            num_walls = sector.we - sector.ws
            for _ in range(num_walls):
                self.walls.pop(sector.ws)
            
            # Update other sectors' wall indices
            for s in self.sectors[self.selected_sector + 1:]:
                s.ws -= num_walls
                s.we -= num_walls
            
            # Remove sector
            self.sectors.pop(self.selected_sector)
            
            self.selected_sector = None
            self.selected_wall = None
            print("Deleted sector")
            
        elif self.selected_enemy is not None:
            if 0 <= self.selected_enemy < len(self.enemies):
                self.enemies.pop(self.selected_enemy)
                self.selected_enemy = None
                print("Deleted enemy")
        
        elif self.selected_pickup is not None:
            if 0 <= self.selected_pickup < len(self.pickups):
                self.pickups.pop(self.selected_pickup)
                self.selected_pickup = None
                print("Deleted pickup")
        
        elif self.selected_light is not None:
            if 0 <= self.selected_light < len(self.lights):
                self.lights.pop(self.selected_light)
                self.selected_light = None
                print("Deleted light")
        
        elif self.selected_gate is not None:
            if 0 <= self.selected_gate < len(self.gates):
                self.gates.pop(self.selected_gate)
                self.selected_gate = None
                print("Deleted gate")
        
        elif self.selected_switch is not None:
            if 0 <= self.selected_switch < len(self.switches):
                self.switches.pop(self.selected_switch)
                self.selected_switch = None
                print("Deleted switch")
    
    def new_level(self):
        """Create new level"""
        self.sectors = []
        self.walls = []
        self.player = Player(x=32*9, y=48, z=30, a=0, l=0)
        self.selected_sector = None
        self.selected_wall = None
        self.selected_vertices = []
        self.enemies = []
        self.selected_enemy = None
        self.pickups = []
        self.selected_pickup = None
        self.lights = []
        self.selected_light = None
        self.gates = []
        self.selected_gate = None
        self.switches = []
        self.selected_switch = None
        self.entity_mode = "ENEMY"
        self.gate_mode = "GATE"
        self.creating_sector = False
        self.sector_vertices = []
        self.current_level_path = DEFAULT_LEVEL_PATH  # Reset to default path
        
        # Center viewports on player position for new level
        self.center_viewports_on_level()
        
        print("New level created")
    
    def save_level(self):
        """Save level to current file path"""
        try:
            # Ensure directory exists
            os.makedirs(os.path.dirname(self.current_level_path), exist_ok=True)
            
            with open(self.current_level_path, "w") as f:
                # Write sectors
                f.write(f"{len(self.sectors)}\n")
                for s in self.sectors:
                    f.write(f"{s.ws} {s.we} {s.z1} {s.z2} {s.st} {s.ss} {s.tag}\n")
                
                # Write walls (including material properties for glass)
                f.write(f"{len(self.walls)}\n")
                for w in self.walls:
                    material = getattr(w, 'material', 0)
                    tint_r = getattr(w, 'tint_r', 220)
                    tint_g = getattr(w, 'tint_g', 230)
                    tint_b = getattr(w, 'tint_b', 240)
                    opacity = getattr(w, 'opacity', 80)
                    f.write(f"{w.x1} {w.y1} {w.x2} {w.y2} {w.wt} {w.u} {w.v} {w.shade} {material} {tint_r} {tint_g} {tint_b} {opacity}\n")
                
                # Write player
                f.write(f"\n{self.player.x} {self.player.y} {self.player.z} {self.player.a} {self.player.l}\n")
                
                # Write enemies
                f.write(f"\n{len(self.enemies)}\n")
                for e in self.enemies:
                    f.write(f"{e.x} {e.y} {e.z} {e.enemy_type}\n")

                # Write pickups
                f.write(f"\n{len(self.pickups)}\n")
                for p in self.pickups:
                    f.write(f"{p.x} {p.y} {p.z} {p.pickup_type} {p.respawns}\n")
                
                # Write lights
                f.write(f"\n{len(self.lights)}\n")
                for light in self.lights:
                    if light.active:
                        # Format: x y z radius intensity r g b type spotAngle dirX dirY dirZ flicker flickerSpeed
                        f.write(f"{light.x} {light.y} {light.z} {light.radius} {light.intensity} ")
                        f.write(f"{light.r} {light.g} {light.b} ")
                        f.write(f"{light.light_type} 0 0 0 0 {light.flicker} 100\n")
                
                # Write gates
                f.write(f"\n{len(self.gates)}\n")
                for gate in self.gates:
                    if gate.active:
                        # Format: x y z_closed z_open gate_id trigger_radius texture speed width
                        f.write(f"{gate.x} {gate.y} {gate.z_closed} {gate.z_open} ")
                        f.write(f"{gate.gate_id} {gate.trigger_radius} {gate.texture} ")
                        f.write(f"{gate.speed} {gate.width}\n")
                
                # Write switches
                f.write(f"\n{len(self.switches)}\n")
                for sw in self.switches:
                    if sw.active:
                        # Format: x y z linked_gate_id texture
                        f.write(f"{sw.x} {sw.y} {sw.z} {sw.linked_gate_id} {sw.texture}\n")
            
            print(f"Level saved to {self.current_level_path}")
            self.notification_message = "Level Saved!"
            self.notification_timer = self.notification_duration
        except Exception as e:
            print(f"Error saving level: {e}")
            self.notification_message = "Error Saving!"
            self.notification_timer = self.notification_duration
    
    def load_level(self):
        """Load level from current file path"""
        try:
            if not os.path.exists(self.current_level_path):
                print(f"File not found: {self.current_level_path}")
                return
            
            with open(self.current_level_path, "r") as f:
                lines = f.readlines()
            
            # Parse sectors
            num_sectors = int(lines[0].strip())
            self.sectors = []
            for i in range(1, num_sectors + 1):
                parts = list(map(int, lines[i].split()))
                if len(parts) < 7: parts.append(0) # Default tag 0 if missing
                self.sectors.append(Sector(*parts))
            
            # Parse walls
            wall_line = num_sectors + 1
            num_walls = int(lines[wall_line].strip())
            self.walls = []
            for i in range(wall_line + 1, wall_line + 1 + num_walls):
                parts = list(map(int, lines[i].split()))
                self.walls.append(Wall(*parts))
            
            # Parse player
            player_line = wall_line + num_walls + 2
            if player_line < len(lines):
                parts = list(map(int, lines[player_line].split()))
                self.player = Player(*parts)
            
            # Parse enemies (if available)
            self.enemies = []
            enemy_line = player_line + 2
            if enemy_line < len(lines):
                try:
                    num_enemies = int(lines[enemy_line].strip())
                    for i in range(enemy_line + 1, enemy_line + 1 + num_enemies):
                        if i < len(lines):
                            parts = list(map(int, lines[i].split()))
                            if len(parts) >= 4:
                                self.enemies.append(Enemy(parts[0], parts[1], parts[2], parts[3]))
                            else:
                                # Backward compatibility or malformed line
                                self.enemies.append(Enemy(parts[0], parts[1], parts[2], 0))
                except ValueError:
                    print("No enemies found or invalid format")
            
            # Parse pickups
            self.pickups = []
            if 'num_enemies' in locals():
                pickup_line = enemy_line + num_enemies + 2
            else:
                pickup_line = enemy_line + 2 # Fallback if no enemies found but maybe line existed?

            if pickup_line < len(lines):
                 try:
                    num_pickups = int(lines[pickup_line].strip())
                    for i in range(pickup_line + 1, pickup_line + 1 + num_pickups):
                        if i < len(lines):
                            parts = list(map(int, lines[i].split()))
                            if len(parts) >= 5:
                                self.pickups.append(Pickup(parts[0], parts[1], parts[2], parts[3], parts[4]))
                            elif len(parts) >= 4:
                                # Legacy support for 4-column format
                                self.pickups.append(Pickup(parts[0], parts[1], parts[2], parts[3], 1))
                 except ValueError:
                     print("No pickups found")
            
            # Parse lights
            self.lights = []
            if 'num_pickups' in locals():
                light_line = pickup_line + num_pickups + 2
            else:
                light_line = pickup_line + 2
            
            if light_line < len(lines):
                try:
                    num_lights = int(lines[light_line].strip())
                    for i in range(light_line + 1, light_line + 1 + num_lights):
                        if i < len(lines):
                            parts = list(map(int, lines[i].split()))
                            if len(parts) >= 15:
                                # Full format: x y z radius intensity r g b type spotAngle dirX dirY dirZ flicker flickerSpeed
                                light = Light(
                                    x=parts[0], y=parts[1], z=parts[2],
                                    radius=parts[3], intensity=parts[4],
                                    r=parts[5], g=parts[6], b=parts[7],
                                    light_type=parts[8], flicker=parts[13]
                                )
                                self.lights.append(light)
                            elif len(parts) >= 8:
                                # Minimal format: x y z radius intensity r g b
                                light = Light(
                                    x=parts[0], y=parts[1], z=parts[2],
                                    radius=parts[3], intensity=parts[4],
                                    r=parts[5], g=parts[6], b=parts[7]
                                )
                                self.lights.append(light)
                    print(f"Loaded {num_lights} lights")
                except ValueError:
                    print("No lights found or invalid format")
            
            # Parse gates
            self.gates = []
            if 'num_lights' in locals():
                gate_line = light_line + num_lights + 2
            else:
                gate_line = light_line + 2
            
            if gate_line < len(lines):
                try:
                    num_gates = int(lines[gate_line].strip())
                    for i in range(gate_line + 1, gate_line + 1 + num_gates):
                        if i < len(lines):
                            parts = list(map(int, lines[i].split()))
                            if len(parts) >= 9:
                                # Full format: x y z_closed z_open gate_id trigger_radius texture speed width
                                gate = Gate(
                                    x=parts[0], y=parts[1],
                                    z_closed=parts[2], z_open=parts[3],
                                    gate_id=parts[4], trigger_radius=parts[5],
                                    texture=parts[6], speed=parts[7], width=parts[8]
                                )
                                self.gates.append(gate)
                    print(f"Loaded {num_gates} gates")
                except ValueError:
                    print("No gates found or invalid format")
            
            # Parse switches
            self.switches = []
            if 'num_gates' in locals():
                switch_line = gate_line + num_gates + 2
            else:
                switch_line = gate_line + 2
            
            if switch_line < len(lines):
                try:
                    num_switches = int(lines[switch_line].strip())
                    for i in range(switch_line + 1, switch_line + 1 + num_switches):
                        if i < len(lines):
                            parts = list(map(int, lines[i].split()))
                            if len(parts) >= 5:
                                # Format: x y z linked_gate_id texture
                                sw = Switch(
                                    x=parts[0], y=parts[1], z=parts[2],
                                    linked_gate_id=parts[3], texture=parts[4]
                                )
                                self.switches.append(sw)
                    print(f"Loaded {num_switches} switches")
                except ValueError:
                    print("No switches found or invalid format")
            
            # Center viewports on the loaded geometry
            self.center_viewports_on_level()
            
            print(f"Loaded {len(self.sectors)} sectors, {len(self.walls)} walls, {len(self.lights)} lights, {len(self.gates)} gates, {len(self.switches)} switches from {self.current_level_path}")
        except Exception as e:
            print(f"Error loading level: {e}")
    
    def update(self):
        """Update editor state"""
        # Update properties if sector is selected
        if self.selected_sector is not None:
            sector = self.sectors[self.selected_sector]
            self.prop_sector_z1 = sector.z1
            self.prop_sector_z2 = sector.z2
            self.prop_sector_texture = sector.st
            self.prop_sector_scale = sector.ss
            self.prop_sector_tag = sector.tag
            
            if self.selected_wall is not None:
                wall = self.walls[self.selected_wall]
                self.prop_wall_texture = wall.wt
                self.prop_wall_u = wall.u
                self.prop_wall_v = wall.v
        
        if self.selected_enemy is not None and self.selected_enemy < len(self.enemies):
            enemy = self.enemies[self.selected_enemy]
            self.prop_enemy_type = enemy.enemy_type
            
        if self.selected_pickup is not None and self.selected_pickup < len(self.pickups):
            pickup = self.pickups[self.selected_pickup]
            self.prop_pickup_type = pickup.pickup_type
            
        # Update notification timer
        if self.notification_timer > 0:
            self.notification_timer -= 1.0 / FPS

    
    def render(self):
        """Render the editor"""
        self.screen.fill(Colors.BG_DARK)
        
        # Draw menu bar (without dropdown)
        self.draw_menu_bar_base()
        
        # Draw toolbar
        self.draw_toolbar()
        
        # Draw viewports
        for vp in self.viewports:
            self.draw_viewport(vp)
        
        # Draw properties panel
        self.draw_properties_panel()
        
        # Draw status bar
        self.draw_status_bar()
        
        # Draw dropdown menu LAST so it appears on top of everything
        if self.active_menu:
            self.draw_dropdown_menu(self.active_menu)
            
        # Draw notification
        if self.notification_timer > 0:
            self.draw_notification()
        
        pygame.display.flip()
    
    def draw_notification(self):
        """Draw fading notification message"""
        if not self.notification_message:
            return
            
        alpha = min(255, int(255 * (self.notification_timer / 1.0))) # Fade out in last 1 second
        if self.notification_timer > 1.0:
            alpha = 255
            
        text_surf = self.font_title.render(self.notification_message, True, Colors.ACCENT_BRIGHT)
        text_surf.set_alpha(alpha)
        
        padding = 10
        x = WINDOW_WIDTH - text_surf.get_width() - padding
        y = WINDOW_HEIGHT - self.STATUS_BAR_HEIGHT - text_surf.get_height() - padding
        
        # Background for better visibility
        bg_surf = pygame.Surface((text_surf.get_width() + 20, text_surf.get_height() + 10))
        bg_surf.fill(Colors.BG_DARK)
        bg_surf.set_alpha(int(alpha * 0.8))
        
        self.screen.blit(bg_surf, (x - 10, y - 5))
        self.screen.blit(text_surf, (x, y))
    
    def draw_menu_bar_base(self):
        """Draw top menu bar (without dropdown)"""
        menu_height = 30
        pygame.draw.rect(self.screen, Colors.PANEL_BG, (0, 0, WINDOW_WIDTH, menu_height))
        
        # Menu items
        menus = ["File", "Edit", "View", "Tools", "Help"]
        x = 10
        mouse_pos = pygame.mouse.get_pos()
        
        for menu in menus:
            text = self.font_medium.render(menu, True, Colors.TEXT)
            menu_width = text.get_width() + 20
            menu_rect = pygame.Rect(x, 0, menu_width, menu_height)
            
            # Highlight active or hovered menu
            if menu == self.active_menu or (menu_rect.collidepoint(mouse_pos) and mouse_pos[1] < menu_height):
                pygame.draw.rect(self.screen, Colors.BG_LIGHT, menu_rect)
            
            self.screen.blit(text, (x + 10, 8))
            x += menu_width
    
    def draw_dropdown_menu(self, menu_name):
        """Draw dropdown menu - called LAST to ensure it's on top"""
        menus = ["File", "Edit", "View", "Tools", "Help"]
        x = 10
        
        # Find menu position
        for menu in menus:
            text = self.font_medium.render(menu, True, Colors.TEXT)
            menu_width = text.get_width() + 20;
            
            if menu == menu_name:
                items = self.menu_items[menu_name]
                dropdown_width = 200
                dropdown_height = len(items) * 25 + 10
                dropdown_x = x
                dropdown_y = 30
                dropdown_rect = pygame.Rect(dropdown_x, dropdown_y, dropdown_width, dropdown_height)
                
                # Draw dropdown background with shadow for depth
                shadow_offset = 3
                shadow_rect = pygame.Rect(dropdown_x + shadow_offset, dropdown_y + shadow_offset, dropdown_width, dropdown_height)
                pygame.draw.rect(self.screen, (0, 0, 0), shadow_rect)
                
                dropdown_rect = pygame.Rect(dropdown_x, dropdown_y, dropdown_width, dropdown_height)
                pygame.draw.rect(self.screen, Colors.PANEL_BG, dropdown_rect)
                pygame.draw.rect(self.screen, Colors.SEPARATOR, dropdown_rect, 2)
                
                # Draw menu items
                item_y = dropdown_y + 5
                mouse_pos = pygame.mouse.get_pos()
                
                for item in items:
                    if item == "---":
                        # Draw separator
                        pygame.draw.line(self.screen, Colors.SEPARATOR, 
                                       (dropdown_x + 5, item_y + 10), 
                                       (dropdown_x + dropdown_width - 5, item_y + 10), 1)
                    else:
                        item_rect = pygame.Rect(dropdown_x, item_y, dropdown_width, 25)
                        
                        # Highlight hovered item
                        if item_rect.collidepoint(mouse_pos):
                            pygame.draw.rect(self.screen, Colors.BG_LIGHT, item_rect)
                        
                        # Draw item text
                        item_text = self.font_small.render(item, True, Colors.TEXT)
                        self.screen.blit(item_text, (dropdown_x + 10, item_y + 5))
                    
                    item_y += 25
                
                break
            
            x += menu_width
    
    def draw_toolbar(self):
        """Draw left toolbar"""
        toolbar_width = self.TOOLBAR_WIDTH
        toolbar_height = WINDOW_HEIGHT - 30 - self.STATUS_BAR_HEIGHT
        toolbar_rect = pygame.Rect(0, 30, toolbar_width, toolbar_height)
        
        pygame.draw.rect(self.screen, Colors.PANEL_BG, toolbar_rect)
        pygame.draw.line(self.screen, Colors.SEPARATOR, (toolbar_width, 30),
                        (toolbar_width, WINDOW_HEIGHT - self.STATUS_BAR_HEIGHT), 1)
        
        # Tool buttons
        tools = [Tool.SELECT, Tool.CREATE_SECTOR, Tool.VERTEX_EDIT, Tool.ENTITY, Tool.LIGHT, Tool.GATE]
        button_y = 40
        button_size = 48
        button_margin = (toolbar_width - button_size) // 2
        
        mouse_pos = pygame.mouse.get_pos()
        
        for tool in tools:
            rect = pygame.Rect(button_margin, button_y, button_size, button_size)
            
            # Highlight active or hovered
            is_active = self.current_tool == tool
            is_hovered = rect.collidepoint(mouse_pos)
            
            if is_active:
                pygame.draw.rect(self.screen, Colors.BUTTON_ACTIVE, rect, border_radius=4)
            elif is_hovered:
                pygame.draw.rect(self.screen, Colors.BUTTON_HOVER, rect, border_radius=4)
            
            # Draw icon
            if tool in self.tool_icons:
                icon = self.tool_icons[tool]
                icon_rect = icon.get_rect(center=rect.center)
                self.screen.blit(icon, icon_rect)
            else:
                # Fallback text
                text = self.font_small.render(tool.name[:3], True, Colors.TEXT)
                text_rect = text.get_rect(center=rect.center)
                self.screen.blit(text, text_rect)
                
            # Draw active border LAST to ensure visibility
            if is_active:
                pygame.draw.rect(self.screen, Colors.ACCENT, rect, 2, border_radius=4)
            
            button_y += button_size + 10

    def handle_toolbar_click(self, pos):
        """Handle clicks on the toolbar"""
        toolbar_width = self.TOOLBAR_WIDTH
        if pos[0] > toolbar_width or pos[1] < 30 or pos[1] > WINDOW_HEIGHT - self.STATUS_BAR_HEIGHT:
            return False
            
        tools = [Tool.SELECT, Tool.CREATE_SECTOR, Tool.VERTEX_EDIT, Tool.ENTITY, Tool.LIGHT, Tool.GATE]
        button_y = 40
        button_size = 48
        button_margin = (toolbar_width - button_size) // 2
        
        for tool in tools:
            rect = pygame.Rect(button_margin, button_y, button_size, button_size)
            if rect.collidepoint(pos):
                self.current_tool = tool
                # Reset selection if switching to Create Sector (consistent with keypress)
                if tool == Tool.CREATE_SECTOR:
                    self.selected_sector = None
                    self.selected_wall = None
                    self.selected_vertices = []
                elif tool == Tool.ENTITY:
                    self.selected_sector = None
                    self.selected_wall = None
                    self.selected_vertices = []
                return True
            button_y += button_size + 10
            
        return True # Clicked toolbar but missed buttons, still consume event

    def draw_status_bar(self):
        """Draw bottom status bar"""
        bar_height = self.STATUS_BAR_HEIGHT
        y = WINDOW_HEIGHT - bar_height
        
        # Background
        pygame.draw.rect(self.screen, Colors.PANEL_BG, (0, y, WINDOW_WIDTH, bar_height))
        pygame.draw.line(self.screen, Colors.SEPARATOR, (0, y), (WINDOW_WIDTH, y), 1)
        
        # Status items
        x = 10
        
        # Current Tool
        tool_text = f"Tool: {self.current_tool.name}"
        t_surf = self.font_small.render(tool_text, True, Colors.TEXT)
        self.screen.blit(t_surf, (x, y + 5))
        x += 150
        
        # Grid / Snap
        grid_text = f"Grid: {'ON' if self.show_grid else 'OFF'} | Snap: {'ON' if self.snap_to_grid else 'OFF'}"
        g_surf = self.font_small.render(grid_text, True, Colors.TEXT)
        self.screen.blit(g_surf, (x, y + 5))
        x += 200
        
        # Zoom (Active Viewport)
        if self.active_viewport:
            zoom_text = f"Zoom: {self.active_viewport.zoom:.1f}x"
            z_surf = self.font_small.render(zoom_text, True, Colors.TEXT)
            self.screen.blit(z_surf, (x, y + 5))
        x += 100
        
        # Mouse Pos (World)
        if self.active_viewport:
            mx, my = pygame.mouse.get_pos()
            if self.active_viewport.rect.collidepoint((mx, my)):
                wx, wy = self.active_viewport.screen_to_world(mx, my)
                pos_text = f"Pos: {int(wx)}, {int(wy)}"
                p_surf = self.font_small.render(pos_text, True, Colors.TEXT)
                self.screen.blit(p_surf, (x, y + 5))
        
        # Message (Right aligned)
        if self.status_message:
            m_surf = self.font_small.render(self.status_message, True, Colors.ACCENT)
            self.screen.blit(m_surf, (WINDOW_WIDTH - m_surf.get_width() - 10, y + 5))
    
    def draw_viewport(self, vp):
        """Draw a viewport"""
        # Background
        pygame.draw.rect(self.screen, Colors.BG_MEDIUM, vp.rect)
        
        # Border
        border_color = Colors.ACCENT if vp.is_active else Colors.BG_LIGHT
        pygame.draw.rect(self.screen, border_color, vp.rect, 2)
        
        # Title bar
        title_rect = pygame.Rect(vp.rect.x, vp.rect.y, vp.rect.width, 25)
        pygame.draw.rect(self.screen, Colors.BG_LIGHT, title_rect)
        title_text = self.font_medium.render(vp.name, True, Colors.TEXT)
        self.screen.blit(title_text, (vp.rect.x + 5, vp.rect.y + 5))
        
        # Draw based on viewport type
        if vp.view_type == ViewType.CAMERA_3D:
            self.draw_3d_view(vp)
        else:
            self.draw_2d_view(vp)
    
    def draw_2d_view(self, vp):
        """Draw 2D orthographic view"""
        # Clip to viewport
        clip_rect = pygame.Rect(vp.rect.x, vp.rect.y + 25,
                               vp.rect.width, vp.rect.height - 25)
        self.screen.set_clip(clip_rect)
        
        # Draw grid
        if self.show_grid:
            self.draw_grid(vp)
        
        # Draw walls and sectors
        if vp.view_type == ViewType.TOP:
            # Top view: Draw walls as lines
            for i, wall in enumerate(self.walls):
                is_sector_selected = False
                if self.selected_sector is not None:
                    sec = self.sectors[self.selected_sector]
                    if sec.ws <= i < sec.we:
                        is_sector_selected = True
                self.draw_wall_2d(vp, wall, i == self.selected_wall, i == self.hover_wall, is_sector_selected)
            
            # Draw vertices
            for i, wall in enumerate(self.walls):
                for vertex_idx, (vx, vy) in enumerate([(wall.x1, wall.y1), (wall.x2, wall.y2)]):
                    self.draw_vertex_2d(vp, vx, vy, (i, vertex_idx) in self.selected_vertices, self.hover_vertex == (i, vertex_idx))
            
            # Draw sector creation preview
            if self.creating_sector and len(self.sector_vertices) > 0:
                self.draw_sector_creation_preview(vp)
            
            # Draw player
            self.draw_player_2d(vp)
            
            # Draw enemies
            for i, enemy in enumerate(self.enemies):
                self.draw_enemy_2d(vp, enemy, i == self.selected_enemy)
                
            # Draw pickups
            for i, pickup in enumerate(self.pickups):
                self.draw_pickup_2d(vp, pickup, i == self.selected_pickup)
            
            # Draw lights
            for i, light in enumerate(self.lights):
                if light.active:
                    self.draw_light_2d(vp, light, i == self.selected_light)
            
            # Draw gates
            for i, gate in enumerate(self.gates):
                if gate.active:
                    self.draw_gate_2d(vp, gate, i == self.selected_gate)
            
            # Draw switches
            for i, sw in enumerate(self.switches):
                if sw.active:
                    self.draw_switch_2d(vp, sw, i == self.selected_switch)
        
        elif vp.view_type in (ViewType.FRONT, ViewType.SIDE):
            # Front/Side view: Draw sectors as rectangles with height
            for sector_idx, sector in enumerate(self.sectors):
                is_selected = (sector_idx == self.selected_sector)
                
                for wall_idx in range(sector.ws, sector.we):
                    if wall_idx >= len(self.walls):
                        continue
                    
                    wall = self.walls[wall_idx]
                    
                    if vp.view_type == ViewType.FRONT:
                        # Front view: X-Z plane
                        x1, _ = vp.world_to_screen(wall.x1, 0)
                        x2, _ = vp.world_to_screen(wall.x2, 0)
                        _, y_bottom = vp.world_to_screen(0, sector.z1)
                        _, y_top = vp.world_to_screen(0, sector.z2)
                    else:  # SIDE
                        # Side view: Y-Z plane
                        x1, _ = vp.world_to_screen(wall.y1, 0)
                        x2, _ = vp.world_to_screen(wall.y2, 0)
                        _, y_bottom = vp.world_to_screen(0, sector.z1)
                        _, y_top = vp.world_to_screen(0, sector.z2)
                    
                    # Draw wall as vertical line with height
                    color = Colors.WALL_SELECTED if is_selected else Colors.WALL
                    width = 2 if is_selected else 1
                    
                    # Draw bottom and top edges
                    pygame.draw.line(self.screen, color, (x1, y_bottom), (x2, y_bottom), width)
                    pygame.draw.line(self.screen, color, (x1, y_top), (x2, y_top), width)
                    
                    # Draw vertical edges at endpoints
                    pygame.draw.line(self.screen, color, (x1, y_bottom), (x1, y_top), width)
                    pygame.draw.line(self.screen, color, (x2, y_bottom), (x2, y_top), width)
        
        # Reset clip
        self.screen.set_clip(None)

        # Draw selection box
        if self.selecting_box and vp.view_type == ViewType.TOP and vp.is_active:
             # Draw semi-transparent rect
             x = min(self.box_start[0], self.box_end[0])
             y = min(self.box_start[1], self.box_end[1])
             w = abs(self.box_end[0] - self.box_start[0])
             h = abs(self.box_end[1] - self.box_start[1])
             
             s = pygame.Surface((w, h), pygame.SRCALPHA)
             s.fill((128, 0, 128, 50)) # Purple transparent
             self.screen.blit(s, (x, y))
             pygame.draw.rect(self.screen, (128, 0, 128), (x, y, w, h), 1)
    
    def draw_grid(self, vp):
        """Draw grid in viewport"""
        grid_size = vp.grid_size
        
        # Calculate grid range
        world_x_min = vp.offset_x - vp.rect.width / (2 * vp.zoom)
        world_x_max = vp.offset_x + vp.rect.width / (2 * vp.zoom)
        world_y_min = vp.offset_y - vp.rect.height / (2 * vp.zoom)
        world_y_max = vp.offset_y + vp.rect.height / (2 * vp.zoom)
        
        # Snap to grid
        grid_x_min = int(world_x_min / grid_size) * grid_size
        grid_x_max = int(world_x_max / grid_size) * grid_size
        grid_y_min = int(world_y_min / grid_size) * grid_size
        grid_y_max = int(world_y_max / grid_size) * grid_size
        
        # Draw vertical lines
        x = grid_x_min
        while x <= grid_x_max:
            sx1, sy1 = vp.world_to_screen(x, world_y_min)
            sx2, sy2 = vp.world_to_screen(x, world_y_max)
            
            # Axis vs regular grid
            color = Colors.GRID_AXIS_X if x == 0 else Colors.GRID_DARK
            pygame.draw.line(self.screen, color, (sx1, sy1), (sx2, sy2), 1)
            x += grid_size
        
        # Draw horizontal lines
        y = grid_y_min
        while y <= grid_y_max:
            sx1, sy1 = vp.world_to_screen(world_x_min, y)
            sx2, sy2 = vp.world_to_screen(world_x_max, y)
            
            # Axis vs regular grid
            color = Colors.GRID_AXIS_Y if y == 0 else Colors.GRID_DARK
            pygame.draw.line(self.screen, color, (sx1, sy1), (sx2, sy2), 1)
            y += grid_size
    
    def draw_wall_2d(self, vp, wall, is_selected, is_hovered, is_sector_selected=False):
        """Draw a wall in 2D view"""
        # Draw in all 2D views
        if vp.view_type == ViewType.TOP:
            x1, y1 = vp.world_to_screen(wall.x1, wall.y1)
            x2, y2 = vp.world_to_screen(wall.x2, wall.y2)
        elif vp.view_type == ViewType.FRONT:
            # Front view: X-Z plane
            x1, y1 = vp.world_to_screen(wall.x1, 0)  # Use X and a default Z
            x2, y2 = vp.world_to_screen(wall.x2, 0)
        elif vp.view_type == ViewType.SIDE:
            # Side view: Y-Z plane
            x1, y1 = vp.world_to_screen(wall.y1, 0)  # Use Y and a default Z
            x2, y2 = vp.world_to_screen(wall.y2, 0)
        else:
            return
        
        # Check wall material type
        is_glass = getattr(wall, 'material', 0) == MATERIAL_GLASS
        is_fence = getattr(wall, 'material', 0) == MATERIAL_FENCE
        
        # Wall color based on state and material
        if is_selected:
            color = Colors.WALL_SELECTED
            width = 3
        elif is_sector_selected:
            color = Colors.WALL_SELECTED
            width = 2
        elif is_hovered:
            color = Colors.SELECT_HOVER
            width = 2
        elif is_glass:
            # Glass: cyan/teal color
            color = (0, 200, 220)
            width = 2
        elif is_fence:
            # Fence: yellow/orange
            color = (220, 180, 50)
            width = 1
        else:
            color = Colors.WALL
            width = 1
        
        # Draw glass walls with dashed line pattern
        if is_glass and not (is_selected or is_sector_selected or is_hovered):
            # Draw dashed line for glass
            dx = x2 - x1
            dy = y2 - y1
            length = math.sqrt(dx*dx + dy*dy)
            if length > 0:
                dash_len = 8
                gap_len = 4
                num_segments = int(length / (dash_len + gap_len))
                if num_segments < 1:
                    num_segments = 1
                for i in range(num_segments + 1):
                    t1 = i * (dash_len + gap_len) / length
                    t2 = min(1.0, (i * (dash_len + gap_len) + dash_len) / length)
                    if t1 < 1.0:
                        sx1 = int(x1 + dx * t1)
                        sy1 = int(y1 + dy * t1)
                        sx2 = int(x1 + dx * t2)
                        sy2 = int(y1 + dy * t2)
                        pygame.draw.line(self.screen, color, (sx1, sy1), (sx2, sy2), width)
                
                # Draw 'G' label at center for glass
                mid_x = (x1 + x2) / 2
                mid_y = (y1 + y2) / 2
                label = self.font_small.render("G", True, (0, 255, 255))
                self.screen.blit(label, (mid_x - 4, mid_y - 8))
        else:
            pygame.draw.line(self.screen, color, (x1, y1), (x2, y2), width)
        
        # Draw direction indicator (small tick)
        if is_selected or is_sector_selected or is_hovered:
            mid_x = (x1 + x2) / 2
            mid_y = (y1 + y2) / 2
            dx = x2 - x1
            dy = y2 - y1
            length = math.sqrt(dx*dx + dy*dy)
            if length > 0:
                # Normal vector
                nx = -dy / length * 5
                ny = dx / length * 5
                pygame.draw.line(self.screen, color, (mid_x, mid_y), (mid_x + nx, mid_y + ny), 1)
    
    def draw_vertex_2d(self, vp, vx, vy, is_selected, is_hovered):
        """Draw a vertex in 2D view"""
        # Draw in all 2D views
        if vp.view_type == ViewType.TOP:
            sx, sy = vp.world_to_screen(vx, vy)
        elif vp.view_type == ViewType.FRONT:
            # Front view: X-Z plane
            sx, sy = vp.world_to_screen(vx, 0)
        elif vp.view_type == ViewType.SIDE:
            # Side view: Y-Z plane
            sx, sy = vp.world_to_screen(vy, 0)
        else:
            return
        
        # Vertex size and color
        size = 4 if not (is_selected or is_hovered) else 6
        if is_selected:
            color = Colors.VERTEX_SELECTED
        elif is_hovered:
            color = Colors.SELECT_HOVER
        else:
            color = Colors.VERTEX
        
        pygame.draw.circle(self.screen, color, (sx, sy), size)
        pygame.draw.circle(self.screen, Colors.BG_DARK, (sx, sy), size - 1, 1)
    
    def draw_player_2d(self, vp):
        """Draw player in 2D view"""
        px, py = vp.world_to_screen(self.player.x, self.player.y)
        
        # Player position
        pygame.draw.circle(self.screen, Colors.PLAYER, (px, py), 5)
        
        # Player direction
        angle_rad = math.radians(self.player.a)
        dir_length = 15
        dx = int(px + math.sin(angle_rad) * dir_length)
        dy = int(py - math.cos(angle_rad) * dir_length)
        pygame.draw.line(self.screen, Colors.PLAYER, (px, py), (dx, dy), 2)

    def draw_enemy_2d(self, vp, enemy, is_selected):
        """Draw enemy in 2D view"""
        ex, ey = vp.world_to_screen(enemy.x, enemy.y)
        
        color = (255, 0, 0) # Red
        if enemy.enemy_type == 1: color = (0, 255, 255) # Cyan
        elif enemy.enemy_type == 2: color = (255, 0, 255) # Magenta
        
        size = 6 if is_selected else 5
        pygame.draw.circle(self.screen, color, (ex, ey), size)
        if is_selected:
            pygame.draw.circle(self.screen, (255, 255, 255), (ex, ey), size + 2, 1)

    def draw_pickup_2d(self, vp, pickup, is_selected):
        """Draw pickup in 2D view"""
        px, py = vp.world_to_screen(pickup.x, pickup.y)
        
        # Pickup colors based on type (simple vis)
        color = (0, 255, 0) # Green generic
        if pickup.pickup_type == 1: color = (255, 165, 0) # Orange
        elif pickup.pickup_type == 2: color = (0, 0, 255) # Blue
        
        size = 4 if is_selected else 3
        
        # Draw as small square
        rect = pygame.Rect(px - size, py - size, size*2, size*2)
        pygame.draw.rect(self.screen, color, rect)
        
        if is_selected:
            pygame.draw.rect(self.screen, (255, 255, 255), rect, 1)

    def draw_light_2d(self, vp, light, is_selected):
        """Draw light source in 2D view"""
        lx, ly = vp.world_to_screen(light.x, light.y)
        
        # Light color
        color = (light.r, light.g, light.b)
        
        # Draw radius indicator (scaled by zoom, but capped)
        radius_pixels = int(light.radius * vp.zoom * 0.15)
        radius_pixels = max(8, min(radius_pixels, 100))  # Clamp for visibility
        
        # Draw outer glow circle (semi-transparent)
        glow_surf = pygame.Surface((radius_pixels * 2, radius_pixels * 2), pygame.SRCALPHA)
        glow_color = (light.r, light.g, light.b, 60)
        pygame.draw.circle(glow_surf, glow_color, (radius_pixels, radius_pixels), radius_pixels)
        self.screen.blit(glow_surf, (lx - radius_pixels, ly - radius_pixels))
        
        # Draw radius ring
        pygame.draw.circle(self.screen, color, (lx, ly), radius_pixels, 1)
        
        # Draw center dot (bright)
        center_size = 6 if is_selected else 4
        pygame.draw.circle(self.screen, color, (lx, ly), center_size)
        
        # Draw selection highlight
        if is_selected:
            pygame.draw.circle(self.screen, (255, 255, 255), (lx, ly), center_size + 3, 2)
            # Also highlight the radius ring
            pygame.draw.circle(self.screen, (255, 255, 255), (lx, ly), radius_pixels + 2, 1)
        
        # Draw small light icon (sun rays)
        ray_len = 8 if is_selected else 5
        for angle in [0, 45, 90, 135, 180, 225, 270, 315]:
            rad = math.radians(angle)
            x1 = lx + int(math.cos(rad) * (center_size + 2))
            y1 = ly + int(math.sin(rad) * (center_size + 2))
            x2 = lx + int(math.cos(rad) * (center_size + ray_len))
            y2 = ly + int(math.sin(rad) * (center_size + ray_len))
            pygame.draw.line(self.screen, color, (x1, y1), (x2, y2), 1)

    def draw_gate_2d(self, vp, gate, is_selected):
        """Draw gate in 2D top view"""
        gx, gy = vp.world_to_screen(gate.x, gate.y)
        
        # Gate color (brown/door-like)
        color = (180, 120, 60) if not is_selected else (255, 200, 100)
        
        # Draw gate as a thick horizontal line (representing the door)
        half_width = int(gate.width * vp.zoom * 0.5)
        half_width = max(8, min(half_width, 80))  # Clamp for visibility
        
        # Draw the door bar
        pygame.draw.line(self.screen, color, 
                        (gx - half_width, gy), (gx + half_width, gy), 4)
        
        # Draw door ends/hinges
        pygame.draw.circle(self.screen, color, (gx - half_width, gy), 5)
        pygame.draw.circle(self.screen, color, (gx + half_width, gy), 5)
        
        # Draw center marker
        pygame.draw.circle(self.screen, (255, 255, 0), (gx, gy), 4)
        
        # Draw trigger radius (dashed circle would be ideal, but solid for simplicity)
        trigger_radius_px = int(gate.trigger_radius * vp.zoom * 0.5)
        trigger_radius_px = max(10, min(trigger_radius_px, 100))
        pygame.draw.circle(self.screen, (100, 100, 100), (gx, gy), trigger_radius_px, 1)
        
        # Selection highlight
        if is_selected:
            pygame.draw.circle(self.screen, (255, 255, 255), (gx, gy), 8, 2)
            # Show gate ID
            id_text = self.font_small.render(f"G{gate.gate_id}", True, (255, 255, 255))
            self.screen.blit(id_text, (gx + 10, gy - 10))

    def draw_switch_2d(self, vp, sw, is_selected):
        """Draw switch in 2D top view"""
        sx, sy = vp.world_to_screen(sw.x, sw.y)
        
        # Switch color (red for power switch)
        color = (200, 50, 50) if not is_selected else (255, 100, 100)
        
        # Draw switch as a small square with handle
        size = 8 if is_selected else 6
        rect = pygame.Rect(sx - size, sy - size, size * 2, size * 2)
        pygame.draw.rect(self.screen, color, rect)
        pygame.draw.rect(self.screen, (255, 255, 255), rect, 1)
        
        # Draw toggle indicator
        pygame.draw.line(self.screen, (0, 0, 0), 
                        (sx - size//2, sy), (sx + size//2, sy - size), 2)
        
        # Selection highlight
        if is_selected:
            pygame.draw.rect(self.screen, (255, 255, 0), rect.inflate(4, 4), 2)
            # Show linked gate ID
            link_text = self.font_small.render(f"→G{sw.linked_gate_id}", True, (255, 255, 100))
            self.screen.blit(link_text, (sx + size + 4, sy - 6))

    
    def draw_sector_creation_preview(self, vp):

        """Draw preview of sector being created"""
        if len(self.sector_vertices) == 0:
            return
        
        # Draw edges
        for i in range(len(self.sector_vertices)):
            x1, y1 = self.sector_vertices[i]
            x2, y2 = self.sector_vertices[(i + 1) % len(self.sector_vertices)] if i < len(self.sector_vertices) - 1 else pygame.mouse.get_pos()
            
            if i < len(self.sector_vertices) - 1:
                sx1, sy1 = vp.world_to_screen(x1, y1)
                sx2, sy2 = vp.world_to_screen(x2, y2)
            else:
                sx1, sy1 = vp.world_to_screen(x1, y1)
                wx, wy = vp.screen_to_world(pygame.mouse.get_pos()[0], pygame.mouse.get_pos()[1])
                if self.snap_to_grid:
                    wx, wy = vp.snap_to_grid(wx, wy)
                sx2, sy2 = vp.world_to_screen(wx, wy)
            
            pygame.draw.line(self.screen, Colors.SELECT, (sx1, sy1), (sx2, sy2), 2)
        
        # Draw vertices
        for vx, vy in self.sector_vertices:
            sx, sy = vp.world_to_screen(vx, vy)
            pygame.draw.circle(self.screen, Colors.SELECT, (sx, sy), 6)
        
        # Highlight first vertex if can close
        if len(self.sector_vertices) >= 3:
            first_x, first_y = self.sector_vertices[0]
            sx, sy = vp.world_to_screen(first_x, first_y)
            pygame.draw.circle(self.screen, Colors.ACCENT_BRIGHT, (sx, sy), 8, 2)
    
    def draw_3d_view(self, vp):
        """Draw 3D camera view"""
        # Clip to viewport
        clip_rect = pygame.Rect(vp.rect.x, vp.rect.y + 25,
                               vp.rect.width, vp.rect.height - 25)
        self.screen.set_clip(clip_rect)
        
        # Clear background
        pygame.draw.rect(self.screen, (0, 60, 130), clip_rect)
        
        # Simple 3D projection (basic wireframe)
        if len(self.walls) > 0:
            self.draw_3d_walls(vp)
            
        # Draw enemies in 3D
        if len(self.enemies) > 0:
            self.draw_3d_enemies(vp)
            
        # Draw pickups in 3D
        if len(self.pickups) > 0:
            self.draw_3d_pickups(vp)
        
        # Draw lights in 3D
        if len(self.lights) > 0:
            self.draw_3d_lights(vp)
        
        # Draw gates in 3D
        if len(self.gates) > 0:
            self.draw_3d_gates(vp)
        
        # Draw controls overlay if this is the active viewport
        if vp.is_active:
            self.draw_3d_controls_overlay(vp)
        
        # Reset clip
        self.screen.set_clip(None)
    
    def draw_3d_controls_overlay(self, vp):
        """Draw control hints in 3D view"""
        overlay_x = vp.rect.x + 10
        overlay_y = vp.rect.y + vp.rect.height - 120
        
        # Semi-transparent background
        overlay_bg = pygame.Surface((200, 100))
        overlay_bg.set_alpha(180)
        overlay_bg.fill(Colors.BG_DARK)
        self.screen.blit(overlay_bg, (overlay_x, overlay_y))
        
        # Draw border
        pygame.draw.rect(self.screen, Colors.ACCENT, (overlay_x, overlay_y, 200, 100), 1)
        
        # Controls text
        controls = [
            "WASD - Move",
            "Q/E - Up/Down",
            "Arrows - Look/Turn",
            "[/] - Floor Z (+/- 4)",
            "Shift+[/] - Ceil Z (+/- 4)",
            f"Pos: ({self.player.x}, {self.player.y}, {self.player.z})",
            f"Angle: {self.player.a} deg"
        ]
        
        y_offset = overlay_y + 5
        for control in controls:
            text = self.font_small.render(control, True, Colors.TEXT)
            self.screen.blit(text, (overlay_x + 5, y_offset))
            y_offset += 18

    def draw_3d_walls(self, vp):
        """Draw walls in 3D view"""
        # Camera setup
        cam_x = self.player.x
        cam_y = self.player.y
        cam_z = self.player.z
        cam_angle = math.radians(self.player.a)
        
        cos_a = math.cos(cam_angle)
        sin_a = math.sin(cam_angle)
        
        # Collect all walls with their distance
        walls_to_draw = []
        
        for sector in self.sectors:
            for wall_idx in range(sector.ws, sector.we):
                if wall_idx >= len(self.walls):
                    continue
                
                wall = self.walls[wall_idx]
                
                # Calculate distance to wall midpoint
                mid_x = (wall.x1 + wall.x2) / 2
                mid_y = (wall.y1 + wall.y2) / 2
                dist_sq = (mid_x - cam_x)**2 + (mid_y - cam_y)**2
                
                walls_to_draw.append((dist_sq, sector, wall, wall_idx))
        
        # Sort by distance (furthest first) - Painter's Algorithm
        walls_to_draw.sort(key=lambda x: x[0], reverse=True)
        
        # Draw sorted walls
        for _, sector, wall, wall_idx in walls_to_draw:
            # Transform to camera space
            rel_x1 = wall.x1 - cam_x
            rel_y1 = wall.y1 - cam_y
            rel_x2 = wall.x2 - cam_x
            rel_y2 = wall.y2 - cam_y
            
            # Rotate to camera space
            wx1 = rel_x1 * cos_a - rel_y1 * sin_a
            wy1 = rel_x1 * sin_a + rel_y1 * cos_a
            wx2 = rel_x2 * cos_a - rel_y2 * sin_a
            wy2 = rel_x2 * sin_a + rel_y2 * cos_a
            
            # Skip if behind camera
            if wy1 < 1 and wy2 < 1:
                continue
            
            # Clip behind camera
            if wy1 < 1:
                wx1, wy1, _ = self.clip_behind_camera(wx1, wy1, sector.z1 - cam_z, wx2, wy2, sector.z1 - cam_z)
            if wy2 < 1:
                wx2, wy2, _ = self.clip_behind_camera(wx2, wy2, sector.z2 - cam_z, wx1, wy1, sector.z2 - cam_z)
            
            # Project to screen
            scale = 200
            sx1 = int(vp.rect.x + vp.rect.width // 2 + wx1 * scale / wy1)
            sy1_bottom = int(vp.rect.y + vp.rect.height // 2 - (sector.z1 - cam_z) * scale / wy1)
            sy1_top = int(vp.rect.y + vp.rect.height // 2 - (sector.z2 - cam_z) * scale / wy1)
            
            sx2 = int(vp.rect.x + vp.rect.width // 2 + wx2 * scale / wy2)
            sy2_bottom = int(vp.rect.y + vp.rect.height // 2 - (sector.z1 - cam_z) * scale / wy2)
            sy2_top = int(vp.rect.y + vp.rect.height // 2 - (sector.z2 - cam_z) * scale / wy2)
            
            # Draw wall
            if self.textured_view and wall.wt < len(self.textures):
                self.draw_textured_wall(vp, wall, sx1, sx2, sy1_top, sy1_bottom, sy2_top, sy2_bottom, wx1, wy1, wx2, wy2)
            else:
                color = Colors.WALL_SELECTED if wall_idx == self.selected_wall else Colors.WALL
                
                # Draw vertical edges
                pygame.draw.line(self.screen, color, (sx1, sy1_bottom), (sx1, sy1_top), 1)
                pygame.draw.line(self.screen, color, (sx2, sy2_bottom), (sx2, sy2_top), 1)
                
                # Draw horizontal edges
                pygame.draw.line(self.screen, color, (sx1, sy1_bottom), (sx2, sy2_bottom), 1)
                pygame.draw.line(self.screen, color, (sx1, sy1_top), (sx2, sy2_top), 1)
    
    def draw_textured_wall(self, vp, wall, sx1, sx2, sy1_top, sy1_bottom, sy2_top, sy2_bottom, wx1, wy1, wx2, wy2):
        """Draw a textured wall using vertical strips"""
        texture = self.textures[wall.wt].frames[0]
        tex_w, tex_h = texture.get_size()
        
        # Ensure sx1 < sx2
        if sx1 > sx2:
            sx1, sx2 = sx2, sx1
            sy1_top, sy2_top = sy2_top, sy1_top
            sy1_bottom, sy2_bottom = sy2_bottom, sy1_bottom
            wx1, wx2 = wx2, wx1
            wy1, wy2 = wy2, wy1
            
        # Clip to viewport
        start_x = max(sx1, vp.rect.x)
        end_x = min(sx2, vp.rect.x + vp.rect.width)
        
        if start_x >= end_x:
            return
            
        # Perspective correct interpolation setup
        z1 = wy1
        z2 = wy2
        
        # Avoid division by zero
        if z1 < 0.1: z1 = 0.1
        if z2 < 0.1: z2 = 0.1
        
        inv_z1 = 1.0 / z1
        inv_z2 = 1.0 / z2
        
        # Texture coordinates (u)
        # Assuming texture repeats every 64 units or based on wall length
        # Calculate wall length for u mapping
        wall_len = math.sqrt((wall.x2 - wall.x1)**2 + (wall.y2 - wall.y1)**2)
        u1 = 0
        u2 = wall_len * wall.u # Scale u by wall length and texture scale
        
        u_over_z1 = u1 * inv_z1
        u_over_z2 = u2 * inv_z2
        
        # Draw strips
        step = 2 # Pixel width of each strip (increase for speed, decrease for quality)
        
        for x in range(start_x, end_x, step):
            # Calculate t (0.0 to 1.0) based on screen x relative to full wall width
            if sx2 == sx1: break
            t = (x - sx1) / (sx2 - sx1)
            
            # Interpolate 1/z
            inv_z = inv_z1 + t * (inv_z2 - inv_z1)
            if inv_z == 0: continue
            z = 1.0 / inv_z
            
            # Interpolate u/z and recover u
            u_over_z = u_over_z1 + t * (u_over_z2 - u_over_z1)
            u = u_over_z * z
            
            # Wrap u
            u = int(u) % tex_w
            
            # Interpolate y (screen space linear interpolation is "okay" for vertical walls)
            # For perfect perspective, we should project z, but linear screen Y is usually fine for Doom walls
            y_top = int(sy1_top + t * (sy2_top - sy1_top))
            y_bottom = int(sy1_bottom + t * (sy2_bottom - sy1_bottom))
            
            h = y_bottom - y_top
            if h <= 0: continue
            
            # Get texture column
            # Optimization: Pre-scale texture or use fast scaling
            # For now, just slice and scale
            col = texture.subsurface((u, 0, 1, tex_h))
            scaled_col = pygame.transform.scale(col, (step, h))
            
            # Blit
            # Clip y
            draw_y = y_top
            
            # Simple clipping
            if draw_y < vp.rect.y:
                # Skip top part
                offset = vp.rect.y - draw_y
                if offset >= h: continue
                area = pygame.Rect(0, offset, step, h - offset)
                self.screen.blit(scaled_col, (x, vp.rect.y), area)
            elif draw_y + h > vp.rect.y + vp.rect.height:
                # Skip bottom part
                h_clip = (draw_y + h) - (vp.rect.y + vp.rect.height)
                if h_clip >= h: continue
                area = pygame.Rect(0, 0, step, h - h_clip)
                self.screen.blit(scaled_col, (x, draw_y), area)
            else:
                self.screen.blit(scaled_col, (x, draw_y))
    
    def draw_3d_enemies(self, vp):
        """Draw enemies in 3D view (billboards)"""
        cam_x = self.player.x
        cam_y = self.player.y
        cam_z = self.player.z
        cam_angle = math.radians(self.player.a)
        
        cos_a = math.cos(cam_angle)
        sin_a = math.sin(cam_angle)
        
        # Sort enemies by distance
        sorted_enemies = []
        for i, enemy in enumerate(self.enemies):
            dist_sq = (enemy.x - cam_x)**2 + (enemy.y - cam_y)**2
            sorted_enemies.append((dist_sq, enemy))
        
        sorted_enemies.sort(key=lambda x: x[0], reverse=True)
        
        for _, enemy in sorted_enemies:
            # Transform to camera space
            rel_x = enemy.x - cam_x
            rel_y = enemy.y - cam_y
            
            # Rotate
            rot_x = rel_x * cos_a - rel_y * sin_a
            rot_y = rel_x * sin_a + rel_y * cos_a
            
            if rot_y < 1: continue
            
            # Project
            scale = 200
            screen_x = int(vp.rect.x + vp.rect.width // 2 + rot_x * scale / rot_y)
            screen_y = int(vp.rect.y + vp.rect.height // 2 - (enemy.z - cam_z) * scale / rot_y)
            
            size = int(64 * scale / rot_y)
            if size < 1: continue
            
            # Draw simple circle/rect for now
            # Use texture if available
            tex_idx = enemy.enemy_type
            if 0 <= tex_idx < len(self.enemy_textures):
                tex = self.enemy_textures[tex_idx]
                frame = tex.get_current_frame()
                
                # Scale frame
                # Assuming frame is roughly 64x64 or similar, we scale it to 'size'
                # Maintain aspect ratio
                w, h = frame.get_size()
                aspect = w / h
                
                draw_h = size
                draw_w = int(size * aspect)
                
                scaled_frame = pygame.transform.scale(frame, (draw_w, draw_h))
                
                screen_x_mid = screen_x
                screen_y_mid = screen_y
                
                dest_rect = scaled_frame.get_rect(center=(screen_x_mid, screen_y_mid))
                
                # Clip
                if dest_rect.colliderect(vp.rect):
                    self.screen.blit(scaled_frame, dest_rect)
            else:
                # Fallback
                color = (255, 0, 0)
                if enemy.enemy_type == 1: color = (0, 255, 255)
                elif enemy.enemy_type == 2: color = (255, 0, 255)
                elif enemy.enemy_type == 3: color = (255, 50, 0)
                
                rect = pygame.Rect(screen_x - size//2, screen_y - size//2, size, size)
                
                # Clip to viewport
                if rect.colliderect(vp.rect):
                    pygame.draw.rect(self.screen, color, rect)

    def draw_3d_pickups(self, vp):
        """Draw pickups in 3D view"""
        cam_x = self.player.x
        cam_y = self.player.y
        cam_z = self.player.z
        cam_angle = math.radians(self.player.a)
        
        cos_a = math.cos(cam_angle)
        sin_a = math.sin(cam_angle)
        
        sorted_pickups = []
        for i, pickup in enumerate(self.pickups):
            dist_sq = (pickup.x - cam_x)**2 + (pickup.y - cam_y)**2
            sorted_pickups.append((dist_sq, pickup))
        
        sorted_pickups.sort(key=lambda x: x[0], reverse=True)
        
        for _, pickup in sorted_pickups:
            rel_x = pickup.x - cam_x
            rel_y = pickup.y - cam_y
            
            rot_x = rel_x * cos_a - rel_y * sin_a
            rot_y = rel_x * sin_a + rel_y * cos_a
            
            if rot_y < 1: continue
            
            scale = 200
            screen_x = int(vp.rect.x + vp.rect.width // 2 + rot_x * scale / rot_y)
            screen_y = int(vp.rect.y + vp.rect.height // 2 - (pickup.z - cam_z) * scale / rot_y)
            
            size = int(32 * scale / rot_y) # Pickups are smaller
            if size < 1: continue
            
            # Use texture if available
            tex_idx = pickup.pickup_type
            if 0 <= tex_idx < len(self.pickup_textures):
                tex = self.pickup_textures[tex_idx]
                frame = tex.get_current_frame()
                scaled_frame = pygame.transform.scale(frame, (size, size))
                dest_rect = scaled_frame.get_rect(center=(screen_x, screen_y))
                if dest_rect.colliderect(vp.rect):
                    self.screen.blit(scaled_frame, dest_rect)
            else:
                rect = pygame.Rect(screen_x - size//2, screen_y - size//2, size, size)
                if rect.colliderect(vp.rect):
                    pygame.draw.rect(self.screen, (0, 255, 0), rect) # Fallback green box

    def draw_3d_lights(self, vp):
        """Draw lights in 3D view as glowing spheres"""
        cam_x = self.player.x
        cam_y = self.player.y
        cam_z = self.player.z
        cam_angle = math.radians(self.player.a)
        
        cos_a = math.cos(cam_angle)
        sin_a = math.sin(cam_angle)
        
        # Sort lights by distance (far to near for proper draw order)
        sorted_lights = []
        for i, light in enumerate(self.lights):
            if light.active:
                dist_sq = (light.x - cam_x)**2 + (light.y - cam_y)**2
                sorted_lights.append((dist_sq, light, i))
        
        sorted_lights.sort(key=lambda x: x[0], reverse=True)
        
        for _, light, idx in sorted_lights:
            rel_x = light.x - cam_x
            rel_y = light.y - cam_y
            
            # Rotate by camera angle
            rot_x = rel_x * cos_a - rel_y * sin_a
            rot_y = rel_x * sin_a + rel_y * cos_a
            
            # Skip if behind camera
            if rot_y < 1:
                continue
            
            # Project to screen
            scale = 200
            screen_x = int(vp.rect.x + vp.rect.width // 2 + rot_x * scale / rot_y)
            screen_y = int(vp.rect.y + vp.rect.height // 2 - (light.z - cam_z) * scale / rot_y)
            
            # Size based on distance (lights are bigger visual to show glow)
            glow_size = int(light.radius * 0.5 * scale / rot_y)
            glow_size = max(5, min(glow_size, 150))
            
            center_size = max(3, glow_size // 5)
            
            # Skip if too small
            if center_size < 1:
                continue
            
            # Light color
            color = (light.r, light.g, light.b)
            
            # Draw outer glow (multiple transparent circles)
            for i in range(3):
                glow_radius = glow_size - i * (glow_size // 4)
                if glow_radius > 0:
                    glow_surf = pygame.Surface((glow_radius * 2, glow_radius * 2), pygame.SRCALPHA)
                    alpha = 40 - i * 10
                    glow_color = (light.r, light.g, light.b, alpha)
                    pygame.draw.circle(glow_surf, glow_color, (glow_radius, glow_radius), glow_radius)
                    pos_x = screen_x - glow_radius
                    pos_y = screen_y - glow_radius
                    if vp.rect.collidepoint(screen_x, screen_y):
                        self.screen.blit(glow_surf, (pos_x, pos_y))
            
            # Draw center bright point
            if vp.rect.collidepoint(screen_x, screen_y):
                pygame.draw.circle(self.screen, color, (screen_x, screen_y), center_size)
                
                # Draw selection ring if selected
                is_selected = (idx == self.selected_light)
                if is_selected:
                    pygame.draw.circle(self.screen, (255, 255, 255), (screen_x, screen_y), center_size + 3, 2)

    def draw_3d_gates(self, vp):
        """Draw gates in 3D view as horizontal bars"""
        cam_x = self.player.x
        cam_y = self.player.y
        cam_z = self.player.z
        cam_angle = math.radians(self.player.a)
        
        cos_a = math.cos(cam_angle)
        sin_a = math.sin(cam_angle)
        
        # Sort gates by distance (far to near for proper draw order)
        sorted_gates = []
        for i, gate in enumerate(self.gates):
            if gate.active:
                dist_sq = (gate.x - cam_x)**2 + (gate.y - cam_y)**2
                sorted_gates.append((dist_sq, gate, i))
        
        sorted_gates.sort(key=lambda x: x[0], reverse=True)
        
        for _, gate, idx in sorted_gates:
            # Gate endpoints (perpendicular to X axis by default)
            half_width = gate.width // 2
            gx1, gy1 = gate.x - half_width, gate.y
            gx2, gy2 = gate.x + half_width, gate.y
            
            # Transform to view space
            rel_x1 = gx1 - cam_x
            rel_y1 = gy1 - cam_y
            rel_x2 = gx2 - cam_x
            rel_y2 = gy2 - cam_y
            
            # Rotate by camera angle
            rot_x1 = rel_x1 * cos_a - rel_y1 * sin_a
            rot_y1 = rel_x1 * sin_a + rel_y1 * cos_a
            rot_x2 = rel_x2 * cos_a - rel_y2 * sin_a
            rot_y2 = rel_x2 * sin_a + rel_y2 * cos_a
            
            # Skip if both endpoints behind camera
            if rot_y1 < 1 and rot_y2 < 1:
                continue
            
            # Clip to near plane
            if rot_y1 < 1:
                t = (1 - rot_y1) / (rot_y2 - rot_y1)
                rot_x1 = rot_x1 + t * (rot_x2 - rot_x1)
                rot_y1 = 1
            if rot_y2 < 1:
                t = (1 - rot_y2) / (rot_y1 - rot_y2)
                rot_x2 = rot_x2 + t * (rot_x1 - rot_x2)
                rot_y2 = 1
            
            # Project to screen
            scale = 200
            screen_x1 = int(vp.rect.x + vp.rect.width // 2 + rot_x1 * scale / rot_y1)
            screen_x2 = int(vp.rect.x + vp.rect.width // 2 + rot_x2 * scale / rot_y2)
            
            # Gate Z position (at z_closed, door is 40 units tall - same as game)
            gate_bottom = gate.z_closed
            gate_top = gate_bottom + 40
            
            # Project Y (vertical position on screen) for bottom and top at each endpoint
            screen_y1_bot = int(vp.rect.y + vp.rect.height // 2 - (gate_bottom - cam_z) * scale / rot_y1)
            screen_y1_top = int(vp.rect.y + vp.rect.height // 2 - (gate_top - cam_z) * scale / rot_y1)
            screen_y2_bot = int(vp.rect.y + vp.rect.height // 2 - (gate_bottom - cam_z) * scale / rot_y2)
            screen_y2_top = int(vp.rect.y + vp.rect.height // 2 - (gate_top - cam_z) * scale / rot_y2)
            
            # Color (brown for gate)
            is_selected = (idx == self.selected_gate)
            color = (255, 200, 100) if is_selected else (180, 120, 60)
            
            # Draw gate as a quad (4 lines connecting corners)
            # Corners: top-left, top-right, bottom-right, bottom-left
            tl = (screen_x1, screen_y1_top)
            tr = (screen_x2, screen_y2_top)
            br = (screen_x2, screen_y2_bot)
            bl = (screen_x1, screen_y1_bot)
            
            # Check if any point is in viewport
            if vp.rect.collidepoint(tl) or vp.rect.collidepoint(tr) or \
               vp.rect.collidepoint(br) or vp.rect.collidepoint(bl):
                pygame.draw.polygon(self.screen, color, [tl, tr, br, bl], 0)  # Filled
                pygame.draw.polygon(self.screen, (0, 0, 0), [tl, tr, br, bl], 1)  # Border
                
                # Draw selection highlight
                if is_selected:
                    pygame.draw.polygon(self.screen, (255, 255, 255), [tl, tr, br, bl], 2)

    
    def clip_behind_camera(self, x1, y1, z1, x2, y2, z2):

        """Clip line segment behind camera"""
        if y1 == 0:
            y1 = 1
        if y2 == 0:
            y2 = 1
        
        factor = (1 - y1) / (y2 - y1)
        x1 = x1 + factor * (x2 - x1)
        y1 = 1
        z1 = z1 + factor * (z2 - z1)
        
        return x1, y1, z1
    
    # ADDED: Color Picker Resources
    def init_color_picker_resources(self):
        # 1. Hue Bar (Horizontal) - 240x20
        self.picker_hue_surf = pygame.Surface((240, 20))
        for x in range(240):
            hue = x / 240.0
            r, g, b = colorsys.hsv_to_rgb(hue, 1, 1)
            pygame.draw.line(self.picker_hue_surf, (int(r*255), int(g*255), int(b*255)), (x, 0), (x, 20))
            
        # 2. SV Box Masks - 240x150
        # Saturation (Horizontal: White -> Transparent)
        self.picker_sv_sat_mask = pygame.Surface((240, 150), pygame.SRCALPHA)
        for x in range(240):
            # Left: Sat=0 (White). Right: Sat=1 (Color).
            # So start with White and fade alpha 255->0
            alpha = int(255 * (1.0 - (x/240.0)))
            pygame.draw.line(self.picker_sv_sat_mask, (255, 255, 255, alpha), (x, 0), (x, 150))
            
        # Value (Vertical: Transparent -> Black)
        self.picker_sv_val_mask = pygame.Surface((240, 150), pygame.SRCALPHA)
        for y in range(150):
            # Top: Val=1 (Color). Bottom: Val=0 (Black).
            # So start Transparent and fade to Black 0->255
            alpha = int(255 * (y/150.0))
            pygame.draw.line(self.picker_sv_val_mask, (0, 0, 0, alpha), (0, y), (240, y))

    def draw_color_picker_widget(self, light, x, y, w):
        """Draws integrated color picker and handles interaction."""
        # Calculate current HSV
        h, s, v = colorsys.rgb_to_hsv(light.r/255.0, light.g/255.0, light.b/255.0)
        
        # Dimensions
        hue_h = 20
        sv_h = 150
        margin = 10
        total_h = sv_h + margin + hue_h
        
        # Mouse Interaction check
        mx, my = pygame.mouse.get_pos()
        m_down = pygame.mouse.get_pressed()[0]
        
        if not m_down:
            self.picker_active_drag = None
            
        # === 1. SV BOX ===
        sv_rect = pygame.Rect(x, y, w, sv_h)
        # Background: Current Hue
        r, g, b = colorsys.hsv_to_rgb(h, 1, 1)
        pygame.draw.rect(self.screen, (int(r*255), int(g*255), int(b*255)), sv_rect)
        # Overlays
        # Scale cached surfaces to fit width
        if self.picker_sv_sat_mask.get_width() != w:
             s_mask = pygame.transform.scale(self.picker_sv_sat_mask, (w, sv_h))
             v_mask = pygame.transform.scale(self.picker_sv_val_mask, (w, sv_h))
        else:
             s_mask = self.picker_sv_sat_mask
             v_mask = self.picker_sv_val_mask
             
        self.screen.blit(s_mask, sv_rect)
        self.screen.blit(v_mask, sv_rect)
        pygame.draw.rect(self.screen, Colors.SEPARATOR, sv_rect, 1)
        
        # Interaction SV
        if m_down:
             # Check collision or drag state
             if (sv_rect.collidepoint(mx, my)) or (self.picker_active_drag == 'sv'):
                 if self.picker_active_drag is None: self.picker_active_drag = 'sv'
                 
                 if self.picker_active_drag == 'sv':
                     # Update S and V
                     rel_x = max(0, min(w, mx - x))
                     rel_y = max(0, min(sv_h, my - y))
                     
                     new_s = rel_x / w
                     new_v = 1.0 - (rel_y / sv_h)
                     
                     # Update Light
                     nr, ng, nb = colorsys.hsv_to_rgb(h, new_s, new_v)
                     light.r, light.g, light.b = int(nr*255), int(ng*255), int(nb*255)
                     s, v = new_s, new_v
        
        # Cursor SV
        cx = x + int(s * w)
        cy_pos = y + int((1.0 - v) * sv_h)
        pygame.draw.circle(self.screen, (0,0,0), (cx, cy_pos), 6, 2)
        pygame.draw.circle(self.screen, (255,255,255), (cx, cy_pos), 4, 1)

        # === 2. HUE BAR ===
        hue_y = y + sv_h + margin
        hue_rect = pygame.Rect(x, hue_y, w, hue_h)
        
        # Scale hue surf
        if self.picker_hue_surf.get_width() != w:
             h_surf = pygame.transform.scale(self.picker_hue_surf, (w, hue_h))
        else:
             h_surf = self.picker_hue_surf
             
        self.screen.blit(h_surf, hue_rect)
        pygame.draw.rect(self.screen, Colors.SEPARATOR, hue_rect, 1)
        
        # Interaction Hue
        if m_down:
             if (hue_rect.collidepoint(mx, my)) or (self.picker_active_drag == 'hue'):
                 if self.picker_active_drag is None: self.picker_active_drag = 'hue'
                 
                 if self.picker_active_drag == 'hue':
                     # Update Hue
                     rel_x = max(0, min(w, mx - x))
                     new_h = rel_x / w
                     
                     # Update Light
                     nr, ng, nb = colorsys.hsv_to_rgb(new_h, s, v)
                     light.r, light.g, light.b = int(nr*255), int(ng*255), int(nb*255)
                     h = new_h
                     
        # Cursor Hue
        hx = x + int(h * w)
        pygame.draw.rect(self.screen, (0,0,0), (hx-2, hue_y, 4, hue_h), 1)
        pygame.draw.rect(self.screen, (255,255,255), (hx-1, hue_y, 2, hue_h))
        
        return total_h

    def draw_properties_panel(self):
        """Draw right properties panel (Unity-Style Inspector)"""
        panel_width = 280
        panel_height = WINDOW_HEIGHT - 30 - 25
        panel_x = WINDOW_WIDTH - panel_width
        panel_y = 30
        
        # Background
        panel_rect = pygame.Rect(panel_x, panel_y, panel_width, panel_height)
        pygame.draw.rect(self.screen, (30, 30, 30), panel_rect) # Darker bg
        pygame.draw.line(self.screen, (20, 20, 20), (panel_x, 30), (panel_x, WINDOW_HEIGHT - 25), 1) # Shadow line
        
        # Reset interaction lists
        self.property_items = []
        self.texture_nav_buttons = []
        self.ui_buttons = []
        
        # Helper Variables
        cx = panel_x + 10 # Content X
        cw = panel_width - 20 # Content Width
        cy = panel_y + 10 # Content Y cursors
        
        # --- Helpers ---
        def draw_header(text):
            nonlocal cy
            # Background strip
            h_rect = pygame.Rect(panel_x, cy, panel_width, 28)
            pygame.draw.rect(self.screen, (45, 45, 48), h_rect)
            pygame.draw.line(self.screen, (60, 60, 60), (panel_x, cy), (panel_x + panel_width, cy), 1) # Top highlight
            pygame.draw.line(self.screen, (20, 20, 20), (panel_x, cy+27), (panel_x + panel_width, cy+27), 1) # Bottom shadow
            
            # Text
            surf = self.font_large.render(text.upper(), True, (220, 220, 220))
            self.screen.blit(surf, (cx, cy + 4))
            cy += 35
            
        def draw_section(text):
            nonlocal cy
            surf = self.font_medium.render(text, True, Colors.ACCENT_BRIGHT)
            self.screen.blit(surf, (cx, cy))
            pygame.draw.line(self.screen, (60, 60, 60), (cx, cy + 20), (cx + cw, cy + 20), 1)
            cy += 28

        def draw_field(label, target, attr, value):
            nonlocal cy
            # Label
            l_surf = self.font_small.render(label, True, (180, 180, 180))
            self.screen.blit(l_surf, (cx, cy + 3))
            
            # Input Box
            input_w = 100
            input_h = 22
            input_x = panel_x + panel_width - 10 - input_w
            rect = pygame.Rect(input_x, cy, input_w, input_h)
            
            # Editing state
            is_editing = (self.edit_field and self.edit_field.get('attr') == attr and self.edit_field.get('target') is target)
            
            bg_color = (255, 255, 255) if is_editing else (40, 40, 40)
            text_color = (0, 0, 0) if is_editing else (220, 220, 220)
            border_color = Colors.ACCENT if is_editing else (60, 60, 60)
            
            pygame.draw.rect(self.screen, bg_color, rect, border_radius=3)
            pygame.draw.rect(self.screen, border_color, rect, 1, border_radius=3)
            
            display_val = self.edit_buffer if is_editing else str(value)
            v_surf = self.font_small.render(display_val, True, text_color)
            
            # Clip text
            self.screen.set_clip(rect)
            self.screen.blit(v_surf, (input_x + 5, cy + 3))
            self.screen.set_clip(None)
            
            # Hitbox
            full_rect = pygame.Rect(panel_x, cy, panel_width, input_h)
            self.property_items.append({'rect': full_rect, 'target': target, 'attr': attr})
            
            cy += 26

        def draw_info(label, value_text):
            nonlocal cy
            l_surf = self.font_small.render(label, True, (150, 150, 150))
            v_surf = self.font_small.render(value_text, True, (220, 220, 220))
            self.screen.blit(l_surf, (cx, cy))
            self.screen.blit(v_surf, (cx + 80, cy))
            cy += 20

        def draw_texture_selector(label, current_idx, texture_list, target_obj, attr_name):
            nonlocal cy
            draw_section(label)
            
            # Preview Box
            preview_size = 140
            p_rect = pygame.Rect(panel_x + (panel_width - preview_size)//2, cy, preview_size, preview_size)
            pygame.draw.rect(self.screen, (50, 50, 50), p_rect) # Bg
            pygame.draw.rect(self.screen, (0, 0, 0), p_rect, 1) # Border
            
            # Texture Image
            if texture_list:
                tex = texture_list[current_idx % len(texture_list)]
                frame = tex.get_current_frame()
                scaled = pygame.transform.smoothscale(frame, (preview_size-4, preview_size-4))
                self.screen.blit(scaled, (p_rect.x+2, p_rect.y+2))
                
                # Name overlay
                name_bg = pygame.Surface((preview_size, 20))
                name_bg.fill((0, 0, 0))
                name_bg.set_alpha(150)
                self.screen.blit(name_bg, (p_rect.x, p_rect.y + preview_size - 20))
                name_surf = self.font_small.render(tex.name, True, (255, 255, 255))
                self.screen.blit(name_surf, (p_rect.x + 5, p_rect.y + preview_size - 18))
            else:
                s_surf = self.font_small.render("No Texture", True, (150, 150, 150))
                self.screen.blit(s_surf, (p_rect.centerx - s_surf.get_width()//2, p_rect.centery))
                
            cy += preview_size + 10
            
            # Nav Buttons
            btn_w = 80
            btn_h = 24
            prev_x = panel_x + 20
            next_x = panel_x + panel_width - 20 - btn_w
            
            for rx, label, delta in [(prev_x, "Previous", -1), (next_x, "Next", 1)]:
                btn_rect = pygame.Rect(rx, cy, btn_w, btn_h)
                is_hover = btn_rect.collidepoint(pygame.mouse.get_pos())
                color = Colors.BUTTON_HOVER if is_hover else Colors.BUTTON
                pygame.draw.rect(self.screen, color, btn_rect, border_radius=4)
                
                t_surf = self.font_small.render(label, True, (255, 255, 255))
                t_rect = t_surf.get_rect(center=btn_rect.center)
                self.screen.blit(t_surf, t_rect)
                
                # Check collision later
                # HACK: Using existing texture_nav_buttons logic which expects just rect and delta
                # BUT logic differs for global (defaults) vs selected object.
                # In handle_property_click we assumed only one texture selector active at a time (selected obj OR tool default).
                # That assumption holds true mostly.
                
                # If we are targeting a specific object attribute (like enemy.enemy_type), handle_property_click needs to know that.
                # The existing logic in handle_property_click handles selected_wall, Create Sector tool, and Entity tool.
                # It doesn't genericize "target object".
                # For now, let's stick to the current rigid logic in handle_property_click but make the UI nicer here.
                # Or we can use closure in ui_buttons!
                
                def make_action(d=delta):
                     if attr_name == 'wt': # Wall Texture
                         obj = target_obj
                         obj.wt = (obj.wt + d) % max(1, len(texture_list))
                         self.prop_wall_texture = obj.wt
                     elif attr_name == 'st': # Sector Texture (new support maybe?)
                         pass # Not implemented fully in handle_click?
                     elif attr_name == 'prop_wall_texture':
                         self.prop_wall_texture = (self.prop_wall_texture + d) % max(1, len(texture_list))
                     elif attr_name == 'enemy_type':
                         target_obj.enemy_type = (target_obj.enemy_type + d) % max(1, len(texture_list))
                         self.prop_enemy_type = target_obj.enemy_type
                     elif attr_name == 'pickup_type':
                         target_obj.pickup_type = (target_obj.pickup_type + d) % max(1, len(texture_list))
                         self.prop_pickup_type = target_obj.pickup_type
                     elif attr_name == 'prop_enemy_type':
                         self.prop_enemy_type = (self.prop_enemy_type + d) % max(1, len(texture_list))
                     elif attr_name == 'prop_pickup_type':
                         self.prop_pickup_type = (self.prop_pickup_type + d) % max(1, len(texture_list))
                         
                self.ui_buttons.append({'rect': btn_rect, 'action': make_action})
                
            cy += 35

        def draw_checkbox(label, target, attr):
            nonlocal cy
            # Label
            l_surf = self.font_small.render(label, True, (180, 180, 180))
            self.screen.blit(l_surf, (cx, cy + 5))
            
            # Checkbox
            cb_size = 20
            cb_x = panel_x + panel_width - 10 - cb_size
            cb_rect = pygame.Rect(cb_x, cy, cb_size, cb_size)
            
            val = getattr(target, attr)
            is_checked = (val == 1)
            
            # Draw box
            pygame.draw.rect(self.screen, (40, 40, 40), cb_rect, border_radius=4)
            pygame.draw.rect(self.screen, (60, 60, 60), cb_rect, 1, border_radius=4)
            
            if is_checked:
                # Draw checkmark
                color = Colors.ACCENT_BRIGHT
                pygame.draw.rect(self.screen, color, (cb_x+4, cy+4, cb_size-8, cb_size-8), border_radius=2)
            
            def toggle_action():
                new_val = 0 if val == 1 else 1
                setattr(target, attr, new_val)
                # Sync props if needed
                if isinstance(target, Sector):
                    self.prop_sector_tag = new_val
                elif attr == 'prop_sector_tag':
                    self.prop_sector_tag = new_val
            
            self.ui_buttons.append({'rect': cb_rect, 'action': toggle_action})
            cy += 28

        def draw_mode_switch():
            nonlocal cy
            # Draw a toggle switch for Entity Mode
            draw_section("Values")
            
            # Switch Container
            sw_w = 160
            sw_h = 30
            sw_x = panel_x + (panel_width - sw_w)//2
            sw_rect = pygame.Rect(sw_x, cy, sw_w, sw_h)
            pygame.draw.rect(self.screen, (40, 40, 40), sw_rect, border_radius=15)
            pygame.draw.rect(self.screen, (60, 60, 60), sw_rect, 1, border_radius=15)
            
            # Active side
            half_w = sw_w // 2
            if self.entity_mode == "ENEMY":
                active_rect = pygame.Rect(sw_x, cy, half_w, sw_h)
                color = (200, 50, 50)
            else:
                active_rect = pygame.Rect(sw_x + half_w, cy, half_w, sw_h)
                color = (50, 200, 50)
                
            pygame.draw.rect(self.screen, color, active_rect, border_radius=15)
            
            # Text
            e_surf = self.font_small.render("ENEMY", True, (255, 255, 255))
            p_surf = self.font_small.render("PICKUP", True, (255, 255, 255))
            
            self.screen.blit(e_surf, (sw_x + (half_w - e_surf.get_width())//2, cy + 8))
            self.screen.blit(p_surf, (sw_x + half_w + (half_w - p_surf.get_width())//2, cy + 8))
            
            # Button Action
            def toggle_mode():
                self.entity_mode = "PICKUP" if self.entity_mode == "ENEMY" else "ENEMY"
                
            self.ui_buttons.append({'rect': sw_rect, 'action': toggle_mode})
            cy += 40

        # --- Content Determination ---
        
        # 1. Selection takes precedence
        if self.selected_sector is not None:
             sector = self.sectors[self.selected_sector]
             draw_header(f"SECTOR #{self.selected_sector}")
             
             draw_section("Geometry")
             draw_info("Walls", f"{sector.we - sector.ws}")
             draw_field("Floor Z", sector, 'z1', sector.z1)
             draw_field("Ceil Z", sector, 'z2', sector.z2)
             draw_checkbox("Is Stair", sector, 'tag')
             
             draw_section("Surface")
             draw_field("Texture ID", sector, 'st', sector.st)
             draw_field("Scale", sector, 'ss', sector.ss)
             
             if self.selected_wall is not None:
                 wall = self.walls[self.selected_wall]
                 cy += 10
                 draw_header(f"WALL #{self.selected_wall}")
                 
                 draw_section("Coordinates")
                 draw_info("Start", f"{wall.x1}, {wall.y1}")
                 draw_info("End", f"{wall.x2}, {wall.y2}")
                 
                 draw_section("Mapping")
                 draw_field("Offset U", wall, 'u', wall.u)
                 draw_field("Offset V", wall, 'v', wall.v)
                 
                 # Glass checkbox for selected wall
                 draw_section("Material")
                 wall_is_glass = getattr(wall, 'material', 0) == MATERIAL_GLASS
                 
                 checkbox_rect = pygame.Rect(panel_x + 10, cy, 20, 20)
                 pygame.draw.rect(self.screen, (40, 40, 40), checkbox_rect, border_radius=3)
                 pygame.draw.rect(self.screen, (60, 60, 60), checkbox_rect, 1, border_radius=3)
                 if wall_is_glass:
                     pygame.draw.line(self.screen, (0, 255, 150), (checkbox_rect.x + 4, checkbox_rect.y + 10), 
                                    (checkbox_rect.x + 8, checkbox_rect.y + 14), 2)
                     pygame.draw.line(self.screen, (0, 255, 150), (checkbox_rect.x + 8, checkbox_rect.y + 14), 
                                    (checkbox_rect.x + 16, checkbox_rect.y + 5), 2)
                 
                 lbl = self.font_small.render("Is Glass", True, (200, 200, 200))
                 self.screen.blit(lbl, (panel_x + 35, cy + 2))
                 self.property_items.append({'rect': pygame.Rect(panel_x, cy, panel_width, 22), 
                                           'target': wall, 'attr': 'material', 'toggle_wall_glass': True})
                 cy += 28
                 
                 # Show texture if not glass, otherwise show glass properties
                 if not wall_is_glass:
                     draw_texture_selector("Wall Texture", wall.wt, self.textures, wall, 'wt')
                 else:
                     # Glass properties for selected wall
                     draw_section("Glass Tint")
                     draw_field("Red", wall, 'tint_r', getattr(wall, 'tint_r', 220))
                     draw_field("Green", wall, 'tint_g', getattr(wall, 'tint_g', 230))
                     draw_field("Blue", wall, 'tint_b', getattr(wall, 'tint_b', 240))
                     
                     # Color preview
                     tint_r = getattr(wall, 'tint_r', 220)
                     tint_g = getattr(wall, 'tint_g', 230)
                     tint_b = getattr(wall, 'tint_b', 240)
                     preview_rect = pygame.Rect(panel_x + 10, cy, panel_width - 20, 25)
                     pygame.draw.rect(self.screen, (tint_r, tint_g, tint_b), preview_rect, border_radius=3)
                     pygame.draw.rect(self.screen, Colors.TEXT, preview_rect, 1, border_radius=3)
                     cy += 32
                     
                     draw_section("Transparency")
                     draw_field("Opacity", wall, 'opacity', getattr(wall, 'opacity', 80))

        elif self.selected_enemy is not None and self.selected_enemy < len(self.enemies):
            enemy = self.enemies[self.selected_enemy]
            draw_header(f"ENEMY #{self.selected_enemy}")
            
            draw_section("Transform")
            draw_field("Pos X", enemy, 'x', enemy.x)
            draw_field("Pos Y", enemy, 'y', enemy.y)
            draw_field("Pos Z", enemy, 'z', enemy.z)
            
            draw_texture_selector("Enemy Type", enemy.enemy_type, self.enemy_textures, enemy, 'enemy_type')

        elif self.selected_pickup is not None and self.selected_pickup < len(self.pickups):
            pickup = self.pickups[self.selected_pickup]
            draw_header(f"PICKUP #{self.selected_pickup}")
            
            draw_section("Transform")
            draw_field("Pos X", pickup, 'x', pickup.x)
            draw_field("Pos Y", pickup, 'y', pickup.y)
            draw_field("Pos Z", pickup, 'z', pickup.z)
            
            draw_texture_selector("Pickup Type", pickup.pickup_type, self.pickup_textures, pickup, 'pickup_type')

        # 2. Tool Defaults
        elif self.current_tool == Tool.CREATE_SECTOR:
            draw_header("TOOL: SECTOR")
            
            draw_section("Defaults")
            draw_field("Floor Z", self, 'prop_sector_z1', self.prop_sector_z1)
            draw_field("Ceil Z", self, 'prop_sector_z2', self.prop_sector_z2)
            draw_checkbox("Is Stair", self, 'prop_sector_tag')
            
            # Show wall texture only if NOT glass
            if self.prop_wall_material != MATERIAL_GLASS:
                draw_texture_selector("Wall Texture", self.prop_wall_texture, self.textures, self, 'prop_wall_texture')
            
            # Glass checkbox
            draw_section("Material")
            
            # Create a checkbox function for glass toggle
            def toggle_glass():
                if self.prop_wall_material == MATERIAL_GLASS:
                    self.prop_wall_material = MATERIAL_SOLID
                else:
                    self.prop_wall_material = MATERIAL_GLASS
            
            is_glass = self.prop_wall_material == MATERIAL_GLASS
            
            # Draw checkbox
            checkbox_rect = pygame.Rect(panel_x + 10, cy, 20, 20)
            pygame.draw.rect(self.screen, (40, 40, 40), checkbox_rect, border_radius=3)
            pygame.draw.rect(self.screen, (60, 60, 60), checkbox_rect, 1, border_radius=3)
            if is_glass:
                # Draw checkmark
                pygame.draw.line(self.screen, (0, 255, 150), (checkbox_rect.x + 4, checkbox_rect.y + 10), 
                               (checkbox_rect.x + 8, checkbox_rect.y + 14), 2)
                pygame.draw.line(self.screen, (0, 255, 150), (checkbox_rect.x + 8, checkbox_rect.y + 14), 
                               (checkbox_rect.x + 16, checkbox_rect.y + 5), 2)
            
            label = self.font_small.render("Is Glass Wall", True, (200, 200, 200))
            self.screen.blit(label, (panel_x + 35, cy + 2))
            
            # Add click handler for checkbox
            full_rect = pygame.Rect(panel_x, cy, panel_width, 22)
            self.property_items.append({'rect': full_rect, 'target': self, 'attr': 'prop_wall_material', 'toggle_glass': True})
            cy += 28
            
            # Extensive glass menu (only shown when glass is enabled)
            if is_glass:
                draw_section("Glass Properties")
                
                # Tint preset buttons
                tint_y = cy
                preset_names = list(GLASS_TINT_PRESETS.keys())
                button_w = (panel_width - 20) // len(preset_names)
                for i, preset in enumerate(preset_names):
                    btn_x = panel_x + 10 + i * button_w
                    btn_rect = pygame.Rect(btn_x, cy, button_w - 4, 24)
                    
                    is_selected = self.prop_wall_tint_preset == preset
                    bg_color = GLASS_TINT_PRESETS[preset]
                    
                    pygame.draw.rect(self.screen, bg_color, btn_rect, border_radius=3)
                    if is_selected:
                        pygame.draw.rect(self.screen, (255, 255, 255), btn_rect, 2, border_radius=3)
                    else:
                        pygame.draw.rect(self.screen, (60, 60, 60), btn_rect, 1, border_radius=3)
                    
                    # Label (first letter only to save space)
                    lbl = self.font_small.render(preset[0], True, (0, 0, 0))
                    self.screen.blit(lbl, (btn_x + button_w // 2 - 4, cy + 4))
                    
                    # Add click handler for this preset button
                    self.property_items.append({
                        'rect': btn_rect, 
                        'target': self, 
                        'attr': 'prop_wall_tint_preset',
                        'tint_preset': preset
                    })
                cy += 30
                
                # Draw preset names below
                for i, preset in enumerate(preset_names):
                    btn_x = panel_x + 10 + i * button_w
                    lbl = self.font_small.render(preset, True, (150, 150, 150))
                    self.screen.blit(lbl, (btn_x, cy))
                cy += 18
                
                # RGB sliders
                draw_section("Custom Tint (RGB)")
                draw_field("Red", self, 'prop_wall_tint_r', self.prop_wall_tint_r)
                draw_field("Green", self, 'prop_wall_tint_g', self.prop_wall_tint_g)
                draw_field("Blue", self, 'prop_wall_tint_b', self.prop_wall_tint_b)
                
                # Color preview
                preview_rect = pygame.Rect(panel_x + 10, cy, panel_width - 20, 30)
                pygame.draw.rect(self.screen, (self.prop_wall_tint_r, self.prop_wall_tint_g, self.prop_wall_tint_b), preview_rect, border_radius=5)
                pygame.draw.rect(self.screen, Colors.TEXT, preview_rect, 1, border_radius=5)
                
                # Label on color preview
                preview_label = self.font_small.render("Color Preview", True, (0, 0, 0))
                self.screen.blit(preview_label, (preview_rect.x + 5, preview_rect.y + 8))
                cy += 38
                
                # Opacity section
                draw_section("Transparency")
                draw_field("Opacity (0-255)", self, 'prop_wall_opacity', self.prop_wall_opacity)
                
                # Opacity bar visualization
                bar_rect = pygame.Rect(panel_x + 10, cy, panel_width - 20, 16)
                pygame.draw.rect(self.screen, (30, 30, 30), bar_rect, border_radius=3)
                fill_w = int((self.prop_wall_opacity / 255) * (panel_width - 20))
                fill_rect = pygame.Rect(panel_x + 10, cy, fill_w, 16)
                pygame.draw.rect(self.screen, (0, 180, 220), fill_rect, border_radius=3)
                pygame.draw.rect(self.screen, (60, 60, 60), bar_rect, 1, border_radius=3)
                
                # Opacity percentage label
                opacity_pct = int((self.prop_wall_opacity / 255) * 100)
                pct_label = self.font_small.render(f"{opacity_pct}%", True, (255, 255, 255))
                self.screen.blit(pct_label, (bar_rect.x + bar_rect.width // 2 - 12, cy))
                cy += 24
                
                # Glass info
                draw_section("Info")
                draw_info("Effect", "Fresnel + Specular")
                draw_info("Render", "Deferred blend")

        elif self.current_tool == Tool.ENTITY:
            draw_header("TOOL: ENTITY")
            draw_mode_switch()
            
            if self.entity_mode == "ENEMY":
                draw_texture_selector("Default Enemy", self.prop_enemy_type, self.enemy_textures, self, 'prop_enemy_type')
            else:
                draw_texture_selector("Default Pickup", self.prop_pickup_type, self.pickup_textures, self, 'prop_pickup_type')

        elif self.selected_light is not None and self.selected_light < len(self.lights):
            # Light is selected - show its properties
            light = self.lights[self.selected_light]
            draw_header(f"LIGHT #{self.selected_light}")
            
            draw_section("Transform")
            draw_field("Pos X", light, 'x', light.x)
            draw_field("Pos Y", light, 'y', light.y)
            draw_field("Pos Z", light, 'z', light.z)
            
            draw_section("Lighting")
            draw_field("Radius", light, 'radius', light.radius)
            draw_field("Intensity", light, 'intensity', light.intensity)
            
            # Color preview box (Clickable)
            color_box_h = 24
            # Color Picker Widget
            picker_h = self.draw_color_picker_widget(light, panel_x + 20, cy, panel_width - 40)
            cy += picker_h + 10
            
            draw_section("Color (RGB)")
            draw_field("Red", light, 'r', light.r)
            draw_field("Green", light, 'g', light.g)
            draw_field("Blue", light, 'b', light.b)
            
            draw_section("Effects")
            draw_field("Flicker", light, 'flicker', light.flicker)
            
            # Flicker name hint
            flicker_name = self.flicker_names[light.flicker] if light.flicker < len(self.flicker_names) else "Unknown"
            name_surf = self.font_small.render(f"({flicker_name})", True, Colors.TEXT_DIM)
            self.screen.blit(name_surf, (panel_x + panel_width - name_surf.get_width() - 12, cy - 24))

        elif self.current_tool == Tool.LIGHT:
            draw_header("TOOL: LIGHT")
            
            draw_section("Click to place a light")
            
            # Color preview box
            color_box_h = 30
            color_box_rect = pygame.Rect(panel_x + 20, cy, panel_width - 40, color_box_h)
            pygame.draw.rect(self.screen, (self.prop_light_r, self.prop_light_g, self.prop_light_b), color_box_rect)
            pygame.draw.rect(self.screen, Colors.SEPARATOR, color_box_rect, 1)
            cy += color_box_h + 12
            
            draw_section("Default Color")
            draw_field("Red", self, 'prop_light_r', self.prop_light_r)
            draw_field("Green", self, 'prop_light_g', self.prop_light_g)
            draw_field("Blue", self, 'prop_light_b', self.prop_light_b)
            
            # Color presets
            draw_section("Presets")
            preset_x = panel_x + 12
            preset_y = cy
            preset_size = 24
            preset_gap = 4
            
            for i, (name, r, g, b) in enumerate(self.light_presets):
                x = preset_x + (i % 4) * (preset_size + preset_gap)
                y = preset_y + (i // 4) * (preset_size + preset_gap)
                rect = pygame.Rect(x, y, preset_size, preset_size)
                pygame.draw.rect(self.screen, (r, g, b), rect)
                pygame.draw.rect(self.screen, Colors.SEPARATOR, rect, 1)
                
                # Highlight if current color matches
                if r == self.prop_light_r and g == self.prop_light_g and b == self.prop_light_b:
                    pygame.draw.rect(self.screen, (255, 255, 255), rect, 2)
                
                # Store button for click
                def make_preset_action(pr, pg, pb):
                    def action():
                        self.prop_light_r = pr
                        self.prop_light_g = pg
                        self.prop_light_b = pb
                        # Also update selected light if any
                        if self.selected_light is not None and self.selected_light < len(self.lights):
                            self.lights[self.selected_light].r = pr
                            self.lights[self.selected_light].g = pg
                            self.lights[self.selected_light].b = pb
                    return action
                
                self.ui_buttons.append({'rect': rect, 'action': make_preset_action(r, g, b)})
            
            cy = preset_y + ((len(self.light_presets) + 3) // 4) * (preset_size + preset_gap) + 8
            
            draw_section("Properties")
            draw_field("Radius", self, 'prop_light_radius', self.prop_light_radius)
            draw_field("Intensity", self, 'prop_light_intensity', self.prop_light_intensity)
            draw_field("Flicker", self, 'prop_light_flicker', self.prop_light_flicker)
            
            # Flicker name hint
            flicker_name = self.flicker_names[self.prop_light_flicker] if self.prop_light_flicker < len(self.flicker_names) else "Unknown"
            name_surf = self.font_small.render(f"({flicker_name})", True, Colors.TEXT_DIM)
            self.screen.blit(name_surf, (panel_x + panel_width - name_surf.get_width() - 12, cy - 24))
            
            # Light count info
            cy += 10
            draw_info("Lights", str(len(self.lights)))

        elif self.selected_gate is not None and self.selected_gate < len(self.gates):
            # Gate is selected - show its properties
            gate = self.gates[self.selected_gate]
            draw_header(f"GATE #{self.selected_gate}")
            
            draw_section("Transform")
            draw_field("Pos X", gate, 'x', gate.x)
            draw_field("Pos Y", gate, 'y', gate.y)
            draw_field("Width", gate, 'width', gate.width)
            
            draw_section("Heights")
            draw_field("Z Closed", gate, 'z_closed', gate.z_closed)
            draw_field("Z Open", gate, 'z_open', gate.z_open)
            
            draw_section("Behavior")
            draw_field("Gate ID", gate, 'gate_id', gate.gate_id)
            draw_field("Speed", gate, 'speed', gate.speed)
            draw_field("Trigger Radius", gate, 'trigger_radius', gate.trigger_radius)
            
            draw_texture_selector("Gate Texture", gate.texture, self.textures, gate, 'texture')

        elif self.selected_switch is not None and self.selected_switch < len(self.switches):
            # Switch is selected - show its properties
            sw = self.switches[self.selected_switch]
            draw_header(f"SWITCH #{self.selected_switch}")
            
            draw_section("Transform")
            draw_field("Pos X", sw, 'x', sw.x)
            draw_field("Pos Y", sw, 'y', sw.y)
            draw_field("Pos Z", sw, 'z', sw.z)
            
            draw_section("Link")
            draw_field("Linked Gate ID", sw, 'linked_gate_id', sw.linked_gate_id)
            
            draw_texture_selector("Switch Texture", sw.texture, self.textures, sw, 'texture')

        elif self.current_tool == Tool.GATE:
            draw_header("TOOL: GATE")
            
            # Mode toggle (Gate vs Switch)
            draw_section("Mode")
            mode_y = cy
            mode_btn_w = (cw - 10) // 2
            
            gate_btn_rect = pygame.Rect(cx, mode_y, mode_btn_w, 26)
            switch_btn_rect = pygame.Rect(cx + mode_btn_w + 10, mode_y, mode_btn_w, 26)
            
            gate_color = Colors.ACCENT if self.gate_mode == "GATE" else (60, 60, 60)
            switch_color = Colors.ACCENT if self.gate_mode == "SWITCH" else (60, 60, 60)
            
            pygame.draw.rect(self.screen, gate_color, gate_btn_rect, border_radius=4)
            pygame.draw.rect(self.screen, switch_color, switch_btn_rect, border_radius=4)
            
            gt = self.font_small.render("GATE", True, (255, 255, 255))
            st = self.font_small.render("SWITCH", True, (255, 255, 255))
            self.screen.blit(gt, (gate_btn_rect.centerx - gt.get_width()//2, mode_y + 5))
            self.screen.blit(st, (switch_btn_rect.centerx - st.get_width()//2, mode_y + 5))
            
            def set_gate_mode():
                self.gate_mode = "GATE"
            def set_switch_mode():
                self.gate_mode = "SWITCH"
                
            self.ui_buttons.append({'rect': gate_btn_rect, 'action': set_gate_mode})
            self.ui_buttons.append({'rect': switch_btn_rect, 'action': set_switch_mode})
            cy = mode_y + 40
            
            if self.gate_mode == "GATE":
                draw_section("Gate Defaults")
                draw_field("Z Closed", self, 'prop_gate_z_closed', self.prop_gate_z_closed)
                draw_field("Z Open", self, 'prop_gate_z_open', self.prop_gate_z_open)
                draw_field("Speed", self, 'prop_gate_speed', self.prop_gate_speed)
                draw_field("Width", self, 'prop_gate_width', self.prop_gate_width)
                draw_field("Trigger Radius", self, 'prop_gate_trigger_radius', self.prop_gate_trigger_radius)
                draw_texture_selector("Gate Texture", self.prop_gate_texture, self.textures, self, 'prop_gate_texture')
            else:
                draw_section("Switch Defaults")
                draw_field("Linked Gate ID", self, 'prop_switch_linked_gate_id', self.prop_switch_linked_gate_id)
                draw_texture_selector("Switch Texture", self.prop_switch_texture, self.textures, self, 'prop_switch_texture')
            
            # Counts
            cy += 10
            draw_info("Gates", str(len(self.gates)))
            draw_info("Switches", str(len(self.switches)))

        else:

            draw_header("SCENE INFO")
            
            draw_section("Statistics")
            draw_info("Total Walls", str(len(self.walls)))
            draw_info("Total Sectors", str(len(self.sectors)))
            draw_info("Total Enemies", str(len(self.enemies)))
            draw_info("Total Pickups", str(len(self.pickups)))
            draw_info("Total Lights", str(len(self.lights)))
            
            draw_section("Editor")
            draw_info("Grid Size", str(self.active_viewport.grid_size) if self.active_viewport else "8")
            draw_info("FPS", str(int(self.clock.get_fps())))




    def draw_status_bar(self):
        """Draw bottom status bar"""
        status_height = 25
        status_y = WINDOW_HEIGHT - status_height
        
        pygame.draw.rect(self.screen, Colors.PANEL_BG, (0, status_y, WINDOW_WIDTH, status_height))
        pygame.draw.line(self.screen, Colors.SEPARATOR, (0, status_y), (WINDOW_WIDTH, status_y), 1)
        
        # Status text
        status_text = f"Tool: {self.current_tool.name}  |  "
        status_text += f"Sectors: {len(self.sectors)}  |  "
        status_text += f"Walls: {len(self.walls)}  |  "
        status_text += f"Enemies: {len(self.enemies)}  |  "

        
        if self.active_viewport:
            mouse_pos = pygame.mouse.get_pos()
            if self.active_viewport.rect.collidepoint(mouse_pos):
                wx, wy = self.active_viewport.screen_to_world(mouse_pos[0], mouse_pos[1])
                status_text += f"X: {int(wx)}  Y: {int(wy)}  |  "
        
        status_text += f"Grid: {'On' if self.show_grid else 'Off'}  |  "
        status_text += f"Snap: {'On' if self.snap_to_grid else 'Off'}  |  "
        
        # Show current file name
        file_name = os.path.basename(self.current_level_path)
        status_text += f"File: {file_name}"
        
        text_surface = self.font_small.render(status_text, True, Colors.TEXT_DIM)
        self.screen.blit(text_surface, (10, status_y + 6))

    def draw_text_center(self, text, x, y, color):
        """Draw centered text"""
        text_surface = self.font_medium.render(text, True, color)
        text_rect = text_surface.get_rect(center=(x, y))
        self.screen.blit(text_surface, text_rect)

# === MAIN ===
def main():
    editor = OracularEditor()
    editor.load_level()
    editor.run()

if __name__ == "__main__":
    main()
