#!/usr/bin/env python3
"""
Modern Texture Editor for DoomClone
Beautiful Tkinter UI with all features
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, colorchooser
from tkinter import font as tkfont
from tkinter import simpledialog
from PIL import Image, ImageDraw, ImageTk
import os
import struct

class ModernTextureEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("DoomClone Texture Editor")
        self.root.geometry("1400x900")
        self.root.configure(bg='#1e1e1e')
        
        # Set modern theme colors
        self.colors = {
            'bg_dark': '#1e1e1e',
            'bg_medium': '#2d2d30',
            'bg_light': '#3e3e42',
            'accent': '#007acc',
            'accent_hover': '#1e8ad6',
            'text': '#cccccc',
            'text_bright': '#ffffff',
            'success': '#4ec9b0',
            'warning': '#ce9178',
            'error': '#f48771'
        }
        
        # Data
        self.frames = []
        self.current_frame = 0
        self.current_color = (255, 255, 255)
        self.brush_size = 5
        self.zoom = 8
        self.tool = "pencil"  # pencil, eraser, fill, picker, line, rect, circle
        self.grid_enabled = True
        self.is_playing = False
        self.canvas_offset_x = 0
        self.canvas_offset_y = 0
        
        # Drawing state
        self.is_drawing = False
        self.last_x = None
        self.last_y = None
        self.start_x = None
        self.start_y = None
        
        # Palette
        self.palette = [
            (255, 255, 255), (0, 0, 0), (255, 0, 0), (0, 255, 0),
            (0, 0, 255), (255, 255, 0), (255, 0, 255), (0, 255, 255),
            (128, 128, 128), (192, 192, 192), (128, 0, 0), (0, 128, 0),
            (0, 0, 128), (128, 128, 0), (128, 0, 128), (0, 128, 128),
            (64, 64, 64), (255, 128, 0), (255, 128, 128), (128, 255, 128),
            (128, 128, 255), (200, 200, 200), (100, 100, 100), (150, 75, 0),
            (75, 0, 130), (255, 165, 0), (255, 192, 203), (165, 42, 42),
            (210, 105, 30), (139, 69, 19), (85, 107, 47), (47, 79, 79)
        ]
        
        self.setup_ui()
        self.new_texture(64, 64)
        
    def setup_ui(self):
        """Create the modern, innovative UI"""
        
        # === TOP BAR (Menu & Quick Actions) ===
        top_bar = tk.Frame(self.root, bg=self.colors['bg_dark'], height=60)
        top_bar.pack(side=tk.TOP, fill=tk.X)
        top_bar.pack_propagate(False)
        
        # Logo/Title
        title_frame = tk.Frame(top_bar, bg=self.colors['bg_dark'])
        title_frame.pack(side=tk.LEFT, padx=20)
        
        tk.Label(title_frame, text="DoomClone Texture Editor", 
                font=('Segoe UI', 16, 'bold'), bg=self.colors['bg_dark'], 
                fg=self.colors['text_bright']).pack(side=tk.LEFT, padx=10)
        
        # Quick action buttons
        btn_frame = tk.Frame(top_bar, bg=self.colors['bg_dark'])
        btn_frame.pack(side=tk.RIGHT, padx=20)
        
        self.create_top_button(btn_frame, "Open", self.load_image, "Load PNG/JPG/BMP")
        self.create_top_button(btn_frame, "Save", self.save_texture, "Save project")
        self.create_top_button(btn_frame, "Export", self.export_c_header, "Export to C header")
        self.create_top_button(btn_frame, "New", lambda: self.new_texture(64, 64), "New texture")
        
        # === MAIN CONTAINER ===
        main_container = tk.Frame(self.root, bg=self.colors['bg_dark'])
        main_container.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # === LEFT PANEL (Tools & Palette) ===
        left_panel = tk.Frame(main_container, bg=self.colors['bg_medium'], width=250)
        left_panel.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 5))
        left_panel.pack_propagate(False)
        
        self.setup_tools_panel(left_panel)
        
        # === CENTER (Canvas) ===
        center_panel = tk.Frame(main_container, bg=self.colors['bg_medium'])
        center_panel.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))
        
        self.setup_canvas_panel(center_panel)
        
        # === RIGHT PANEL (Properties & Animation) ===
        right_panel = tk.Frame(main_container, bg=self.colors['bg_medium'], width=280)
        right_panel.pack(side=tk.RIGHT, fill=tk.Y)
        right_panel.pack_propagate(False)
        
        self.setup_properties_panel(right_panel)
        
        # === BOTTOM BAR (Timeline) ===
        bottom_bar = tk.Frame(self.root, bg=self.colors['bg_medium'], height=120)
        bottom_bar.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=(0, 5))
        bottom_bar.pack_propagate(False)
        
        self.setup_timeline(bottom_bar)
        
    def create_top_button(self, parent, text, command, tooltip):
        """Create a modern flat button"""
        btn = tk.Button(parent, text=text, command=command,
                       bg=self.colors['accent'], fg=self.colors['text_bright'],
                       font=('Segoe UI', 10, 'bold'), relief=tk.FLAT,
                       padx=15, pady=8, cursor='hand2', bd=0)
        btn.pack(side=tk.LEFT, padx=5)
        
        # Hover effect
        btn.bind('<Enter>', lambda e: btn.config(bg=self.colors['accent_hover']))
        btn.bind('<Leave>', lambda e: btn.config(bg=self.colors['accent']))
        
        return btn
        
    def setup_tools_panel(self, parent):
        """Setup tools and palette panel"""
        
        # Tools section
        tools_label = tk.Label(parent, text="Tools", font=('Segoe UI', 12, 'bold'),
                              bg=self.colors['bg_medium'], fg=self.colors['text_bright'])
        tools_label.pack(pady=10, padx=10, anchor=tk.W)
        
        tools_frame = tk.Frame(parent, bg=self.colors['bg_medium'])
        tools_frame.pack(fill=tk.X, padx=10)
        
        tools = [
            ("Pencil", "pencil"),
            ("Eraser", "eraser"),
            ("Fill", "fill"),
            ("Picker", "picker"),
            ("Line", "line"),
            ("Rectangle", "rect"),
            ("Circle", "circle")
        ]
        
        self.tool_buttons = {}
        for text, tool in tools:
            btn = tk.Button(tools_frame, text=text, command=lambda t=tool: self.set_tool(t),
                          bg=self.colors['bg_light'], fg=self.colors['text'],
                          font=('Segoe UI', 9), relief=tk.FLAT, anchor=tk.W,
                          padx=10, pady=8, cursor='hand2')
            btn.pack(fill=tk.X, pady=2)
            self.tool_buttons[tool] = btn
        
        self.set_tool("pencil")
        
        # Brush size
        tk.Label(parent, text="Brush Size", font=('Segoe UI', 10, 'bold'),
                bg=self.colors['bg_medium'], fg=self.colors['text']).pack(pady=(20, 5), padx=10, anchor=tk.W)
        
        brush_frame = tk.Frame(parent, bg=self.colors['bg_medium'])
        brush_frame.pack(fill=tk.X, padx=10)
        
        self.brush_slider = tk.Scale(brush_frame, from_=1, to=30, orient=tk.HORIZONTAL,
                                     bg=self.colors['bg_medium'], fg=self.colors['text'],
                                     highlightthickness=0, troughcolor=self.colors['bg_light'],
                                     activebackground=self.colors['accent'])
        self.brush_slider.set(self.brush_size)
        self.brush_slider.pack(fill=tk.X, side=tk.LEFT, expand=True)
        
        self.brush_label = tk.Label(brush_frame, text=f"{self.brush_size}px",
                                    bg=self.colors['bg_medium'], fg=self.colors['text'],
                                    font=('Segoe UI', 9), width=4)
        self.brush_label.pack(side=tk.RIGHT, padx=5)
        
        self.brush_slider.config(command=lambda v: self.brush_label.config(text=f"{int(float(v))}px"))
        
        # Color picker
        tk.Label(parent, text="Current Color", font=('Segoe UI', 10, 'bold'),
                bg=self.colors['bg_medium'], fg=self.colors['text']).pack(pady=(20, 5), padx=10, anchor=tk.W)
        
        color_frame = tk.Frame(parent, bg=self.colors['bg_medium'])
        color_frame.pack(fill=tk.X, padx=10)
        
        self.color_display = tk.Canvas(color_frame, width=100, height=40,
                                       bg='#ffffff', highlightthickness=2,
                                       highlightbackground=self.colors['accent'])
        self.color_display.pack(side=tk.LEFT, padx=(0, 10))
        
        tk.Button(color_frame, text="Pick Color", command=self.pick_color,
                 bg=self.colors['accent'], fg=self.colors['text_bright'],
                 font=('Segoe UI', 9), relief=tk.FLAT, padx=10, pady=5,
                 cursor='hand2').pack(side=tk.LEFT)
        
        # Palette
        tk.Label(parent, text="Palette", font=('Segoe UI', 10, 'bold'),
                bg=self.colors['bg_medium'], fg=self.colors['text']).pack(pady=(20, 5), padx=10, anchor=tk.W)
        
        palette_frame = tk.Frame(parent, bg=self.colors['bg_medium'])
        palette_frame.pack(fill=tk.BOTH, padx=10, pady=5, expand=True)
        
        # Create palette grid (8x4)
        for i, color in enumerate(self.palette):
            row = i // 8
            col = i % 8
            
            color_hex = '#%02x%02x%02x' % color
            btn = tk.Button(palette_frame, bg=color_hex, width=2, height=1,
                           relief=tk.RAISED, bd=1,
                           command=lambda c=color: self.select_palette_color(c))
            btn.grid(row=row, column=col, padx=1, pady=1, sticky='nsew')
            
        for i in range(8):
            palette_frame.grid_columnconfigure(i, weight=1)
        
    def setup_canvas_panel(self, parent):
        """Setup the drawing canvas"""
        
        # Canvas header
        canvas_header = tk.Frame(parent, bg=self.colors['bg_medium'], height=50)
        canvas_header.pack(side=tk.TOP, fill=tk.X)
        canvas_header.pack_propagate(False)
        
        tk.Label(canvas_header, text="Canvas", font=('Segoe UI', 12, 'bold'),
                bg=self.colors['bg_medium'], fg=self.colors['text_bright']).pack(side=tk.LEFT, padx=15, pady=10)
        
        # Zoom controls
        zoom_frame = tk.Frame(canvas_header, bg=self.colors['bg_medium'])
        zoom_frame.pack(side=tk.RIGHT, padx=15)
        
        tk.Button(zoom_frame, text="-", command=self.zoom_out,
                 bg=self.colors['bg_light'], fg=self.colors['text'],
                 font=('Segoe UI', 12, 'bold'), relief=tk.FLAT, width=3,
                 cursor='hand2').pack(side=tk.LEFT, padx=2)
        
        self.zoom_label = tk.Label(zoom_frame, text=f"{self.zoom}x",
                                   bg=self.colors['bg_medium'], fg=self.colors['text'],
                                   font=('Segoe UI', 10), width=4)
        self.zoom_label.pack(side=tk.LEFT, padx=5)
        
        tk.Button(zoom_frame, text="+", command=self.zoom_in,
                 bg=self.colors['bg_light'], fg=self.colors['text'],
                 font=('Segoe UI', 12, 'bold'), relief=tk.FLAT, width=3,
                 cursor='hand2').pack(side=tk.LEFT, padx=2)
        
        # Grid toggle
        self.grid_var = tk.BooleanVar(value=True)
        tk.Checkbutton(canvas_header, text="Grid", variable=self.grid_var,
                      command=self.toggle_grid, bg=self.colors['bg_medium'],
                      fg=self.colors['text'], selectcolor=self.colors['bg_light'],
                      font=('Segoe UI', 9), cursor='hand2').pack(side=tk.RIGHT, padx=10)
        
        # Canvas container with scrollbars
        canvas_container = tk.Frame(parent, bg=self.colors['bg_light'])
        canvas_container.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        # Create canvas
        self.canvas = tk.Canvas(canvas_container, bg='#2d2d30',
                               highlightthickness=0, cursor='crosshair')
        self.canvas.pack(fill=tk.BOTH, expand=True)
        
        # Bind mouse events
        self.canvas.bind('<Button-1>', self.on_canvas_click)
        self.canvas.bind('<B1-Motion>', self.on_canvas_drag)
        self.canvas.bind('<ButtonRelease-1>', self.on_canvas_release)
        
    def setup_properties_panel(self, parent):
        """Setup properties and animation controls"""
        
        # Frame info
        info_frame = tk.Frame(parent, bg=self.colors['bg_medium'])
        info_frame.pack(fill=tk.X, padx=10, pady=10)
        
        tk.Label(info_frame, text="Properties", font=('Segoe UI', 12, 'bold'),
                bg=self.colors['bg_medium'], fg=self.colors['text_bright']).pack(anchor=tk.W, pady=(0, 10))
        
        self.info_label = tk.Label(info_frame, text="Size: 64x64\nFrame: 1/1",
                                   bg=self.colors['bg_light'], fg=self.colors['text'],
                                   font=('Segoe UI', 9), justify=tk.LEFT, padx=10, pady=10)
        self.info_label.pack(fill=tk.X)
        
        # Animation controls
        anim_frame = tk.Frame(parent, bg=self.colors['bg_medium'])
        anim_frame.pack(fill=tk.X, padx=10, pady=(20, 0))
        
        tk.Label(anim_frame, text="Animation", font=('Segoe UI', 12, 'bold'),
                bg=self.colors['bg_medium'], fg=self.colors['text_bright']).pack(anchor=tk.W, pady=(0, 10))
        
        btn_grid = tk.Frame(anim_frame, bg=self.colors['bg_medium'])
        btn_grid.pack(fill=tk.X)
        
        self.create_property_button(btn_grid, "Add Frame", self.add_frame).grid(row=0, column=0, sticky='ew', padx=2, pady=2)
        self.create_property_button(btn_grid, "Delete", self.delete_frame).grid(row=0, column=1, sticky='ew', padx=2, pady=2)
        self.create_property_button(btn_grid, "Duplicate", self.duplicate_frame).grid(row=1, column=0, sticky='ew', padx=2, pady=2)
        self.create_property_button(btn_grid, "Play", self.toggle_play).grid(row=1, column=1, sticky='ew', padx=2, pady=2)
        
        btn_grid.grid_columnconfigure(0, weight=1)
        btn_grid.grid_columnconfigure(1, weight=1)
        
        # Frame duration
        duration_frame = tk.Frame(parent, bg=self.colors['bg_medium'])
        duration_frame.pack(fill=tk.X, padx=10, pady=(20, 0))
        
        tk.Label(duration_frame, text="Frame Duration (ms)", font=('Segoe UI', 9, 'bold'),
                bg=self.colors['bg_medium'], fg=self.colors['text']).pack(anchor=tk.W, pady=(0, 5))
        
        self.duration_entry = tk.Entry(duration_frame, bg=self.colors['bg_light'],
                                       fg=self.colors['text'], font=('Segoe UI', 9),
                                       relief=tk.FLAT, insertbackground=self.colors['text'])
        self.duration_entry.pack(fill=tk.X)
        self.duration_entry.insert(0, "100")
        
        # Clear canvas button
        tk.Button(parent, text="Clear Canvas", command=self.clear_canvas,
                 bg=self.colors['error'], fg=self.colors['text_bright'],
                 font=('Segoe UI', 10, 'bold'), relief=tk.FLAT,
                 padx=10, pady=10, cursor='hand2').pack(side=tk.BOTTOM, fill=tk.X, padx=10, pady=10)
        
    def create_property_button(self, parent, text, command):
        """Create a property panel button"""
        return tk.Button(parent, text=text, command=command,
                        bg=self.colors['bg_light'], fg=self.colors['text'],
                        font=('Segoe UI', 9), relief=tk.FLAT,
                        padx=10, pady=8, cursor='hand2')
        
    def setup_timeline(self, parent):
        """Setup the timeline"""
        
        # Timeline header
        header = tk.Frame(parent, bg=self.colors['bg_medium'], height=30)
        header.pack(side=tk.TOP, fill=tk.X)
        header.pack_propagate(False)
        
        tk.Label(header, text="Timeline", font=('Segoe UI', 10, 'bold'),
                bg=self.colors['bg_medium'], fg=self.colors['text_bright']).pack(side=tk.LEFT, padx=10)
        
        # Timeline canvas (clickable frames)
        self.timeline_canvas = tk.Canvas(parent, bg=self.colors['bg_dark'],
                                         highlightthickness=0, height=80)
        self.timeline_canvas.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        self.timeline_canvas.bind('<Button-1>', self.on_timeline_click)
        
    def set_tool(self, tool):
        """Set the current drawing tool"""
        self.tool = tool
        
        # Update button colors
        for t, btn in self.tool_buttons.items():
            if t == tool:
                btn.config(bg=self.colors['accent'], fg=self.colors['text_bright'])
            else:
                btn.config(bg=self.colors['bg_light'], fg=self.colors['text'])
                
    def pick_color(self):
        """Open color picker"""
        color = colorchooser.askcolor(self.current_color)
        if color[0]:
            self.current_color = tuple(int(c) for c in color[0])
            color_hex = '#%02x%02x%02x' % self.current_color
            self.color_display.config(bg=color_hex)
            
    def select_palette_color(self, color):
        """Select a color from the palette"""
        self.current_color = color
        color_hex = '#%02x%02x%02x' % color
        self.color_display.config(bg=color_hex)
        
    def zoom_in(self):
        """Zoom in"""
        if self.zoom < 32:
            self.zoom += 1
            self.zoom_label.config(text=f"{self.zoom}x")
            self.draw_canvas()
            
    def zoom_out(self):
        """Zoom out"""
        if self.zoom > 1:
            self.zoom -= 1
            self.zoom_label.config(text=f"{self.zoom}x")
            self.draw_canvas()
            
    def toggle_grid(self):
        """Toggle grid display"""
        self.grid_enabled = self.grid_var.get()
        self.draw_canvas()
        
    def new_texture(self, width, height):
        """Create a new texture"""
        img = Image.new("RGB", (width, height), (0, 0, 0))
        self.frames = [{'image': img, 'duration': 100}]
        self.current_frame = 0
        self.update_info()
        self.draw_canvas()
        self.draw_timeline()
        
    def load_image(self):
        """Load image files - supports multiple selection for animation"""
        filenames = filedialog.askopenfilenames(
            title="Select Images (Multiple for animation)",
            filetypes=[
                ("Image Files", "*.png *.jpg *.jpeg *.bmp"),
                ("PNG Files", "*.png"),
                ("JPEG Files", "*.jpg *.jpeg"),
                ("BMP Files", "*.bmp"),
                ("All Files", "*.*")
            ]
        )
        if filenames:
            try:
                self.frames = []
                for filename in filenames:
                    img = Image.open(filename).convert("RGB")
                    self.frames.append({'image': img, 'duration': 100})
                
                self.current_frame = 0
                self.update_info()
                self.draw_canvas()
                self.draw_timeline()
                
                if len(filenames) == 1:
                    messagebox.showinfo("Success", f"Loaded: {os.path.basename(filenames[0])}")
                else:
                    messagebox.showinfo("Success", f"Loaded {len(filenames)} frames as animation!")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load image:\n{e}")
                
    def save_texture(self):
        """Save texture project or export frames"""
        if len(self.frames) > 1:
            # Ask user if they want to save as project or export frames
            choice = messagebox.askyesnocancel(
                "Save Options",
                "Save as project file?\n\nYes - Save .dat project\nNo - Export frames as images\nCancel - Cancel"
            )
            if choice is None:  # Cancel
                return
            elif choice:  # Yes - save as .dat
                self.save_project_file()
            else:  # No - export frames
                self.export_frames()
        else:
            # Single frame, just save as project
            self.save_project_file()
    
    def save_project_file(self):
        """Save texture project as .dat file"""
        filename = filedialog.asksaveasfilename(
            title="Save Texture Project",
            defaultextension=".dat",
            filetypes=[("Texture Data", "*.dat"), ("All Files", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'wb') as f:
                    # Write frame count
                    f.write(struct.pack('i', len(self.frames)))
                    
                    for frame_data in self.frames:
                        img = frame_data['image']
                        duration = frame_data['duration']
                        
                        # Write dimensions and duration
                        f.write(struct.pack('iii', img.width, img.height, duration))
                        
                        # Write RGB data
                        f.write(img.tobytes())
                        
                messagebox.showinfo("Success", "Texture saved!")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save:\n{e}")
    
    def export_frames(self):
        """Export all frames as separate image files"""
        # Ask for directory
        directory = filedialog.askdirectory(title="Select folder to save frames")
        if not directory:
            return
        
        # Ask for base filename
        base_name = tk.simpledialog.askstring(
            "Frame Export",
            "Enter base filename for frames:",
            initialvalue="frame"
        )
        if not base_name:
            return
        
        # Ask for format
        format_choice = messagebox.askquestion(
            "Image Format",
            "Save as PNG?\n\nYes - PNG (lossless)\nNo - BMP (lossless, larger)"
        )
        ext = ".png" if format_choice == "yes" else ".bmp"
        
        try:
            for i, frame_data in enumerate(self.frames):
                img = frame_data['image']
                filename = os.path.join(directory, f"{base_name}_{i+1:03d}{ext}")
                img.save(filename)
            
            messagebox.showinfo(
                "Success", 
                f"Exported {len(self.frames)} frames to:\n{directory}"
            )
        except Exception as e:
            messagebox.showerror("Error", f"Failed to export frames:\n{e}")
        
    def export_c_header(self):
        """Export to C header file(s)"""
        if len(self.frames) > 1:
            # Multiple frames - ask if separate files or single animated file
            choice = messagebox.askyesnocancel(
                "Export Options",
                "Export as separate .h files?\n\nYes - Each frame as separate .h file\nNo - Single animated .h file\nCancel - Cancel"
            )
            if choice is None:  # Cancel
                return
            elif choice:  # Yes - separate files
                self.export_frames_as_headers()
            else:  # No - single animated file
                self.export_single_animated_header()
        else:
            # Single frame
            self.export_single_animated_header()
    
    def export_frames_as_headers(self):
        """Export each frame as a separate .h file"""
        # Ask for directory
        directory = filedialog.askdirectory(title="Select folder for .h files")
        if not directory:
            return
        
        # Ask for base filename
        base_name = tk.simpledialog.askstring(
            "Header Export",
            "Enter base name for header files:",
            initialvalue="frame"
        )
        if not base_name:
            return
        
        base_name = base_name.upper()
        
        try:
            for i, frame_data in enumerate(self.frames):
                img = frame_data['image']
                filename = os.path.join(directory, f"{base_name.lower()}_{i+1:03d}.h")
                name = f"{base_name}_{i+1:03d}"
                
                with open(filename, 'w') as f:
                    f.write(f"#ifndef {name}_H\n")
                    f.write(f"#define {name}_H\n\n")
                    f.write(f"#define {name}_WIDTH {img.width}\n")
                    f.write(f"#define {name}_HEIGHT {img.height}\n\n")
                    f.write(f"static const unsigned char {name.lower()}[] = {{\n")
                    self.write_image_data(f, img)
                    f.write("};\n\n")
                    f.write(f"#endif // {name}_H\n")
            
            messagebox.showinfo(
                "Success",
                f"Exported {len(self.frames)} header files to:\n{directory}"
            )
        except Exception as e:
            messagebox.showerror("Error", f"Failed to export headers:\n{e}")
    
    def export_single_animated_header(self):
        """Export as single animated .h file"""
        filename = filedialog.asksaveasfilename(
            title="Export C Header",
            defaultextension=".h",
            filetypes=[("C Header", "*.h"), ("All Files", "*.*")]
        )
        if not filename:
            return
            
        name = os.path.splitext(os.path.basename(filename))[0].upper()
        
        with open(filename, 'w') as f:
            f.write(f"#ifndef {name}_H\n")
            f.write(f"#define {name}_H\n\n")
            
            if len(self.frames) > 1:
                # Animated
                img = self.frames[0]['image']
                f.write(f"#define {name}_FRAME_COUNT {len(self.frames)}\n")
                f.write(f"#define {name}_FRAME_WIDTH {img.width}\n")
                f.write(f"#define {name}_FRAME_HEIGHT {img.height}\n")
                f.write(f"#define {name}_ANIM_AVAILABLE 1\n\n")
                
                for i, frame_data in enumerate(self.frames):
                    img = frame_data['image']
                    f.write(f"static const unsigned char {name}_frame_{i}[] = {{\n")
                    self.write_image_data(f, img)
                    f.write("};\n\n")
                    
                f.write(f"static const unsigned char* {name}_frames[] = {{\n")
                for i in range(len(self.frames)):
                    f.write(f"    {name}_frame_{i}")
                    if i < len(self.frames) - 1:
                        f.write(",")
                    f.write("\n")
                f.write("};\n\n")
                
                f.write(f"static const int {name}_frame_durations[] = {{")
                for i, frame_data in enumerate(self.frames):
                    f.write(str(frame_data['duration']))
                    if i < len(self.frames) - 1:
                        f.write(", ")
                f.write("};\n")
                f.write(f"#define {name}_FRAME_MS {self.frames[0]['duration']}\n")
            else:
                # Static
                img = self.frames[0]['image']
                f.write(f"#define {name}_WIDTH {img.width}\n")
                f.write(f"#define {name}_HEIGHT {img.height}\n\n")
                f.write(f"static const unsigned char {name}[] = {{\n")
                self.write_image_data(f, img)
                f.write("};\n\n")
                
            f.write(f"#endif // {name}_H\n")
            
        messagebox.showinfo("Success", f"Exported to:\n{filename}")
        
    def write_image_data(self, f, img):
        """Write image RGB data to file"""
        pixels = list(img.getdata())
        for i, (r, g, b) in enumerate(pixels):
            if i % 12 == 0:
                f.write("    ")
            f.write(f"{r:3d}, {g:3d}, {b:3d}")
            if i < len(pixels) - 1:
                f.write(", ")
            if i % 12 == 11:
                f.write("\n")
        f.write("\n")
        
    def add_frame(self):
        """Add a new frame"""
        current_img = self.frames[self.current_frame]['image']
        new_img = current_img.copy()
        self.frames.append({'image': new_img, 'duration': 100})
        self.current_frame = len(self.frames) - 1
        self.update_info()
        self.draw_canvas()
        self.draw_timeline()
        
    def delete_frame(self):
        """Delete current frame"""
        if len(self.frames) <= 1:
            messagebox.showwarning("Cannot Delete", "Must have at least one frame")
            return
        self.frames.pop(self.current_frame)
        self.current_frame = min(self.current_frame, len(self.frames) - 1)
        self.update_info()
        self.draw_canvas()
        self.draw_timeline()
        
    def duplicate_frame(self):
        """Duplicate current frame"""
        current_img = self.frames[self.current_frame]['image']
        current_duration = self.frames[self.current_frame]['duration']
        new_img = current_img.copy()
        self.frames.append({'image': new_img, 'duration': current_duration})
        self.current_frame = len(self.frames) - 1
        self.update_info()
        self.draw_canvas()
        self.draw_timeline()
        
    def toggle_play(self):
        """Toggle animation playback"""
        self.is_playing = not self.is_playing
        if self.is_playing:
            self.play_animation()
            
    def play_animation(self):
        """Play animation"""
        if not self.is_playing or len(self.frames) <= 1:
            return
        self.current_frame = (self.current_frame + 1) % len(self.frames)
        self.update_info()
        self.draw_canvas()
        self.draw_timeline()
        duration = self.frames[self.current_frame]['duration']
        self.root.after(duration, self.play_animation)
        
    def clear_canvas(self):
        """Clear the current frame"""
        if messagebox.askyesno("Clear Canvas", "Clear the current frame?"):
            img = self.frames[self.current_frame]['image']
            draw = ImageDraw.Draw(img)
            draw.rectangle([0, 0, img.width, img.height], fill=(0, 0, 0))
            self.draw_canvas()
            
    def update_info(self):
        """Update info display"""
        if self.frames:
            img = self.frames[self.current_frame]['image']
            info_text = f"Size: {img.width}x{img.height}\nFrame: {self.current_frame + 1}/{len(self.frames)}"
            self.info_label.config(text=info_text)
            
    def draw_canvas(self):
        """Draw the current frame on canvas"""
        if not self.frames:
            return
            
        self.canvas.delete("all")
        img = self.frames[self.current_frame]['image']
        
        # Create zoomed image
        zoomed = img.resize((img.width * self.zoom, img.height * self.zoom), Image.NEAREST)
        
        # Calculate center position
        canvas_w = self.canvas.winfo_width()
        canvas_h = self.canvas.winfo_height()
        x = (canvas_w - zoomed.width) // 2
        y = (canvas_h - zoomed.height) // 2
        
        self.canvas_offset_x = x
        self.canvas_offset_y = y
        
        # Display image
        self.photo = ImageTk.PhotoImage(zoomed)
        self.canvas.create_image(x, y, image=self.photo, anchor=tk.NW)
        
        # Draw grid
        if self.grid_enabled and self.zoom >= 4:
            for gx in range(img.width + 1):
                x_pos = x + gx * self.zoom
                self.canvas.create_line(x_pos, y, x_pos, y + zoomed.height, fill='#555555', width=1)
            for gy in range(img.height + 1):
                y_pos = y + gy * self.zoom
                self.canvas.create_line(x, y_pos, x + zoomed.width, y_pos, fill='#555555', width=1)
                
    def draw_timeline(self):
        """Draw the timeline"""
        self.timeline_canvas.delete("all")
        
        frame_width = 80
        frame_height = 60
        padding = 10
        
        for i, frame_data in enumerate(self.frames):
            x = padding + i * (frame_width + padding)
            y = 10
            
            # Frame box
            color = self.colors['accent'] if i == self.current_frame else self.colors['bg_light']
            self.timeline_canvas.create_rectangle(
                x, y, x + frame_width, y + frame_height,
                fill=color, outline=self.colors['text'], width=2
            )
            
            # Frame thumbnail
            img = frame_data['image']
            thumb = img.resize((frame_width - 10, frame_height - 20), Image.NEAREST)
            photo = ImageTk.PhotoImage(thumb)
            self.timeline_canvas.create_image(x + 5, y + 5, image=photo, anchor=tk.NW)
            # Keep reference
            if not hasattr(self, 'timeline_photos'):
                self.timeline_photos = []
            if i >= len(self.timeline_photos):
                self.timeline_photos.append(photo)
            else:
                self.timeline_photos[i] = photo
                
            # Frame number
            self.timeline_canvas.create_text(
                x + frame_width // 2, y + frame_height + 10,
                text=f"Frame {i + 1}", fill=self.colors['text'],
                font=('Segoe UI', 8, 'bold')
            )
            
    def on_timeline_click(self, event):
        """Handle timeline click"""
        frame_width = 80
        padding = 10
        
        x = event.x - padding
        if x < 0:
            return
            
        frame_index = x // (frame_width + padding)
        if 0 <= frame_index < len(self.frames):
            self.current_frame = frame_index
            self.update_info()
            self.draw_canvas()
            self.draw_timeline()
            
    def canvas_to_pixel(self, cx, cy):
        """Convert canvas coordinates to pixel coordinates"""
        px = (cx - self.canvas_offset_x) // self.zoom
        py = (cy - self.canvas_offset_y) // self.zoom
        return px, py
        
    def on_canvas_click(self, event):
        """Handle canvas mouse click"""
        px, py = self.canvas_to_pixel(event.x, event.y)
        
        self.is_drawing = True
        self.last_x = px
        self.last_y = py
        self.start_x = px
        self.start_y = py
        
        if self.tool in ["pencil", "eraser"]:
            self.draw_point(px, py)
        elif self.tool == "fill":
            self.flood_fill(px, py)
        elif self.tool == "picker":
            self.pick_pixel_color(px, py)
            
    def on_canvas_drag(self, event):
        """Handle canvas mouse drag"""
        if not self.is_drawing:
            return
            
        px, py = self.canvas_to_pixel(event.x, event.y)
        
        if self.tool in ["pencil", "eraser"]:
            self.draw_line(self.last_x, self.last_y, px, py)
            self.last_x = px
            self.last_y = py
            
    def on_canvas_release(self, event):
        """Handle canvas mouse release"""
        if not self.is_drawing:
            return
            
        px, py = self.canvas_to_pixel(event.x, event.y)
        
        if self.tool == "line":
            self.draw_line(self.start_x, self.start_y, px, py)
        elif self.tool == "rect":
            self.draw_rectangle(self.start_x, self.start_y, px, py)
        elif self.tool == "circle":
            self.draw_circle(self.start_x, self.start_y, px, py)
            
        self.is_drawing = False
        
    def draw_point(self, px, py):
        """Draw a single point"""
        img = self.frames[self.current_frame]['image']
        draw = ImageDraw.Draw(img)
        
        brush_size = int(self.brush_slider.get())
        color = (0, 0, 0) if self.tool == "eraser" else self.current_color
        
        draw.ellipse([px - brush_size//2, py - brush_size//2,
                     px + brush_size//2, py + brush_size//2], fill=color)
        self.draw_canvas()
        
    def draw_line(self, x0, y0, x1, y1):
        """Draw a line"""
        img = self.frames[self.current_frame]['image']
        draw = ImageDraw.Draw(img)
        
        brush_size = int(self.brush_slider.get())
        color = (0, 0, 0) if self.tool == "eraser" else self.current_color
        
        draw.line([x0, y0, x1, y1], fill=color, width=brush_size)
        self.draw_canvas()
        
    def draw_rectangle(self, x0, y0, x1, y1):
        """Draw a rectangle"""
        img = self.frames[self.current_frame]['image']
        draw = ImageDraw.Draw(img)
        
        draw.rectangle([x0, y0, x1, y1], fill=self.current_color)
        self.draw_canvas()
        
    def draw_circle(self, x0, y0, x1, y1):
        """Draw a circle"""
        img = self.frames[self.current_frame]['image']
        draw = ImageDraw.Draw(img)
        
        import math
        radius = int(math.sqrt((x1 - x0) ** 2 + (y1 - y0) ** 2))
        
        draw.ellipse([x0 - radius, y0 - radius,
                     x0 + radius, y0 + radius], fill=self.current_color)
        self.draw_canvas()
        
    def flood_fill(self, x, y):
        """Flood fill"""
        img = self.frames[self.current_frame]['image']
        
        if x < 0 or x >= img.width or y < 0 or y >= img.height:
            return
            
        ImageDraw.floodfill(img, (x, y), self.current_color)
        self.draw_canvas()
        
    def pick_pixel_color(self, x, y):
        """Pick color from pixel"""
        img = self.frames[self.current_frame]['image']
        
        if 0 <= x < img.width and 0 <= y < img.height:
            self.current_color = img.getpixel((x, y))
            color_hex = '#%02x%02x%02x' % self.current_color
            self.color_display.config(bg=color_hex)

if __name__ == "__main__":
    root = tk.Tk()
    app = ModernTextureEditor(root)
    root.mainloop()
