import sys
import argparse
from io import BytesIO
from arithmetic_encoder import ArithmeticEncoder
from simple_model import SimpleAdaptiveModel

class FOBitStream:
    def __init__(self, stream, block_size=1):
        self.stream = stream
        self.block_size = max(1, block_size)
        self.reserve0 = False
        self.block_left = 0
        
    def write(self, data):
        for byte in data:
            byte = byte ^ 0x55  # XOR with 55 for obfuscation
            
            if not self.block_left:
                self.reserve0 = not byte if not self.reserve0 else not(byte & 0x7F)
                self.block_left = self.block_size - 1
            else:
                self.block_left -= 1
            
            self.stream.write(bytes([byte]))
        
    def end(self):
        # Pad as needed
        while self.block_left:
            self.stream.write(b'\x55')
            self.block_left -= 1

def compress(input_stream, output_stream, block_size=1):
    model = SimpleAdaptiveModel(256)
    encoder = ArithmeticEncoder(FOBitStream(output_stream, block_size))
    
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
    # For this version, we'll use BytesIO as a bit stream simulator
    # In a real implementation, you'd create a more robust bit input stream
    
    while True:
        sym = input_stream.read(1)
        if not sym:
            break
        
        sym = sym[0]
        output_stream.write(bytes([sym]))
        model.update(sym)

def main():
    parser = argparse.ArgumentParser(description='Bijective Arithmetic Encoder/Decoder')
    parser.add_argument('input_file', help='Input file to compress/decompress')
    parser.add_argument('output_file', help='Output file')
    parser.add_argument('-d', '--decompress', action='store_true', help='Decompress mode')
    parser.add_argument('-b', '--block_size', type=int, default=1, help='Compressed block size')
    
    args = parser.parse_args()
    
    try:
        with open(args.input_file, 'rb') as infile, open(args.output_file, 'wb') as outfile:
            if args.decompress:
                decompress(infile, outfile, args.block_size)
            else:
                compress(infile, outfile, args.block_size)
    except IOError as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
