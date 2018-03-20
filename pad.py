#!/usr/bin/python3

import sys

if len(sys.argv) < 2:
    sys.exit(1)

in_name = sys.argv[1]
out_name = in_name + "_pad"
with open(in_name, "rb") as in_file:
    with open(out_name, "wb") as out_file:
        in_type = in_file.read(2)
        in_file_size = int.from_bytes(in_file.read(4), byteorder='little', signed=False)
        in_file_res = in_file.read(4)
        offset = int.from_bytes(in_file.read(4), byteorder='little', signed=False)

        # We have to fill in 512 - offset bytes
        num_pad_bytes = 512 - offset
        out_file_size = in_file_size + num_pad_bytes
        out_file.write(in_type)
        out_file.write(out_file_size.to_bytes(4, byteorder='little'))
        out_file.write(in_file_res)
        out_file.write((512).to_bytes(4, byteorder='little'))

        pad_bytes = b"\x00" * num_pad_bytes
        out_file.write(in_file.read(offset - 14))
        out_file.write(pad_bytes)

        out_file.write(in_file.read())
