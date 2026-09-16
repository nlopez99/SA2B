from argparse import ArgumentParser
import struct
mtE = [
    "GJ_MT_VTXATTR",
    "GJ_MT_VCD",
    "GJ_MT_FST1",
    "GJ_MT_FST2",
    "GJ_MT_BLEND",
    "GJ_MT_DIFFUSE",
    "GJ_MT_AMBIENT",
    "GJ_MT_SPECULAR",
    "GJ_MT_TEXTURE",
    "GJ_MT_TEVORDER",
    "GJ_MT_TEXGEN",
]

mtE_lookup = { i:x for i,x in enumerate(mtE) }

def convert_binary_to_mt_c_source(src_path, dest_path):
    # Load data
    with open(src_path, "rb") as binary_file, open(dest_path, "w") as c_file:
        # iter through all data, consuming one vtx at a time
        
        for mt_tuple in struct.iter_unpack(">bxxxI", binary_file.read()):
            c_file.write(f"\t{{ {mtE_lookup.get(mt_tuple[0], mt_tuple[0])}, {mt_tuple[1]:#08x} }},\n")


def main():
    parser = ArgumentParser(
        description="Converts a binary file to an array of GJ material types"
    )
    parser.add_argument("src_path", type=str, help="Binary source file path")
    parser.add_argument("dest_path", type=str,
                        help="Destination C include file path")

    args = parser.parse_args()
    convert_binary_to_mt_c_source(args.src_path, args.dest_path)


if __name__ == "__main__":
    main()
