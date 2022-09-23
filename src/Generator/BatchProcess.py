
"""
Batch process Windows 10 and 7/8 icon files colorings
Python 3.x

https://opensource.com/article/19/3/python-image-manipulation-tools
https://docs.python-guide.org/scenarios/imaging/

Online color space converter:
https://www.w3schools.com/colors/colors_converter.asp
https://serennu.com/colour/hsltorgb.php
"""
import io
import os
import sys
import struct
import statistics
from pprint import pprint
from colorsys import rgb_to_hls, hls_to_rgb
from PIL import Image   # pip3 install pillow

"""
Windows10.ico:
Values: 25,559
Hue Avg: 45.228, Median: 45.149, Mode: 44.776,  Stdev: 0.582, Min: 43.235, Max: 46.567
Sat Avg: 74.136, Median: 73.333, Mode: 82.353,  Stdev: 5.108, Min: 64.510, Max: 87.255
Lum Avg: 92.817, Median: 94.161, Mode: 100.000, Stdev: 6.975, Min: 74.444, Max: 100.000

Windows7_8_no_label.ico:
Values: 31,355
Hue Avg: 48.765, Median: 48.358, Mode: 53.924,  Stdev: 2.748,  Min: 41.351, Max: 55.714
Sat Avg: 71.403, Median: 72.157, Mode: 84.510,  Stdev: 10.190, Min: 30.392, Max: 97.843
Lum Avg: 76.613, Median: 79.661, Mode: 100.000, Stdev: 17.735, Min: 31.034, Max: 100.000

Google Material Design (GMD) color palette:
https://material-ui.com/customization/color/
With the base Win10 SL its between the 200 and 300 shade
"""

base_path = '../Controller/Resources/'

# Windows 11 color set tweak table
# TODO: This could use more TLC. The LS parts are not tweaked, just copied from the Win10 set
win11_hue = -45.228  # Hue offset
win11_table = [
    # Output file name         HUE (0-360)        Sat    Level scalar
    ["Win11set/Black.ico",     win11_hue + 50.0,  0.0,   0.35],     # grey 800  hsl(0, 0%, 26%)
    ["Win11set/Blue gray.ico", win11_hue + 200.0, 0.195, 0.73],     # blueGrey 400  hsl(200, 15%, 54%)
    ["Win11set/Blue.ico",      win11_hue + 207.0, 0.99,  0.92],     # blue 400  hsl(207, 89%, 68%)
    ["Win11set/Brown.ico",     win11_hue + 16.0,  0.215, 0.64],     # brown 400  hsl(16, 18%, 47%)
    ["Win11set/Cyan.ico",      win11_hue + 187,   0.79,  0.8],      # cyan 300  hsl(187, 71%, 59%)
    ["Win11set/Gray.ico",      win11_hue + 50.0,  0.0,   1.1],      # grey 300  hsl(0, 0%, 88%)
    ["Win11set/Green.ico",     win11_hue + 123.0, 0.45,  0.77],     # green 400  hsl(123, 38%, 57%)
    ["Win11set/Lime.ico",      win11_hue + 66.0,  0.78,  0.80],     # lime 400  hsl(66, 70%, 61%)
    ["Win11set/Orange.ico",    win11_hue + 36.0,  1.5,   0.86],     # orange 300  hsl(36, 100%, 65%)
    ["Win11set/Pink.ico",      win11_hue + 340.0, 0.92,  0.99],     # pink 300  hsl(340, 83%, 66%)
    ["Win11set/Purple.ico",    win11_hue + 291.0, 0.53,  0.815],    # purple 300  hsl(291, 47%, 60%)
    ["Win11set/Red.ico",       win11_hue + 1.0,   0.92,  0.85],     # red 400  hsl(1, 83%, 63%)
    ["Win11set/Teal.ico",      win11_hue + 174.0, 0.476, 0.69],     # teal 300  hsl(174, 42%, 51%)
    ["Win11set/Yellow.ico",    win11_hue + 54.0,  1.22,  0.91]      # yellow 400  hsl(54, 100%, 67%)
]

# Windows 10 color set tweak table
win10_hue = -45.228  # Hue offset
win10_table = [
    # Output file name         HUE (0-360)        Sat    Level scalar
    ["Win10set/Black.ico",     win10_hue + 50.0,  0.0,   0.35],     # grey 800  hsl(0, 0%, 26%)
    ["Win10set/Blue gray.ico", win10_hue + 200.0, 0.195, 0.73],     # blueGrey 400  hsl(200, 15%, 54%)
    ["Win10set/Blue.ico",      win10_hue + 207.0, 0.99,  0.92],     # blue 400  hsl(207, 89%, 68%)
    ["Win10set/Brown.ico",     win10_hue + 16.0,  0.215, 0.64],     # brown 400  hsl(16, 18%, 47%)
    ["Win10set/Cyan.ico",      win10_hue + 187,   0.79,  0.8],      # cyan 300  hsl(187, 71%, 59%)
    ["Win10set/Gray.ico",      win10_hue + 50.0,  0.0,   1.1],      # grey 300  hsl(0, 0%, 88%)
    ["Win10set/Green.ico",     win10_hue + 123.0, 0.45,  0.77],     # green 400  hsl(123, 38%, 57%)
    ["Win10set/Lime.ico",      win10_hue + 66.0,  0.78,  0.80],     # lime 400  hsl(66, 70%, 61%)
    ["Win10set/Orange.ico",    win10_hue + 36.0,  1.5,   0.86],     # orange 300  hsl(36, 100%, 65%)
    ["Win10set/Pink.ico",      win10_hue + 340.0, 0.92,  0.99],     # pink 300  hsl(340, 83%, 66%)
    ["Win10set/Purple.ico",    win10_hue + 291.0, 0.53,  0.815],    # purple 300  hsl(291, 47%, 60%)
    ["Win10set/Red.ico",       win10_hue + 1.0,   0.92,  0.85],     # red 400  hsl(1, 83%, 63%)
    ["Win10set/Teal.ico",      win10_hue + 174.0, 0.476, 0.69],     # teal 300  hsl(174, 42%, 51%)
    ["Win10set/Yellow.ico",    win10_hue + 54.0,  1.22,  0.91]      # yellow 400  hsl(54, 100%, 67%)
]

# Windows 7 & 8 base icon tweak table
# TODO: This could use more TLC. The LS parts are not tweaked, just copied from the Win10 set
win7_8_hue = -48.765
win7_8_table = [
    ["Win7_8set/Black.ico", win7_8_hue + 50.0,      0.0,   0.35],   # grey 800  hsl(0, 0%, 26%)
    ["Win7_8set/Blue gray.ico", win7_8_hue + 200.0, 0.195, 0.73],   # blueGrey 400  hsl(200, 15%, 54%)
    ["Win7_8set/Blue.ico", win7_8_hue + 207.0,      0.99,  0.92],   # blue 400  hsl(207, 90%, 61%)
    ["Win7_8set/Brown.ico", win7_8_hue + 16.0,      0.215, 0.64],   # brown 400  hsl(16, 18%, 47%)
    ["Win7_8set/Cyan.ico", win7_8_hue + 187,        0.79,  0.8],    # cyan 300  hsl(187, 71%, 59%)
    ["Win7_8set/Gray.ico", win7_8_hue + 50.0,       0.0,   1.1],    # grey 300  hsl(0, 0%, 88%)
    ["Win7_8set/Green.ico", win7_8_hue + 123.0,     0.45,  0.77],   # green 400  hsl(123, 38%, 57%)
    ["Win7_8set/Lime.ico", win7_8_hue + 66.0,       0.78,  0.83],   # lime 400  hsl(66, 70%, 61%)
    ["Win7_8set/Orange.ico", win7_8_hue + 36.0,     1.5,   0.88],   # orange 300  hsl(36, 100%, 65%)
    ["Win7_8set/Pink.ico", win7_8_hue + 340.0,      0.92,  0.99],   # pink 300  hsl(340, 83%, 66%)
    ["Win7_8set/Purple.ico", win7_8_hue + 291.0,    0.53,  0.815],  # purple 300  hsl(291, 47%, 60%)
    ["Win7_8set/Red.ico", win7_8_hue + 1.0,         0.92,  0.85],   # red 400  hsl(1, 83%, 63%)
    ["Win7_8set/Teal.ico", win7_8_hue + 174.0,      0.476, 0.69],   # teal 300  hsl(174, 42%, 51%)
    ["Win7_8set/Yellow.ico", win7_8_hue + 54.0,     1.22,  0.91]    # yellow 400  hsl(54, 100%, 67%)
]


# Apply hue to a copy of the base image and save it
def apply_color(im, out_path, hue_o, sat_s, lum_s, isWin10):
    print("%s:" % out_path)

    # PIL uses scalar values
    hue_o /= 360.0
    assert hue_o <= 1.0

    label_x_limit = 1000

    # Apply color tweak to each sub-icon from the base input set
    out_frames = []
    for i in range(im.ico.nb_items):
        png_im = im.ico.frame(i)
        pxi = png_im.load()
        print(" [%u] size: %u" % (i, png_im.width))

        # Approximate Win 7/8 label x start position
        if not isWin10:
            label_x_limit = (png_im.width * 0.6)

        new_im = Image.new("RGBA", (png_im.width, png_im.width), "purple")
        pxo = new_im.load()

        for y in range(png_im.width):
            for x in range(png_im.width):
                # Pixel from RGB to HLS color space
                r, g, b, a = pxi[x, y]

                # No need to colorize transparent pixels
                if a > 0:
                    h, l, s = rgb_to_hls(float(r) / 255.0, float(g) / 255.0, float(b) / 255.0)

                    # Leave the Windows 7/8 folder label alone
                    # The label is white (zero'ish hue) and blue range(around 90 to 300)
                    if not ((x >= label_x_limit) and (h < (40.0 / 360.0)) or (h > (100.0 / 360.0))):
                        # Tweak pixel
                        h = (h + hue_o)
                        # Hue is circular, let it wrap around
                        if h > 1.0:
                            h -= 1.0
                        s = min(1.0, (s * sat_s))
                        l = min(1.0, (l * lum_s))

                        # Back to RGB from HLS
                        r, g, b = hls_to_rgb(h, l, s)
                        r = min(int(r * 255.0), 255)
                        g = min(int(g * 255.0), 255)
                        b = min(int(b * 255.0), 255)

                pxo[x, y] = (r, g, b, a)

        #new_im.show()
        out_frames.append(new_im)

        """
        Write out the the modified icon copy.
        PIL(Pillow) while supports full loading of multi-size embedded.ico files, it has limited support for writing
        them back out. It takes the first/parent image and saves out scaled down filtered thumbnail sizes instead.
        The IcoImageFile _save() function was almost complete though. Here is a copy of that code with the copy part
        replaced with individual "frame" writes instead.
        As an added bonus from the code, it saves the icon sections out as compressed .png sections (supported since Windows Vista)
        which makes our icon sets much smaller than the default BMP section format that most icon tools use.
        """
        fp = open(base_path + out_path, 'wb')
        fp.write(b"\0\0\1\0")  # Magic
        fp.write(struct.pack("<H", len(out_frames)))  # idCount(2)
        offset = (fp.tell() + len(out_frames) * 16)

        for ni in out_frames:
            width, height = ni.width, ni.height
            # 0 means 256
            fp.write(struct.pack("B", width if width < 256 else 0))  # bWidth(1)
            fp.write(struct.pack("B", height if height < 256 else 0))  # bHeight(1)
            fp.write(b"\0")  # bColorCount(1)
            fp.write(b"\0")  # bReserved(1)
            fp.write(b"\0\0")  # wPlanes(2)
            fp.write(struct.pack("<H", 32))  # wBitCount(2)

            image_io = io.BytesIO()
            ni.save(image_io, "png")
            image_io.seek(0)
            image_bytes = image_io.read()
            bytes_len = len(image_bytes)
            fp.write(struct.pack("<I", bytes_len))  # dwBytesInRes(4)
            fp.write(struct.pack("<I", offset))  # dwImageOffset(4)
            current = fp.tell()
            fp.seek(offset)
            fp.write(image_bytes)
            offset = offset + bytes_len
            fp.seek(current)

        fp.close()
    print(" ")


# From Windows base icon image to our color sets
def process_icon_base(in_file, nfo_table, isWin10):
    # Load base icon image
    im = Image.open(in_file)
    #print(in_file, im.info, im.format, im.size, im.mode)

    # Build color sets
    for e in nfo_table:
        apply_color(im, e[0], e[1], e[2], e[3], isWin10)

# Windows 11
print("-- Creating Windows 11 set..")
process_icon_base("Windows11.ico", win11_table, True)

# Windows 10
print("-- Creating Windows 10 set..")
process_icon_base("Windows10.ico", win10_table, True)

# Windows 7/8
print("-- Creating Windows 7 & 8 set..")
process_icon_base("Windows7_8_rgba.ico", win7_8_table, False)
