#!/usr/bin/env python3
"""
Valve Hammer-Style Editor for DoomClone
Professional 4-way viewport map editor with all features
Inspired by Valve Hammer Editor
"""

import pygame
import sys
import math
import os
import re  # ADDED: for texture header parsing
from enum import Enum
from typing import List, Tuple, Optional
from tkinter import Tk, filedialog

# Initialize Pygame
pygame.init()

# === CONSTANTS ===
WINDOW_WIDTH = 1600
WINDOW_HEIGHT = 900
FPS = 60

# Default level file path
DEFAULT_LEVEL_PATH = r"D:\PROGRAM\VSPRO\DoomClone\level.h"

TEXTURE_DIR = "textures"  # ADDED: directory containing texture .h files

# Colors (Dark Theme - Hammer Style)
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

# Tool types
class Tool(Enum):
    SELECT = 0
    CREATE_SECTOR = 1
    VERTEX_EDIT = 2
    ENTITY = 3

# Viewport types
class ViewType(Enum):
    TOP = 0
    FRONT = 1
    SIDE = 2
    CAMERA_3D = 3

# === DATA STRUCTURES ===
class Wall:
    def __init__(self, x1=0, y1=0, x2=0, y2=0, wt=0, u=1, v=1, shade=0):
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2
        self.wt = wt  # wall texture index
        self.u = u  # u tile / scale
        self.v = v  # v tile / scale
        self.shade = shade

class Sector:
    def __init__(self, ws=0, we=0, z1=0, z2=40, st=0, ss=4):
        self.ws = ws  # wall start
        self.we = we  # wall end
        self.z1 = z1  # bottom height
        self.z2 = z2  # top height
        self.st = st  # surface texture index
        self.ss = ss  # surface scale

class Player:
    def __init__(self, x=0, y=0, z=20, a=0, l=0):
        self.x = x
        self.y = y
        self.z = z
        self.a = a  # angle
        self.l = l  # look up/down

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
class HammerEditor:
    def __init__(self):
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        pygame.display.set_caption("DoomClone Hammer Editor")
        self.clock = pygame.time.Clock()
        self.running = True
        
        # Fonts
        self.font_small = pygame.font.SysFont('Segoe UI', 12)
        self.font_medium = pygame.font.SysFont('Segoe UI', 14)
        self.font_large = pygame.font.SysFont('Segoe UI', 16, bold=True)
        self.font_title = pygame.font.SysFont('Segoe UI', 20, bold=True)
        
        self.current_level_path = DEFAULT_LEVEL_PATH
        
        self.current_tool = Tool.SELECT
        self.sectors = []
        self.walls = []
        self.player = Player()
        self.selected_sector = None
        self.selected_wall = None
        self.selected_vertices = []
        
        self.show_grid = True
        self.snap_to_grid = True
        
        self.active_menu = None
        self.menu_items = {
            "File": ["New (Ctrl+N)", "Open (Ctrl+O)", "Save (Ctrl+S)", "---", "Exit"],
            "Edit": ["Delete (Del)", "---", "Select All"],
            "View": ["Toggle Grid (G)", "Toggle Snap (Shift+S)"],
            "Tools": ["Select (1)", "Create Sector (2)", "Vertex Edit (3)", "Entity (4)"],
            "Help": ["About"]
        }
        
        self.creating_sector = False
        self.sector_vertices = []
        
        self.prop_wall_texture = 0
        self.prop_wall_u = 1
        self.prop_wall_v = 1
        self.prop_sector_z1 = 0
        self.prop_sector_z2 = 40
        self.prop_sector_texture = 0
        self.prop_sector_scale = 4
        
        # ADDED: textures
        self.textures: List[Texture] = []
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
        
        # ADDED: buttons for texture navigation
        self.texture_nav_buttons = []  # list of (rect, delta)

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
            
            # If specific variable names are provided, look for them
            # Otherwise, find all arrays
            
            # Regex to find array definitions: const char NAME[] = { ... };
            # We need to handle 'unsigned char' and 'char'
            # We also need to find corresponding WIDTH and HEIGHT defines
            
            # Strategy: Find all array definitions first
            # Matches: [static] [const] [unsigned] char NAME[] = { ... }
            array_matches = list(re.finditer(r'(?:static\s+)?(?:const\s+)?(?:unsigned\s+)?char\s+([a-zA-Z0-9_]+)(?:\[\])?\s*=\s*\{([^}]*)\}', data, re.DOTALL))
            
            for match in array_matches:
                name = match.group(1)
                content = match.group(2)
                
                if var_names and name not in var_names:
                    continue
                    
                # Find dimensions for this array
                # Look for #define NAME_WIDTH 64
                w_match = re.search(f'#define\s+{name}_WIDTH\s+(\d+)', data)
                h_match = re.search(f'#define\s+{name}_HEIGHT\s+(\d+)', data)
                
                # Fallback: try finding generic WIDTH/HEIGHT if not specific (e.g. WALL58_FRAME_WIDTH)
                if not w_match:
                    w_match = re.search(r'#define\s+[A-Z0-9_]+_WIDTH\s+(\d+)', data)
                if not h_match:
                    h_match = re.search(r'#define\s+[A-Z0-9_]+_HEIGHT\s+(\d+)', data)
                    
                if not (w_match and h_match):
                    print(f"Could not find dimensions for {name} in {filename}")
                    continue
                    
                width = int(w_match.group(1))
                height = int(h_match.group(1))
                
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
                
            # Sort surfaces if var_names provided to match order
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

    def setup_viewports(self):
        """Setup 4-way viewport layout"""
        # Calculate viewport sizes
        toolbar_width = 60
        properties_width = 280
        menu_height = 30
        status_height = 25
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
        if val is not None and target and attr:
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
        self.edit_field = None
        self.edit_buffer = ""

    def handle_keypress(self, event):
        # If editing a field, capture numeric input
        if self.edit_field:
            if event.key == pygame.K_RETURN:
                self.commit_property_edit(); return
            elif event.key == pygame.K_ESCAPE:
                self.edit_field = None; self.edit_buffer = ""; return
            elif event.key == pygame.K_BACKSPACE:
                self.edit_buffer = self.edit_buffer[:-1]; return
            else:
                if event.unicode.isdigit():
                    self.edit_buffer += event.unicode
                return
        if event.key == pygame.K_1:
            self.current_tool = Tool.SELECT
        elif event.key == pygame.K_2:
            self.current_tool = Tool.CREATE_SECTOR
        elif event.key == pygame.K_3:
            self.current_tool = Tool.VERTEX_EDIT
        elif event.key == pygame.K_4:
            self.current_tool = Tool.ENTITY
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
            self.delete_selected()
        elif event.key == pygame.K_ESCAPE:
            if self.creating_sector:
                self.cancel_sector_creation()

    def handle_mouse_down(self, event):
        if event.button == 1:
            self.mouse_down = True
            self.last_mouse_pos = event.pos
            # Property panel click first (to avoid viewport selection overriding edits)
            if self.handle_property_click(event.pos):
                return
            for vp in self.viewports:
                if vp.rect.collidepoint(event.pos):
                    self.active_viewport = vp
                    vp.is_active = True
                    if self.current_tool == Tool.SELECT:
                        self.handle_select_click(vp, event.pos)
                    elif self.current_tool == Tool.CREATE_SECTOR:
                        self.handle_create_sector_click(vp, event.pos)
                    elif self.current_tool == Tool.VERTEX_EDIT:
                        self.handle_vertex_edit_click(vp, event.pos)
                else:
                    vp.is_active = False
            self.check_ui_click(event.pos)
        elif event.button == 3:
            self.last_mouse_pos = event.pos

    # ADDED: property click handler
    def handle_property_click(self, pos):
        panel_width = 280
        panel_x = WINDOW_WIDTH - panel_width
        panel_y = 30
        panel_rect = pygame.Rect(panel_x, panel_y, panel_width, WINDOW_HEIGHT - 30 - 25)
        if not panel_rect.collidepoint(pos):
            return False
        # Texture nav buttons
        for rect, delta in self.texture_nav_buttons:
            if rect.collidepoint(pos) and self.selected_wall is not None:
                wall = self.walls[self.selected_wall]
                wall.wt = (wall.wt + delta) % max(1, len(self.textures))
                self.prop_wall_texture = wall.wt
                return True
        # Property items
        for item in self.property_items:
            if item['rect'].collidepoint(pos):
                self.edit_field = item
                current_val = getattr(item['target'], item['attr'])
                self.edit_buffer = str(current_val)
                return True
        return True  # Click inside panel but not on item still consumes

    def handle_mouse_up(self, event):
        if event.button == 1:
            self.mouse_down = False

    def handle_mouse_motion(self, event):
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

    def finish_sector_creation(self):
        if len(self.sector_vertices) < 3:
            print("Need at least 3 vertices to create a sector")
            self.creating_sector = False
            self.sector_vertices = []
            return
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
                        self.prop_wall_u, self.prop_wall_v, shade)
            self.walls.append(wall)
        wall_end = len(self.walls)
        sector = Sector(wall_start, wall_end, self.prop_sector_z1, self.prop_sector_z2,
                        self.prop_sector_texture, self.prop_sector_scale)
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
        if pos[1] < 30:
            menu_clicked = self.check_menu_click(pos)
            if menu_clicked:
                return
        self.active_menu = None
        toolbar_x = 10
        button_y = 40
        button_height = 50
        button_width = 50
        for i, tool in enumerate(Tool):
            button_rect = pygame.Rect(toolbar_x, button_y + i * (button_height + 5),
                                      button_width, button_height)
            if button_rect.collidepoint(pos):
                self.current_tool = tool
                return

    def check_menu_click(self, pos):
        menus = ["File", "Edit", "View", "Tools", "Help"]
        x = 10
        menu_height = 30
        for menu in menus:
            text = self.font_medium.render(menu, True, Colors.TEXT)
            menu_width = text.get_width() + 20
            if menu == self.active_menu or (x <= pos[0] <= x + menu_width and pos[1] < menu_height):
                if menu == self.active_menu:
                    self.active_menu = None
                else:
                    self.active_menu = menu
                return True
            x += menu_width
        if self.active_menu:
            dropdown_rect, selected_item = self.get_dropdown_rect_and_item(pos)
            if selected_item:
                self.execute_menu_action(self.active_menu, selected_item)
                self.active_menu = None
                return True
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
            if "Delete" in item:
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
            elif "Vertex Edit" in item:
                self.current_tool = Tool.VERTEX_EDIT
            elif "Entity" in item:
                self.current_tool = Tool.ENTITY
        elif menu == "Help":
            if "About" in item:
                print("DoomClone Hammer Editor v1.0")

    def open_level_dialog(self):
        """Open file dialog to select a level file"""
        # Create a hidden Tkinter root window
        root = Tk()
        root.withdraw()  # Hide the main window
        root.attributes('-topmost', True)  # Bring dialog to front
        
        # Get the directory of the current level path
        initial_dir = os.path.dirname(self.current_level_path) if os.path.exists(os.path.dirname(self.current_level_path)) else os.getcwd()
        
        # Open file dialog
        filename = filedialog.askopenfilename(
            title="Open Level File",
            initialdir=initial_dir,
            initialfile="level.h",
            filetypes=[
                ("Level files", "*.h"),
                ("All files", "*.*")
            ]
        )
        
        # Destroy the Tkinter root
        root.destroy()
        
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
    
    def new_level(self):
        """Create new level"""
        self.sectors = []
        self.walls = []
        self.player = Player(x=32*9, y=48, z=30, a=0, l=0)
        self.selected_sector = None
        self.selected_wall = None
        self.selected_vertices = []
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
                    f.write(f"{s.ws} {s.we} {s.z1} {s.z2} {s.st} {s.ss}\n")
                
                # Write walls
                f.write(f"{len(self.walls)}\n")
                for w in self.walls:
                    f.write(f"{w.x1} {w.y1} {w.x2} {w.y2} {w.wt} {w.u} {w.v} {w.shade}\n")
                
                # Write player
                f.write(f"\n{self.player.x} {self.player.y} {self.player.z} {self.player.a} {self.player.l}\n")
            
            print(f"Level saved to {self.current_level_path}")
        except Exception as e:
            print(f"Error saving level: {e}")
    
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
            
            # Center viewports on the loaded geometry
            self.center_viewports_on_level()
            
            print(f"Loaded {len(self.sectors)} sectors, {len(self.walls)} walls from {self.current_level_path}")
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
            
            if self.selected_wall is not None:
                wall = self.walls[self.selected_wall]
                self.prop_wall_texture = wall.wt
                self.prop_wall_u = wall.u
                self.prop_wall_v = wall.v
    
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
        
        pygame.display.flip()
    
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
        toolbar_width = 60
        toolbar_height = WINDOW_HEIGHT - 30 - 25
        toolbar_rect = pygame.Rect(0, 30, toolbar_width, toolbar_height)
        
        pygame.draw.rect(self.screen, Colors.PANEL_BG, toolbar_rect)
        pygame.draw.line(self.screen, Colors.SEPARATOR, (toolbar_width, 30),
                        (toolbar_width, WINDOW_HEIGHT - 25), 1)
        
        # Tool buttons
        tool_icons = ["Sel", "Sec", "Vtx", "Ent"]
        button_y = 40
        button_height = 50
        button_width = 50
        
        for i, (tool, icon_text) in enumerate(zip(Tool, tool_icons)):
            button_rect = pygame.Rect(5, button_y, button_width, button_height)
            
            # Button color based on state
            if self.current_tool == tool:
                color = Colors.BUTTON_ACTIVE
            elif button_rect.collidepoint(pygame.mouse.get_pos()):
                color = Colors.BUTTON_HOVER
            else:
                color = Colors.BUTTON
            
            pygame.draw.rect(self.screen, color, button_rect, border_radius=4)
            
            # Icon text
            text = self.font_small.render(icon_text, True, Colors.TEXT)
            text_rect = text.get_rect(center=button_rect.center)
            self.screen.blit(text, text_rect)
            
            button_y += button_height + 5
    
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
                self.draw_wall_2d(vp, wall, i == self.selected_wall, i == self.hover_wall)
            
            # Draw vertices
            for i, wall in enumerate(self.walls):
                for vertex_idx, (vx, vy) in enumerate([(wall.x1, wall.y1), (wall.x2, wall.y2)]):
                    self.draw_vertex_2d(vp, vx, vy, (i, vertex_idx) in self.selected_vertices, self.hover_vertex == (i, vertex_idx))
            
            # Draw sector creation preview
            if self.creating_sector and len(self.sector_vertices) > 0:
                self.draw_sector_creation_preview(vp)
            
            # Draw player
            self.draw_player_2d(vp)
        
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
    
    def draw_wall_2d(self, vp, wall, is_selected, is_hovered):
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
        
        # Wall color based on state
        if is_selected:
            color = Colors.WALL_SELECTED
            width = 2
        elif is_hovered:
            color = Colors.SELECT_HOVER
            width = 2
        else:
            color = Colors.WALL
            width = 1
        
        pygame.draw.line(self.screen, color, (x1, y1), (x2, y2), width)
    
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
        
        # Project and draw each wall
        for sector in self.sectors:
            for wall_idx in range(sector.ws, sector.we):
                if wall_idx >= len(self.walls):
                    continue
                
                wall = self.walls[wall_idx]
                
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
                color = Colors.WALL_SELECTED if wall_idx == self.selected_wall else Colors.WALL
                
                # Draw vertical edges
                pygame.draw.line(self.screen, color, (sx1, sy1_bottom), (sx1, sy1_top), 1)
                pygame.draw.line(self.screen, color, (sx2, sy2_bottom), (sx2, sy2_top), 1)
                
                # Draw horizontal edges
                pygame.draw.line(self.screen, color, (sx1, sy1_bottom), (sx2, sy2_bottom), 1)
                pygame.draw.line(self.screen, color, (sx1, sy1_top), (sx2, sy2_top), 1)
    
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
    
    def draw_properties_panel(self):
        """Draw right properties panel"""
        panel_width = 280
        panel_height = WINDOW_HEIGHT - 30 - 25
        panel_x = WINDOW_WIDTH - panel_width
        panel_y = 30
        
        panel_rect = pygame.Rect(panel_x, panel_y, panel_width, panel_height)
        pygame.draw.rect(self.screen, Colors.PANEL_BG, panel_rect)
        pygame.draw.line(self.screen, Colors.SEPARATOR, (panel_x, 30),
                        (panel_x, WINDOW_HEIGHT - 25), 1)
        
        # Panel title
        title = self.font_large.render("Properties", True, Colors.TEXT)
        self.screen.blit(title, (panel_x + 10, panel_y + 10))
        
        y = panel_y + 45
        
        # Reset property items each frame
        self.property_items = []
        self.texture_nav_buttons = []
        
        def add_editable(label_text, target, attr, value, x, y):
            label_surface = self.font_small.render(f"{label_text}: {value}", True, Colors.TEXT_DIM)
            rect = pygame.Rect(x, y, panel_width - 20, 20)
            # Highlight if editing
            if self.edit_field and self.edit_field.get('attr') == attr and self.edit_field.get('target') is target:
                pygame.draw.rect(self.screen, Colors.EDIT_BG, rect)
                pygame.draw.rect(self.screen, Colors.EDIT_BORDER, rect, 1)
                edit_text = self.font_small.render(f"{label_text}: {self.edit_buffer}", True, Colors.ACCENT_BRIGHT)
                self.screen.blit(edit_text, (x + 5, y + 2))
            else:
                self.screen.blit(label_surface, (x + 5, y + 2))
            self.property_items.append({'rect': rect, 'target': target, 'attr': attr})
        
        if self.selected_sector is not None:
            sector = self.sectors[self.selected_sector]
            self.draw_property_label("Sector", panel_x + 10, y); y += 25
            add_editable("ID", sector, 'ws', self.selected_sector, panel_x + 10, y); y += 22  # ID not really editable, but keep uniform
            walls_count = sector.we - sector.ws
            self.draw_property_value(f"Walls: {walls_count}", panel_x + 10, y); y += 28
            self.draw_property_label("Heights", panel_x + 10, y); y += 25
            add_editable("Floor (z1)", sector, 'z1', sector.z1, panel_x + 10, y); y += 22
            add_editable("Ceiling (z2)", sector, 'z2', sector.z2, panel_x + 10, y); y += 22
            self.draw_property_label("Textures", panel_x + 10, y); y += 25
            add_editable("Surface Tex (st)", sector, 'st', sector.st, panel_x + 10, y); y += 22
            add_editable("Surface Scale (ss)", sector, 'ss', sector.ss, panel_x + 10, y); y += 30
            if self.selected_wall is not None:
                wall = self.walls[self.selected_wall]
                self.draw_property_label("Wall", panel_x + 10, y); y += 25
                add_editable("Wall ID", wall, 'x1', self.selected_wall, panel_x + 10, y); y += 22  # Not editable logically
                add_editable("Texture (wt)", wall, 'wt', wall.wt, panel_x + 10, y); y += 22
                add_editable("U (u)", wall, 'u', wall.u, panel_x + 10, y); y += 22
                add_editable("V (v)", wall, 'v', wall.v, panel_x + 10, y); y += 22
                self.draw_property_value(f"Shade: {wall.shade}", panel_x + 10, y); y += 30
                # Texture preview
                if self.textures:
                    tex_index = wall.wt % len(self.textures)
                    tex = self.textures[tex_index]
                    preview_size = 128
                    preview_rect = pygame.Rect(panel_x + (panel_width - preview_size)//2, y, preview_size, preview_size)
                    pygame.draw.rect(self.screen, Colors.BG_LIGHT, preview_rect)
                    # Scale texture surface
                    current_frame = tex.get_current_frame()
                    scaled = pygame.transform.smoothscale(current_frame, (preview_size-4, preview_size-4))
                    self.screen.blit(scaled, (preview_rect.x + 2, preview_rect.y + 2))
                    name_text = self.font_small.render(tex.name, True, Colors.TEXT_DIM)
                    self.screen.blit(name_text, (preview_rect.x + 4, preview_rect.y + 4))
                    y += preview_size + 8
                    # Prev / Next buttons
                    btn_w = 60; btn_h = 24
                    prev_rect = pygame.Rect(panel_x + 30, y, btn_w, btn_h)
                    next_rect = pygame.Rect(panel_x + panel_width - 30 - btn_w, y, btn_w, btn_h)
                    for rect, label, delta in [(prev_rect, "Prev", -1), (next_rect, "Next", 1)]:
                        color = Colors.BUTTON_ACTIVE if rect.collidepoint(pygame.mouse.get_pos()) else Colors.BUTTON
                        pygame.draw.rect(self.screen, color, rect, border_radius=4)
                        t_surf = self.font_small.render(label, True, Colors.TEXT)
                        t_rect = t_surf.get_rect(center=rect.center)
                        self.screen.blit(t_surf, t_rect)
                        self.texture_nav_buttons.append((rect, delta))
                    y += btn_h + 10
        else:
            self.draw_property_label("Current Settings", panel_x + 10, y); y += 25
            self.draw_property_value(f"Tool: {self.current_tool.name}", panel_x + 10, y); y += 20
            self.draw_property_value(f"Grid: {'On' if self.show_grid else 'Off'}", panel_x + 10, y); y += 20
            self.draw_property_value(f"Snap: {'On' if self.snap_to_grid else 'Off'}", panel_x + 10, y); y += 30
            self.draw_property_label("Default Properties", panel_x + 10, y); y += 25
            self.draw_property_value(f"Wall Texture: {self.prop_wall_texture}", panel_x + 10, y); y += 20
            self.draw_property_value(f"Floor Height: {self.prop_sector_z1}", panel_x + 10, y); y += 20
            self.draw_property_value(f"Ceiling Height: {self.prop_sector_z2}", panel_x + 10, y)

    def draw_property_label(self, text, x, y):
        label = self.font_medium.render(text, True, Colors.ACCENT_BRIGHT)
        self.screen.blit(label, (x, y))

    def draw_property_value(self, text, x, y):
        value = self.font_small.render(text, True, Colors.TEXT_DIM)
        self.screen.blit(value, (x + 5, y))

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
    editor = HammerEditor()
    editor.load_level()
    editor.run()

if __name__ == "__main__":
    main()
