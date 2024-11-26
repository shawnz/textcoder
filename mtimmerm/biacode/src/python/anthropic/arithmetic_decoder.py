class ArithmeticDecoder:
    BIT16 = 65536
    MASK16 = 0xFFFF

    def __init__(self, input_stream):
        self.bytesin = input_stream
        self.low = 0
        self.range = self.BIT16
        self.interval_bits = 16
        self.free_end_even = self.MASK16
        self.next_free_end = 0
        self.value = 0
        self.value_shift = -24
        self.follow_byte = 0
        self.follow_buf = 1

    def decode(self, model, can_end=False):
        # Refill value buffer
        while self.value_shift <= 0:
            self.value <<= 8
            self.value_shift += 8

            if not self.follow_buf:
                self.value |= self.follow_byte

                # Refill follow buffer
                try:
                    self.follow_byte = self.bytesin.read(1)[0]
                    self.follow_buf = 1
                except (IndexError, TypeError):
                    self.follow_buf = -1
                    self.follow_byte = 0

        # Check for end of stream
        if can_end:
            if self.follow_buf < 0 and (
                ((self.next_free_end - self.low) << self.value_shift) == self.value
            ):
                return -1  # EOF

            if self.next_free_end:
                self.next_free_end += (self.free_end_even + 1) * 2
            else:
                self.next_free_end = self.free_end_even + 1

        # Decode symbol
        newl = (
            (self.value >> self.value_shift) * model.prob_one() + model.prob_one() - 1
        ) // self.range

        symbol, sym_low, sym_high = model.get_symbol(newl)

        newl = sym_low * self.range // model.prob_one()
        newh = sym_high * self.range // model.prob_one()

        self.range = newh - newl
        self.value -= newl << self.value_shift
        self.low += newl

        # Adjust nextfreeend
        if self.next_free_end < self.low:
            self.next_free_end = (
                (self.low + self.free_end_even) & ~self.free_end_even
            ) | (self.free_end_even + 1)

        # Range scaling
        if self.range <= (self.BIT16 >> 1):
            self.low *= 2
            self.range *= 2
            self.next_free_end *= 2
            self.free_end_even = self.free_end_even * 2 + 1
            self.value_shift -= 1

            while self.next_free_end - self.low >= self.range:
                self.free_end_even //= 2
                self.next_free_end = (
                    (self.low + self.free_end_even) & ~self.free_end_even
                ) | (self.free_end_even + 1)

            while self.interval_bits < 24:
                if self.range > (self.BIT16 >> 1):
                    break

                self.low *= 2
                self.range *= 2
                self.next_free_end *= 2
                self.free_end_even = self.free_end_even * 2 + 1
                self.value_shift -= 1
                self.interval_bits += 8

        return symbol
