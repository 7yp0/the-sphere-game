#!/usr/bin/env python3
"""Generate placeholder UI textures for the inventory system."""

from PIL import Image, ImageDraw

def create_inventory_button(path, size=48):
    """Create inventory button icon (bag/backpack shape)."""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Draw a simple bag shape
    margin = 4
    bag_top = size // 4
    
    # Bag body (rounded rectangle approximation)
    draw.rectangle([margin, bag_top, size - margin, size - margin], 
                   fill=(80, 70, 60, 230), outline=(120, 110, 90, 255), width=2)
    
    # Bag flap (closed)
    draw.rectangle([margin + 2, bag_top, size - margin - 2, bag_top + 8], 
                   fill=(100, 90, 75, 255))
    
    # Handle/strap
    draw.arc([size//3, 2, size*2//3, bag_top + 4], 0, 180, fill=(90, 80, 65, 255), width=3)
    
    # Buckle/clasp
    draw.rectangle([size//2 - 4, bag_top + 10, size//2 + 4, bag_top + 18], 
                   fill=(180, 160, 100, 255))
    
    img.save(path)
    print(f"Created: {path}")

def create_inventory_button_hover(path, size=48):
    """Create inventory button hover icon (open bag)."""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    margin = 4
    bag_top = size // 4
    
    # Bag body (same as closed)
    draw.rectangle([margin, bag_top + 6, size - margin, size - margin], 
                   fill=(80, 70, 60, 230), outline=(120, 110, 90, 255), width=2)
    
    # Open bag top - darker inside visible
    draw.ellipse([margin + 4, bag_top - 2, size - margin - 4, bag_top + 14], 
                 fill=(50, 45, 40, 255), outline=(120, 110, 90, 255), width=2)
    
    # Handle/strap (slightly higher)
    draw.arc([size//3, 0, size*2//3, bag_top], 0, 180, fill=(90, 80, 65, 255), width=3)
    
    # Highlight to show it's interactive
    draw.rectangle([margin + 1, bag_top + 7, size - margin - 1, size - margin - 1], 
                   outline=(150, 140, 120, 100), width=1)
    
    img.save(path)
    print(f"Created: {path}")

def create_slot_background(path, size=64):
    """Create slot background with subtle border."""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Inner fill (dark)
    draw.rectangle([2, 2, size - 3, size - 3], fill=(40, 40, 50, 200))
    
    # Border (lighter)
    draw.rectangle([0, 0, size - 1, size - 1], outline=(80, 80, 100, 255), width=2)
    
    # Inner highlight (top-left)
    draw.line([(2, 2), (size - 4, 2)], fill=(70, 70, 85, 255), width=1)
    draw.line([(2, 2), (2, size - 4)], fill=(70, 70, 85, 255), width=1)
    
    # Inner shadow (bottom-right)
    draw.line([(3, size - 3), (size - 3, size - 3)], fill=(25, 25, 35, 255), width=1)
    draw.line([(size - 3, 3), (size - 3, size - 3)], fill=(25, 25, 35, 255), width=1)
    
    img.save(path)
    print(f"Created: {path}")

def create_panel_background(path, size=320):
    """Create panel background texture (tileable or single)."""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Main fill (dark blue-gray)
    draw.rectangle([0, 0, size - 1, size - 1], fill=(30, 30, 45, 240))
    
    # Outer border (dark)
    draw.rectangle([0, 0, size - 1, size - 1], outline=(20, 20, 30, 255), width=3)
    
    # Inner border (lighter)
    draw.rectangle([4, 4, size - 5, size - 5], outline=(60, 60, 80, 255), width=1)
    
    # Subtle corner accents
    corner_size = 12
    accent_color = (70, 70, 90, 255)
    # Top-left
    draw.line([(6, 6), (6 + corner_size, 6)], fill=accent_color, width=2)
    draw.line([(6, 6), (6, 6 + corner_size)], fill=accent_color, width=2)
    # Top-right
    draw.line([(size - 7, 6), (size - 7 - corner_size, 6)], fill=accent_color, width=2)
    draw.line([(size - 7, 6), (size - 7, 6 + corner_size)], fill=accent_color, width=2)
    # Bottom-left
    draw.line([(6, size - 7), (6 + corner_size, size - 7)], fill=accent_color, width=2)
    draw.line([(6, size - 7), (6, size - 7 - corner_size)], fill=accent_color, width=2)
    # Bottom-right
    draw.line([(size - 7, size - 7), (size - 7 - corner_size, size - 7)], fill=accent_color, width=2)
    draw.line([(size - 7, size - 7), (size - 7, size - 7 - corner_size)], fill=accent_color, width=2)
    
    img.save(path)
    print(f"Created: {path}")

if __name__ == "__main__":
    import os
    
    inv_dir = "assets/ui/inventory"
    items_dir = "assets/ui/items"
    os.makedirs(inv_dir, exist_ok=True)
    os.makedirs(items_dir, exist_ok=True)
    
    create_inventory_button(f"{inv_dir}/inventory_button.png", 48)
    create_inventory_button_hover(f"{inv_dir}/inventory_button_hover.png", 48)
    create_slot_background(f"{inv_dir}/slot_bg.png", 64)
    create_panel_background(f"{inv_dir}/panel_bg.png", 320)
    
    print("\nDone! Textures created in assets/ui/")
