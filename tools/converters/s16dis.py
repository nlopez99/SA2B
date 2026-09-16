from argparse import ArgumentParser
import struct


def convert_binary_to_vtx_c_source(src_path, dest_path):
    # Load data
    with open(src_path, "rb") as binary_file, open(dest_path, "w") as c_file:
        # iter through all data, consuming one vtx at a time
        for tup in struct.iter_unpack(">h", binary_file.read()):
            c_file.write(f"\t{tup[0]},\n")


def main():
    parser = ArgumentParser(
        description="Converts a binary file to an array of shorts"
    )
    parser.add_argument("src_path", type=str, help="Binary source file path")
    parser.add_argument("dest_path", type=str,
                        help="Destination C include file path")

    args = parser.parse_args()
    convert_binary_to_vtx_c_source(args.src_path, args.dest_path)


if __name__ == "__main__":
    main()
