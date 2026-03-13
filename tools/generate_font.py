#!/usr/bin/env python3
"""
Generate a bitmap font texture for The Sphere Game.
Supports: English, German, Spanish, French
Uses proper baseline alignment for all characters.
"""

from PIL import Image, ImageDraw, ImageFont
import os

# Configuration
GLYPH_WIDTH = 24
GLYPH_HEIGHT = 32
CHARS_PER_ROW = 16  # Back to 16 since fewer characters

# Character set for multi-language support
# ASCII printable (space through ~)
ASCII_CHARS = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'

# Latin Extended for German, Spanish, French
# German: Ä Ö Ü ä ö ü ß
# Spanish: á é í ó ú ñ Ñ ¿ ¡ ü Ü
# French: à â ç è ê ë î ï ô ù û ÿ œ æ Ç É È Ê Ë Î Ï Ô Ù Û Ÿ Œ Æ À Â « »
LATIN_EXT = 'ÄÖÜäöüßáéíóúñÑ¿¡àâçèêëîïôùûÿœæÇÉÈÊËÎÏÔÙÛŸŒÆÀÂ«»'

# Combine all characters
CHARS = ASCII_CHARS + LATIN_EXT

# Calculate texture dimensions
num_chars = len(CHARS)
num_rows = (num_chars + CHARS_PER_ROW - 1) // CHARS_PER_ROW
TEXTURE_WIDTH = GLYPH_WIDTH * CHARS_PER_ROW
TEXTURE_HEIGHT = GLYPH_HEIGHT * num_rows

print(f"Characters: {num_chars}")
print(f"Grid: {CHARS_PER_ROW}x{num_rows}")
print(f"Texture size: {TEXTURE_WIDTH}x{TEXTURE_HEIGHT}")
print(f"Glyph size: {GLYPH_WIDTH}x{GLYPH_HEIGHT}")

# Create image with transparent background
img = Image.new('RGBA', (TEXTURE_WIDTH, TEXTURE_HEIGHT), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)

# Try to find a good monospace font
font = None
font_size = 24  # Target size

# Try common monospace fonts
font_candidates = [
    "Consolas",
    "Courier New",
    "Lucida Console",
    "C:/Windows/Fonts/consola.ttf",
    "C:/Windows/Fonts/cour.ttf",
    "C:/Windows/Fonts/lucon.ttf",
]

for font_name in font_candidates:
    try:
        if os.path.exists(font_name):
            font = ImageFont.truetype(font_name, font_size)
            print(f"Using font file: {font_name}")
            break
        else:
            font = ImageFont.truetype(font_name, font_size)
            print(f"Using font: {font_name}")
            break
    except (IOError, OSError):
        continue

if font is None:
    print("Warning: No suitable font found, using default")
    font = ImageFont.load_default()

# Get proper font metrics
ascent, descent = font.getmetrics()
print(f"Font metrics: ascent={ascent}, descent={descent}")

# The font metrics include space for accents (Ä, É) which makes centering off.
# Instead, calculate the VISUAL center based on actual rendered characters.

# Measure the visual extent of typical characters
# Use Latin uppercase letters WITHOUT accents as "top" reference
# (accents and Cyrillic Ё extend above - that's fine, they're exceptions)
# Use characters with descenders for "bottom" reference
top_reference = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
bottom_reference = "gjpqy"  # Characters with descenders
middle_reference = "acemnorsuvwxz"  # Characters sitting on baseline

# Find actual visual top (from uppercase letters)
top_bbox = draw.textbbox((0, 0), top_reference, font=font)
visual_top = top_bbox[1]  # Top of uppercase letters

# Find actual visual bottom (from descender characters)
bottom_bbox = draw.textbbox((0, 0), bottom_reference, font=font)
visual_bottom = bottom_bbox[3]  # Bottom of descenders

# Total visual height of the font
visual_height = visual_bottom - visual_top
print(f"Visual bounds: top={visual_top}, bottom={visual_bottom}, height={visual_height}")

# Calculate where to position text so visual content is centered in cell
# We want: cell_center = (visual_top + visual_bottom) / 2 mapped to GLYPH_HEIGHT / 2
# visual_center_offset = visual_top + visual_height / 2 = (visual_top + visual_bottom) / 2
# We draw at y=draw_y, which places origin at draw_y
# The visual center would be at: draw_y + (visual_top + visual_bottom) / 2
# We want this to equal: cell_y + GLYPH_HEIGHT / 2
# So: draw_y = cell_y + GLYPH_HEIGHT / 2 - (visual_top + visual_bottom) / 2
visual_center = (visual_top + visual_bottom) / 2
vertical_offset = GLYPH_HEIGHT / 2 - visual_center

print(f"Visual center offset: {visual_center}, vertical_offset: {vertical_offset}")

# Draw each character
for i, char in enumerate(CHARS):
    row = i // CHARS_PER_ROW
    col = i % CHARS_PER_ROW
    
    cell_x = col * GLYPH_WIDTH
    cell_y = row * GLYPH_HEIGHT
    
    # Get character bounding box for horizontal centering
    bbox = draw.textbbox((0, 0), char, font=font)
    char_width = bbox[2] - bbox[0]
    
    # Horizontal: center the character width in the cell
    draw_x = cell_x + (GLYPH_WIDTH - char_width) // 2 - bbox[0]
    
    # Vertical: center based on visual extent of typical characters
    draw_y = cell_y + vertical_offset
    
    # Draw the character
    draw.text((draw_x, draw_y), char, font=font, fill=(255, 255, 255, 255))

# Add debug visual: draw baseline and cell boundaries (comment out for production)
DEBUG_GRID = False
if DEBUG_GRID:
    for row in range(num_rows):
        for col in range(CHARS_PER_ROW):
            cell_x = col * GLYPH_WIDTH
            cell_y = row * GLYPH_HEIGHT
            # Draw cell boundary in dark gray
            draw.rectangle([cell_x, cell_y, cell_x + GLYPH_WIDTH - 1, cell_y + GLYPH_HEIGHT - 1], outline=(50, 50, 50, 255))
            # Draw visual center line in red
            center_y = cell_y + GLYPH_HEIGHT // 2
            draw.line([(cell_x, center_y), (cell_x + GLYPH_WIDTH - 1, center_y)], fill=(255, 0, 0, 100))

# Save to assets
output_path = os.path.join(os.path.dirname(__file__), '..', 'assets', 'fonts', 'ui.png')
output_path = os.path.normpath(output_path)
img.save(output_path, 'PNG')
print(f"Saved: {output_path}")

# Also print the constants for text.cpp
print("\n// Constants for text.cpp:")
print(f"static constexpr float GLYPH_WIDTH = {GLYPH_WIDTH}.0f;")
print(f"static constexpr float GLYPH_HEIGHT = {GLYPH_HEIGHT}.0f;")
print(f"static constexpr float CHARS_PER_ROW = {CHARS_PER_ROW}.0f;")
print(f"static constexpr float FONT_TEXTURE_WIDTH = {TEXTURE_WIDTH}.0f;")
print(f"static constexpr float FONT_TEXTURE_HEIGHT = {TEXTURE_HEIGHT}.0f;")
