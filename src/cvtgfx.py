#(c) 2024 David Shadoff
import sys
import os
import datetime

# Notes:
#
# This program assists in the production of data for the 6270 character and sprite patterns to be used
# when creating PC-FX programs, especially when using the modern gcc 'C' compiler.
#
# usage:
#   pce_gfx <tile|sprite> <obj_name> <ASCII_art_file> <offset_x> <offset_y> <x_tiles> <y_tiles> <x_virtual_size> <xlate_file>
#
# example:
#   python pce_gfx.py sprite spr0_data sprdata.txt 16 0 2 2 2 sprdata.xlate
#
# This will create an output file named 'spr0_data.gen_data'
#   - containing a 'C' uint16_t array definition named 'spr0_data'
#   - which contains sprite tiles (16 pix x 16 pix)
#   - arranged in a 2 x 2 grid ( A = top-left, B = top-right, C = bottom-left, D = bottom-right)
#   - based on the idea that these do not have additional gaps (i.e. virtual width = x_widht)
#   - as defined in the ASCII art file 'sprdata.txt'
#   - on line 0 (y_offset), starting at character 16 (where "character 0" is the first on the line)
#   - ASCII art can use any characters, where their position within the 'sprdata.xlate' file indicates the value
#      (for example, if 'sprdata.xlate' contains '.x(*#@=)-_abcdv^', then 'x' in the ASCII art will be the value '1')
#

#
# hexdecode: Get a value and convert it if it has a hexadecimal prefix
#
def hexdecode(input):
    hexadecimal = 0
    num = 0
    if input[0] == "$":
        hexadecimal = 1
        hexnum = input[1:]
    elif input[0:2] == "0x" or input[0:2] == "0X":
        hexadecimal = 1
        hexnum = input[2:]

    if hexadecimal == 1:
        num = int(hexnum, 16)
    else:
        num = int(input)
    return num


def usage():
    print("")
    print("Usage:")
    print("    pce_gfx <tile|sprite> <obj_name> <ASCII_art_file> <offset_x> <offset_y> <x_tiles> <y_tiles> <x_virtual_size> <xlate_file>")
    exit()


#
## Main Program:
#

if (len(sys.argv) != 10):
    print("Incorrect number of parameters")
    usage()

tilesprite = sys.argv[1].upper()

if ((tilesprite != 'TILE') and (tilesprite != 'SPRITE')):
    print("Error - must state 'TILE' or 'SPRITE'")
    usage()

objectname = sys.argv[2]

infilename = sys.argv[3]
if (os.path.isfile(infilename) == False):
    print("File {0} does not exist".format(infilename))
    usage()

xreffilename = sys.argv[9]
if (os.path.isfile(xreffilename) == False):
    print("File {0} does not exist".format(xreffilename))
    usage()

x_offset = int(sys.argv[4])
y_offset = int(sys.argv[5])

x_tiles = int(sys.argv[6])
y_tiles = int(sys.argv[7])
virt_width = int(sys.argv[8])

if (x_tiles > virt_width):
    print("Horizontal tiles cannot be greater than virtual width")
    usage()

# Type = tile
if (tilesprite == 'TILE'):
    byte_width = 1
    x_bits = 8
    y_bits = 8
    plane_0_offset = 0
    plane_1_offset = 1
    plane_2_offset = 16
    plane_3_offset = 17
    bytes_in_tile  = 0x20
else:
# type = sprite
    byte_width = 2
    x_bits = 16
    y_bits = 16
    plane_0_offset = 0
    plane_1_offset = 32
    plane_2_offset = 64
    plane_3_offset = 96
    bytes_in_tile  = 128

data_array = bytearray(x_tiles * y_tiles * bytes_in_tile)


# Cross-reference file for sub-palette indexes
# of characters used in ASCII art
#
xreffile = open(xreffilename, "r")
xrefstr = xreffile.readline(16)
xreffile.close()

asciiart = [  ]     # to create an array of strings

# input text file containing the ASCII art
#
infile = open(infilename, "r")
myline = infile.readline()

mylinelen = 512         # fictitious width of file
numlines = 0

while myline:
    if (myline[-1] == "\n"):
        myline = myline[:-1]

    # find minimum line length in file
    if (len(myline) < mylinelen):
        mylinelen = len(myline)

    numlines = numlines + 1
    asciiart.append(myline)
    myline = infile.readline()

infile.close()

if ((y_offset + y_bits) > numlines):
    print(tilesprite, " extends beyond the lower edge of the ASCII art file")
    usage()

if ((x_offset + x_bits) > mylinelen):
    print(tilesprite, " extends beyond the right edge of the ASCII art file")
    usage()


i = 0
while (i < y_tiles):

    j = 0
    while (j < x_tiles):

        offset = (((virt_width * i) + j) * bytes_in_tile);

        k = 0
        while (k < y_bits):

            l = x_bits
            while (l > 0):
                if (l > 8):
                    extra_byte = 1
                    shift_bit = l - 9
                else:
                    extra_byte = 0
                    shift_bit = l - 1

                # this would be (k * byte_width), but tiles also need 2 bytes offset
                tile_offset = (k * 2) + extra_byte

                x1 = x_offset + (j * x_bits) + (x_bits-l)
                y1 = y_offset + (i * y_bits) + k

                buff = asciiart[y1][x1]
                buffbit = xrefstr.find(buff)
                if (buffbit == -1):
                    print("Can't find ", buff, " in translate file")
                    exit()

                pos_bitmask = (1 << shift_bit)
                neg_bitmask = ~(pos_bitmask)

                p0 = (data_array[offset + tile_offset+plane_0_offset] & neg_bitmask)
                p0 = (p0 | pos_bitmask)  if (buffbit & 0x01) else p0
                data_array[offset + tile_offset+plane_0_offset] = p0

                p1 = (data_array[offset + tile_offset+plane_1_offset] & neg_bitmask)
                p1 = (p1 | pos_bitmask)  if (buffbit & 0x02) else p1
                data_array[offset + tile_offset+plane_1_offset] = p1

                p2 = (data_array[offset + tile_offset+plane_2_offset] & neg_bitmask)
                p2 = (p2 | pos_bitmask)  if (buffbit & 0x04) else p2
                data_array[offset + tile_offset+plane_2_offset] = p2

                p3 = (data_array[offset + tile_offset+plane_3_offset] & neg_bitmask)
                p3 = (p3 | pos_bitmask)  if (buffbit & 0x08) else p3
                data_array[offset + tile_offset+plane_3_offset] = p3

                l = l - 1
            k = k + 1
        j = j + 1
    i = i + 1

time_now = datetime.datetime.today().strftime('%Y-%m-%d %H:%M:%S')

outfilename = objectname + ".gen_data"
header1 = '// {0}\n'.format(outfilename, tilesprite)
header2 = '// HuC6270 {0} data\n'.format(tilesprite)
header3 = '//\n'
header4 = '// Generated on {0}\n'.format(time_now)
header5 = '// Generated by the cvtgfx script by David Shadoff\n'
header6 = '//\n'
header7 = '\n'

outfile = open(outfilename, "w")

outfile.write(header1)
outfile.write(header2)
outfile.write(header3)
outfile.write(header4)
outfile.write(header5)
outfile.write(header6)
outfile.write(header7)

txt = "const uint16_t {0}[] = {1}\n".format(objectname, "{")
outfile.write(txt)

i = 0
num_array_bytes = (x_tiles * y_tiles * bytes_in_tile)
while (i < num_array_bytes):
    textstr = ''
    # if start of line, indent
    if ((i % 16) == 0):
        textstr = textstr + '  '

    # print hexadecimal value
    intval = (data_array[i+1] * 256) + data_array[i]
    textstr = textstr + '0x{0:0{1}X}'.format(intval,4)
    i = i + 2

    # separate with commase except if end of definition
    if (i != num_array_bytes):
        textstr = textstr + ','

    # every 8th entry, carriage return
    if ((i % 16) == 0):
        textstr = textstr + '\n'

    outfile.write(textstr)

endstr = '};\n'
outfile.write(endstr)

outfile.close()

