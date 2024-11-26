import sys
import argparse
from io import BytesIO
from arithmetic_encoder import ArithmeticEncoder
from arithmetic_decoder import ArithmeticDecoder
from simple_model import SimpleAdaptiveModel


class FOBitInputStream:
    def __init__(self, stream, block_size=1):
        self.stream = stream
        self.block_size = max(1, block_size)
        self.reserve0 = False
        self.block_left = 0
        self.in_done = False

    def read(self, size=1):
        if size != 1:
            raise ValueError("This stream only supports reading 1 byte at a time")

        while True:
            if self.in_done:
                return b"\x00"

            byte = self.stream.read(1)
            if not byte:
                self.in_done = True
                return b"\x00"

            byte = byte[0] ^ 0x55  # XOR with 55 for de-obfuscation

            if self.block_left:
                self.reserve0 = self.reserve0 and not byte
                self.block_left -= 1
                return bytes([byte])

            # Start of a new block
            if self.in_done:
                if self.reserve0:
                    return b"\x80"  # End marker
                break

            self.reserve0 = not byte if not self.reserve0 else not (byte & 0x7F)
            self.block_left = self.block_size - 1
            return bytes([byte])


def compress(input_stream, output_stream, block_size=1):
    model = SimpleAdaptiveModel(256)
    encoder = ArithmeticEncoder(FOBitOutputStream(output_stream, block_size))

    while True:
        byte = input_stream.read(1)
        if not byte:
            break

        sym = byte[0]
        encoder.encode(model, sym, True)
        model.update(sym)

    encoder.end()


def decompress(input_stream, output_stream, block_size=1):
    model = SimpleAdaptiveModel(256)
    decoder = ArithmeticDecoder(FOBitInputStream(input_stream, block_size))

    while True:
        sym = decoder.decode(model, True)
        if sym < 0:
            break

        output_stream.write(bytes([sym]))
        model.update(sym)


# Rest of the script remains the same as in the previous version
