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
