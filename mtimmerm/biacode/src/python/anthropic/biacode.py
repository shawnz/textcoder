import sys
import argparse
from arithmetic_decoder import ArithmeticDecoder
from arithmetic_encoder import ArithmeticEncoder
from simple_model import SimpleAdaptiveModel


class FOBitOutputStream:
    def __init__(self, stream, block_size=1):
        self.stream = stream
        self.block_size = max(1, block_size)
        self.reserve0 = False
        self.block_left = 0
        self.seg_size = 0
        self.seg_first = 0

    def write(self, byte):
        if not self.seg_size:
            self.seg_first = byte
            self.seg_size = 1
        elif byte == 0:
            self.seg_size += 1
        else:
            if not self.block_left:
                if self.reserve0:
                    self.reserve0 = not (self.seg_first & 0x7F)
                else:
                    self.reserve0 = not self.seg_first
                self.block_left = self.block_size - 1
            else:
                self.reserve0 = self.reserve0 and not self.seg_first
                self.block_left -= 1

            self.stream.write(bytes([self.seg_first ^ 0x55]))

            for _ in range(self.seg_size - 1):
                if not self.block_left:
                    self.reserve0 = True
                    self.block_left = self.block_size - 1
                else:
                    self.block_left -= 1
                self.stream.write(b"\x55")  # 0 ^ 0x55

            self.seg_first = byte
            self.seg_size = 1

    def end(self):
        if not self.seg_size:
            self.seg_first = 0

        while True:
            while self.block_left > 0:
                self.reserve0 = self.reserve0 and not self.seg_first
                self.stream.write(bytes([self.seg_first ^ 0x55]))
                self.seg_first = 0
                self.block_left -= 1

            if self.reserve0:
                assert self.seg_first != 0
                if self.seg_first != 0x80:  # no end here
                    self.reserve0 = False
                    self.block_left = self.block_size
                    continue
            elif self.seg_first:  # no end here
                self.block_left = self.block_size
                continue
            break

        self.seg_size = 0
        self.reserve0 = False
        self.block_left = 0


class FOBitInputStream:
    def __init__(self, stream, block_size=1):
        self.stream = stream
        self.block_size = max(1, block_size)
        self.reserve0 = False
        self.block_left = 0
        self.in_done = False

    def read(self):
        if self.in_done:
            in_byte = 0
        else:
            data = self.stream.read(1)
            if not data:
                self.in_done = True
                in_byte = 0
            else:
                in_byte = data[0] ^ 0x55  # XOR with 55 for de-obfuscation

        if self.block_left:
            self.reserve0 = self.reserve0 and not in_byte
            self.block_left -= 1
            return in_byte

        if self.in_done:
            if self.reserve0:
                self.reserve0 = False
                return 0x80  # End marker
            return -1

        if self.reserve0:
            self.reserve0 = not (in_byte & 0x7F)
        else:
            self.reserve0 = not in_byte
        self.block_left = self.block_size - 1
        return in_byte


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
