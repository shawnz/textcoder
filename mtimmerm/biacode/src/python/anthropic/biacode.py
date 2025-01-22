import argparse
import sys

from arithmetic_decoder import ArithmeticDecoder
from arithmetic_encoder import ArithmeticEncoder
from foio import FOBitInputStream, FOBitOutputStream
from simple_model import SimpleAdaptiveModel


def compress(input_stream, output_stream, block_size=1):
    model = SimpleAdaptiveModel(256)
    outbits = FOBitOutputStream(output_stream, block_size)
    encoder = ArithmeticEncoder(outbits)

    while True:
        byte = input_stream.read(1)
        if not byte:
            break

        sym = byte[0]
        encoder.encode(model, sym, True)
        model.update(sym)

    encoder.end()
    outbits.end()


def decompress(input_stream, output_stream, block_size=1):
    model = SimpleAdaptiveModel(256)
    decoder = ArithmeticDecoder(FOBitInputStream(input_stream, block_size))

    while True:
        sym = decoder.decode(model, True)
        if sym < 0:
            break

        output_stream.write(bytes([sym]))
        model.update(sym)


def main():
    parser = argparse.ArgumentParser(description="Bijective Arithmetic Encoder/Decoder")
    parser.add_argument("input_file", help="Input file to compress/decompress")
    parser.add_argument("output_file", help="Output file")
    parser.add_argument(
        "-d", "--decompress", action="store_true", help="Decompress mode"
    )
    parser.add_argument(
        "-b", "--block_size", type=int, default=1, help="Compressed block size"
    )

    args = parser.parse_args()

    try:
        with (
            open(args.input_file, "rb") as infile,
            open(args.output_file, "wb") as outfile,
        ):
            if args.decompress:
                decompress(infile, outfile, args.block_size)
            else:
                compress(infile, outfile, args.block_size)
    except IOError as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
