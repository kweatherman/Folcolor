
"""
Get HSL color statistics for a given icon image.
Used to get the initial HUE distance adjustment offset, and to help tweak output sets to match a target color palette.
Python 3.x

https://opensource.com/article/19/3/python-image-manipulation-tools
https://docs.python-guide.org/scenarios/imaging/

HSL color space:
http://www.chaospro.de/documentation/html/paletteeditor/colorspace_hsl.htm

Online color space converter:
https://www.w3schools.com/colors/colors_converter.asp
https://serennu.com/colour/hsltorgb.php

"""
import os
import sys
import statistics
from colorsys import rgb_to_hls
from PIL import Image   # pip3 install pillow

# "Windows10.ico" "Windows7_8_rgba.ico" "Windows7_8_no_label.ico"
assert len(sys.argv) == 2, "Missing source icon path"

# Get average hue for the 256 ico image
print('Gathering stats for "%s"..' % sys.argv[1])
im = Image.open(sys.argv[1])

hues = []
hue_max = 0.0
hue_min = 360.0
sats = []
sat_max = 0.0
sat_min = 100.0
lums = []
lum_max = 0.0
lum_min = 100.0

png256 = im.ico.getimage(im.ico.getentryindex(256))
for x in range(256):
    for y in range(256):
        pxl = png256.getpixel((x, y))

        # Only opaque pixels
        if pxl[3] == 255:
            # From RGB to HLS (note PIL deviation from the more standard "HSL" naming)
            h, l, s = rgb_to_hls(float(pxl[0]) / 255.0, float(pxl[1]) / 255.0, float(pxl[2]) / 255.0)

            h *= 360.0
            hues.append(h)
            hue_max = max(hue_max, h)
            hue_min = min(hue_min, h)

            l *= 100.0
            lums.append(l)
            lum_max = max(lum_max, l)
            lum_min = min(lum_min, l)

            s *= 100.0
            sats.append(s)
            sat_max = max(sat_max, s)
            sat_min = min(sat_min, s)

print("\nValues: %u" % len(hues))
print("Hue Avg: %.3f, Median: %.3f, Mode: %.3f, Stdev: %.3f, Min: %.3f, Max: %.3f" % (statistics.mean(hues), statistics.median(hues), statistics.mode(hues), statistics.stdev(hues), hue_min, hue_max))
print("Sat Avg: %.3f, Median: %.3f, Mode: %.3f, Stdev: %.3f, Min: %.3f, Max: %.3f" % (statistics.mean(sats), statistics.median(sats), statistics.mode(sats), statistics.stdev(sats), sat_min, sat_max))
print("Lum Avg: %.3f, Median: %.3f, Mode: %.3f, Stdev: %.3f, Min: %.3f, Max: %.3f" % (statistics.mean(lums), statistics.median(lums), statistics.mode(lums), statistics.stdev(lums), lum_min, lum_max))
