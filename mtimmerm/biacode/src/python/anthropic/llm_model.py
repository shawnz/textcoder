import time

import torch
import transformers

_TARGET_PROB_ONE = 32768
_TOP_K = 40


class LLMModel:
    def __init__(self, model, initial_input, special_token_ids):
        self._model = model
        self._special_token_ids = special_token_ids
        self._past_key_values = transformers.DynamicCache()
        # self._past_key_values = transformers.StaticCache(
        #     config=model.config,
        #     batch_size=1,
        #     # If you plan to reuse the cache, make sure the cache length is large enough for all cases
        #     max_cache_len=(len(initial_input.input_ids) + 1000),
        #     device=model.device,
        #     dtype=model.dtype,
        # )
        print(f"{time.time()}: started initial forward pass")
        self._logits = model(
            input_ids=initial_input, past_key_values=self._past_key_values
        ).logits[0, -1]
        print(f"{time.time()}: done initial forward pass")
        self._probs_dirty = True
        self._sorted_tok_ids_dirty = True

    @staticmethod
    def _append_input_tensor(tensor, value):
        tail = torch.tensor([[value]], dtype=tensor.dtype, device=tensor.device)
        return torch.cat((tensor, tail), dim=1)

    def _recompute_probs(self):
        print(f"{time.time()}: started recomputing probs")
        # logits = self._model(
        #     **self._input, past_key_values=self._past_key_values
        # ).logits[0, -1]
        # print(f"{time.time()}: ran the model")
        filtered_logits = self._logits
        filtered_logits[self._special_token_ids] = float("-inf")
        if _TOP_K == 0:
            sorted_decimal_probs, sorted_decimal_prob_indices = torch.sort(
                torch.softmax(filtered_logits, dim=0), descending=True
            )
        else:
            topk_logits, topk_logit_indices = torch.topk(filtered_logits, _TOP_K)
            sorted_decimal_probs, sorted_decimal_prob_indices = (
                torch.softmax(topk_logits, dim=0),
                topk_logit_indices,
            )
        print(f"{time.time()}: sorted softmax logits")
        scaled_int_probs = torch.ceil(sorted_decimal_probs * _TARGET_PROB_ONE).int()
        print(f"{time.time()}: scaled and rounded probs to ints")
        cumsum_probs = torch.cumsum(scaled_int_probs, dim=0)
        print(f"{time.time()}: computed cumsum probs")
        truncation_point = torch.searchsorted(cumsum_probs, _TARGET_PROB_ONE)
        self._probs = cumsum_probs[: truncation_point + 1]
        self._prob_tok_ids = sorted_decimal_prob_indices[: truncation_point + 1]
        self._probs_dirty = False
        print(f"{time.time()}: done recomputing probs")

    def _recompute_sorted_tok_ids(self):
        print(f"{time.time()}: started recomputing sorted tok ids")
        if self._probs_dirty:
            self._recompute_probs()
        self._sorted_tok_ids, self._sorted_tok_id_indices = torch.sort(
            self._prob_tok_ids
        )
        self._sorted_tok_ids_dirty = False
        print(f"{time.time()}: done recomputing sorted tok ids")

    def update(self, token):
        print(f"{time.time()}: started update")
        # print(f"{time.time()}: started appending token")
        # old_input_ids = self._input["input_ids"]
        # old_attention_mask = self._input["attention_mask"]
        # self._input = {
        #     "input_ids": self._append_input_tensor(old_input_ids, token),
        #     "attention_mask": self._append_input_tensor(old_attention_mask, 1),
        # }
        self._logits = self._model(
            input_ids=torch.tensor([[token]], device=self._model.device),
            past_key_values=self._past_key_values,
        ).logits[0, -1]
        self._probs_dirty = True
        self._sorted_tok_ids_dirty = True
        # print(f"{time.time()}: done appending token")
        print(f"{time.time()}: done update")

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
