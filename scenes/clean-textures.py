#!/usr/bin/env python

# Unfinished, and no longer neccesary. Keeping it in the repo just for reference
# 
# Pipeline tool for cleaning up transparent pixels in textures
# Sets color of transparent pixels to be the same as nearest non-transparent pixel
# to prevent artifacts caused by bilinear texture filtering
#
# Will perform this adjustment to ALL textures in textures folder DESTRUCTIVELY
#
# Usage: python3 ./scenes/clean-textures.py ./dist/assets/textures/
# Must have Pillow installed: python3 -m pip install --upgrade Pillow
# Pillow docs here: https://pillow.readthedocs.io/en/stable/index.html
#
# Scott King (11/10/22)

import sys
import re
from PIL import Image
import os

# Helper Functions
def clamp(num, min_bound, max_bound):
   return max(min(num, max_bound), min_bound)

def main():
    if len(sys.argv) != 2:
        print(f"\nUsage:\n {sys.argv[0]} [./dist/assets/textures/]\n")
        exit(1)

    textures_dir = sys.argv[1]
    # Loop through ALL files in given directory
    for subdir, dirs, files in os.walk(textures_dir):
        for file in files:
            filename = os.path.join(subdir, file)
            if (not filename.lower().endswith('.png')):
                print("Found a file that is not a png, ignoring...")
            else:
                print(f"Processing {os.path.join(subdir, file)}")

                with Image.open(filename) as texture:
                    if (texture.mode == "RGBA"):
                        pixels = texture.load()
                        width = texture.width
                        height = texture.height

                        for x in range(width):
                            for y in range(height):
                                # check for fully transparent pixels, ignore only partially transparent ones
                                if pixels[x, y][3] == 0:
                                    # look for nearest pixel that is not fully transparent, and use first one found to set color of this pixel
                                    found = False
                                    cur_extent = 1
                                     # Needs a LOT of optimization to be usable, more of a proof of concept
                                    while not found and (cur_extent < width or cur_extent < height):
                                        for i in range(clamp(x-cur_extent, 0, x), clamp(x+cur_extent, 0, width - 1)):
                                            for j in range(clamp(y-cur_extent, 0, y), clamp(y+cur_extent, 0, height - 1)):
                                                if pixels[i, j][3] != 0:
                                                    # copy color of most recently found non-transparent pixel into current pixel, but retain alpha value (of 0)
                                                    texture.putpixel((x, y), (pixels[i, j][0], pixels[i, j][1], pixels[i, j][2], pixels[x, y][3]))
                                                    found = True
                                        cur_extent += 1
                        # Save texture with changes applied, overwriting original image          
                        texture.save(filename)
                    else:
                        print("No alpha channel present in this png, skipping...")

if __name__ == '__main__':
    main()