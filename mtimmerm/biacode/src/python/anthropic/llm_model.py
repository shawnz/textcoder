import torch

_TARGET_PROB_ONE = 32768


class LLMModel:
    def __init__(self, model, initial_input):
        self._model = model
        self._input = initial_input
        self._probs_dirty = True
        self._sorted_tok_ids_dirty = True

    @staticmethod
    def _append_input_tensor(tensor, value):
        tail = torch.tensor([[value]], dtype=tensor.dtype, device=tensor.device)
        return torch.cat((tensor, tail), dim=1)

    def _recompute_probs(self):
        logits = self._model(**self._input).logits[0, -1]
        sorted_decimal_probs, sorted_decimal_prob_indices = torch.sort(
            torch.softmax(logits, dim=0), descending=True
        )
        scaled_int_probs = torch.ceil(sorted_decimal_probs * _TARGET_PROB_ONE).int()
        cumsum_probs = torch.cumsum(scaled_int_probs, dim=0)
        truncation_point = torch.searchsorted(cumsum_probs, _TARGET_PROB_ONE)
        self._probs = cumsum_probs[: truncation_point + 1]
        self._prob_tok_ids = sorted_decimal_prob_indices[: truncation_point + 1]
        self._probs_dirty = False

    def _recompute_sorted_tok_ids(self):
        if self._probs_dirty:
            self._recompute_probs()
        self._sorted_tok_ids, self._sorted_tok_id_indices = torch.sort(
            self._prob_tok_ids
        )
        self._sorted_tok_ids_dirty = False

    def update(self, token):
        old_input_ids = self._input["input_ids"]
        old_attention_mask = self._input["attention_mask"]
        self._input = {
            "input_ids": self._append_input_tensor(old_input_ids, token),
            "attention_mask": self._append_input_tensor(old_attention_mask, 1),
        }
        self._probs_dirty = True
        self._sorted_tok_ids_dirty = True

    def get_sym_range(self, symbol):
        if self._sorted_tok_ids_dirty:
            self._recompute_sorted_tok_ids()
        idx = self._sorted_tok_id_indices[
            torch.searchsorted(self._sorted_tok_ids, symbol)
        ]
        low = self._probs[idx - 1].item() if idx > 0 else 0
        high = self._probs[idx].item()
        return low, high

    def get_symbol(self, p):
        if self._probs_dirty:
            self._recompute_probs()
        idx = torch.searchsorted(self._probs, p + 1)
        symbol = self._prob_tok_ids[idx].item()
        low = self._probs[idx - 1].item() if idx > 0 else 0
        high = self._probs[idx].item()
        return symbol, low, high

    def prob_one(self):
        return _TARGET_PROB_ONE
