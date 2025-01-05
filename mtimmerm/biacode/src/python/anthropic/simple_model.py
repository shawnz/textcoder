class SimpleAdaptiveModel:
    def __init__(self, num_symbols):
        # Round num_symbols to next power of 2
        self.sym_zero_index = 1
        while self.sym_zero_index < num_symbols:
            self.sym_zero_index *= 2

        # Probability heap is twice the size
        self.prob_heap = [0] * (self.sym_zero_index * 2)
        self.prob1 = 0

        # Initial probabilities
        for i in range(num_symbols):
            self.add_p(i, 1)

        # Circular window for tracking recent symbols
        self.window = [-1] * 4096
        self.indices = [1024, 2048, 3072, 0]

    def update(self, symbol):
        # Circular buffer navigation and probability updates
        weights = [2, 1, 1, 2]

        for i in range(len(self.indices)):
            self.indices[i] = (self.indices[i] - 1) % 4096
            if self.window[self.indices[i]] >= 0:
                self.sub_p(self.window[self.indices[i]], weights[i])

        self.window[self.indices[-1]] = symbol
        self.add_p(symbol, 6)

    def reset(self):
        # Reset probabilities and window
        weights = [6, 4, 3, 2]

        for i in range(len(self.indices)):
            j = (i + len(self.indices) - 1) % len(self.indices)
            curr = self.indices[j]
            while curr != self.indices[i]:
                if self.window[curr] < 0:
                    break
                self.sub_p(self.window[curr], weights[i])
                self.window[curr] = -1
                curr = (curr + 1) % 4096

    def get_sym_range(self, symbol):
        # Get probability range for a symbol
        low = 0
        bit = self.sym_zero_index
        i = 1

        while i < self.sym_zero_index:
            bit //= 2
            i *= 2
            if symbol & bit:
                low += self.prob_heap[i]
                i += 1

        return low, low + self.prob_heap[i]

    def get_symbol(self, p):
        # Get symbol for a given probability
        low = 0
        i = 1

        while i < self.sym_zero_index:
            i *= 2
            if (p - low) >= self.prob_heap[i]:
                low += self.prob_heap[i]
                i += 1

        return i - self.sym_zero_index, low, low + self.prob_heap[i]

    def add_p(self, sym, n):
        # Add probability for a symbol
        sym += self.sym_zero_index
        while sym:
            self.prob_heap[sym] += n
            sym //= 2
        self.prob1 = self.prob_heap[1]

    def sub_p(self, sym, n):
        # Subtract probability for a symbol
        sym += self.sym_zero_index
        while sym:
            self.prob_heap[sym] -= n
            sym //= 2
        self.prob1 = self.prob_heap[1]

    def prob_one(self):
        return self.prob1
