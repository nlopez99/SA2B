from argparse import ArgumentParser
import struct


def convert_binary_to_vtx_c_source(src_path, dest_path):
    # Load data
    with open(src_path, "rb") as binary_file, open(dest_path, "w") as c_file:
        bs = binary_file.read()
        c_file.write(", ".join(f"{b:#02x}" for b in bs))


def main():
    parser = ArgumentParser(
        description="(WIP) Converts a binary file to a GC displaylist"
    )
    parser.add_argument("src_path", type=str, help="Binary source file path")
    parser.add_argument("dest_path", type=str,
                        help="Destination C include file path")

    args = parser.parse_args()
    convert_binary_to_vtx_c_source(args.src_path, args.dest_path)


if __name__ == "__main__":
    main()
