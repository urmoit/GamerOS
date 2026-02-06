import struct
import sys
import os

def convert_cur_to_c(input_file, output_file, variable_name):
    with open(input_file, 'rb') as f:
        data = f.read()

    # ICONDIR header
    reserved, icon_type, count = struct.unpack('<HHH', data[:6])
    if icon_type != 2:
        print(f"Error: {input_file} is not a cursor file")
        return

    # ICONDIRENTRY
    offset = 6
    width, height, colors, res, x_hotspot, y_hotspot, size, data_offset = struct.unpack('<BBBBHHII', data[offset:offset+16])
    
    if width == 0: width = 256
    if height == 0: height = 256

    print(f"Cursor: {width}x{height}, Hotspot: {x_hotspot},{y_hotspot}")

    # DIB data starts at data_offset
    dib_data = data[data_offset:data_offset+size]
    
    # DIB Header (BITMAPINFOHEADER)
    header_size = struct.unpack('<I', dib_data[:4])[0]
    biWidth, biHeight, biPlanes, biBitCount = struct.unpack('<iiHH', dib_data[4:16])
    
    # biHeight is often 2*height in icons/cursors because of the mask
    actual_height = biHeight // 2 if biHeight > height else biHeight
    
    print(f"DIB: {biWidth}x{actual_height}, bpp: {biBitCount}")

    with open(output_file, 'w') as out:
        out.write(f"#ifndef {variable_name.upper()}_H\n")
        out.write(f"#define {variable_name.upper()}_H\n\n")
        out.write("#include <stdint.h>\n\n")
        out.write(f"const uint32_t {variable_name}_width = {width};\n")
        out.write(f"const uint32_t {variable_name}_height = {height};\n")
        out.write(f"const uint32_t {variable_name}_x_hotspot = {x_hotspot};\n")
        out.write(f"const uint32_t {variable_name}_y_hotspot = {y_hotspot};\n\n")
        
        out.write(f"const uint8_t {variable_name}_data[] = {{\n")
        for i, b in enumerate(dib_data):
            out.write(f"0x{b:02x}, ")
            if (i + 1) % 16 == 0:
                out.write("\n")
        out.write("\n};\n\n")
        out.write("#endif\n")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python convert_cursor.py <input.cur> <output.h> <variable_name>")
    else:
        convert_cur_to_c(sys.argv[1], sys.argv[2], sys.argv[3])
