from textcoder.foio import FOBitOutputStream
from textcoder.model import LLMArithmeticModel


class ArithmeticEncoder:
    BIT16 = 65536
    MASK16 = 0xFFFF

    def __init__(self, output_stream: FOBitOutputStream):
        self.bytesout = output_stream
        self.low = 0
        self.range = self.BIT16
        self.interval_bits = 16
        self.free_end_even = self.MASK16
        self.next_free_end = 0
        self.carry_byte = 0
        self.carry_buf = 0

    def encode(self, model: LLMArithmeticModel, symbol, could_have_ended=False):
        if could_have_ended:
            if self.next_free_end:
                self.next_free_end += (self.free_end_even + 1) * 2
            else:
                self.next_free_end = self.free_end_even + 1

        new_low, new_high = model.get_sym_range(symbol)
        new_low = new_low * self.range // model.prob_one()
        new_high = new_high * self.range // model.prob_one()
        self.range = new_high - new_low
        self.low += new_low

        if self.next_free_end < self.low:
            self.next_free_end = (
                (self.low + self.free_end_even) & ~self.free_end_even
            ) | (self.free_end_even + 1)

        if self.range <= (self.BIT16 >> 1):
            self.low *= 2
            self.range *= 2
            self.next_free_end *= 2
            self.free_end_even = self.free_end_even * 2 + 1

            while self.next_free_end - self.low >= self.range:
                self.free_end_even //= 2
                self.next_free_end = (
                    (self.low + self.free_end_even) & ~self.free_end_even
                ) | (self.free_end_even + 1)

            while True:
                self.interval_bits += 1
                if self.interval_bits == 24:
                    # need to output a byte
                    # adjust and output
                    low_decrement = self.low & ~self.MASK16
                    self.low -= low_decrement
                    self.next_free_end -= low_decrement

                    # there can only be one number this even in the range.
                    # nextfreeend is in the range
                    # if nextfreeend is this even, next step must reduce evenness
                    self.free_end_even &= self.MASK16

                    self._byte_with_carry(low_decrement >> 16)
                    self.interval_bits -= 8

                if self.range > (self.BIT16 >> 1):
                    break

                self.low *= 2
                self.range *= 2
                self.next_free_end *= 2
                self.free_end_even = self.free_end_even * 2 + 1
        else:
            while self.next_free_end - self.low > self.range:
                self.free_end_even //= 2
                self.next_free_end = (
                    (self.low + self.free_end_even) & ~self.free_end_even
                ) | (self.free_end_even + 1)

    def _byte_with_carry(self, out_byte):
        if self.carry_buf:
            if out_byte >= 256:
                self.bytesout.write(self.carry_byte + 1)
                for _ in range(self.carry_buf - 1):
                    self.bytesout.write(0x00)
                self.carry_buf = 0
                self.carry_byte = out_byte & 0xFF
            elif out_byte < 255:
                self.bytesout.write(self.carry_byte)
                for _ in range(self.carry_buf - 1):
                    self.bytesout.write(0xFF)
                self.carry_buf = 0
                self.carry_byte = out_byte & 0xFF
        else:
            self.carry_byte = out_byte & 0xFF

        self.carry_buf += 1

    def end(self):
        self.next_free_end <<= 24 - self.interval_bits

        while self.next_free_end:
            self._byte_with_carry(self.next_free_end >> 16)
            self.next_free_end = (self.next_free_end & self.MASK16) << 8

        if self.carry_buf:
            self._byte_with_carry(0)

        # Reset for potential future use
        self.low = 0
        self.range = self.BIT16
        self.interval_bits = 16
        self.free_end_even = self.MASK16
        self.next_free_end = 0
        self.carry_byte = 0
        self.carry_buf = 0
