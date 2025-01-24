import torch
import transformers

_TARGET_PROB_ONE = 32768
_TOP_K = 100


class LLMArithmeticModel:
    def __init__(
        self,
        hf_model: transformers.PreTrainedModel,
        initial_input: list[int],
        special_token_ids: list[int],
    ):
        self._model = hf_model
        self._special_token_ids = special_token_ids
        self._past_key_values = transformers.DynamicCache()
        self._logits = hf_model(
            input_ids=initial_input, past_key_values=self._past_key_values
        ).logits[0, -1]
        self._probs_dirty = True
        self._sorted_tok_ids_dirty = True

    def _recompute_probs(self):
        # filter out special tokens
        filtered_logits = self._logits
        filtered_logits[self._special_token_ids] = float("-inf")

        # convert logits to probabilities with softmax, sort probabilities to
        # allow for efficient searching, and optionally apply top K sampling
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

        # fit probabilities into _TARGET_PROB_ONE integer buckets with the
        # following approach:
        #
        # 1. multiply probabilities by _TARGET_PROB_ONE
        # 2. round up result and cast to integer
        # 3. calculate cumulative sum and truncate once _TARGET_PROB_ONE is
        #    exceeded
        scaled_int_probs = torch.ceil(sorted_decimal_probs * _TARGET_PROB_ONE).int()
        cumsum_probs = torch.cumsum(scaled_int_probs, dim=0)
        truncation_point = torch.searchsorted(cumsum_probs, _TARGET_PROB_ONE)
        self._probs = cumsum_probs[: truncation_point + 1]
        self._prob_tok_ids = sorted_decimal_prob_indices[: truncation_point + 1]
        self._probs_dirty = False

    def _recompute_sorted_tok_ids(self):
        if self._probs_dirty:
            self._recompute_probs()

        # sort token IDs for efficient searching when encoding
        self._sorted_tok_ids, self._sorted_tok_id_indices = torch.sort(
            self._prob_tok_ids
        )
        self._sorted_tok_ids_dirty = False

    def update(self, token_id: int):
        self._logits = self._model(
            input_ids=torch.tensor([[token_id]], device=self._model.device),
            past_key_values=self._past_key_values,
        ).logits[0, -1]
        self._probs_dirty = True
        self._sorted_tok_ids_dirty = True

    def get_sym_range(self, token_id: int) -> tuple[int, int]:
        if self._sorted_tok_ids_dirty:
            self._recompute_sorted_tok_ids()
        sorted_idx = torch.searchsorted(self._sorted_tok_ids, token_id)
        if (
            sorted_idx >= len(self._sorted_tok_ids)
            or self._sorted_tok_ids[sorted_idx] != token_id
        ):
            # token_id is not present in tok_ids: this could be because it was
            # eliminated during top-K sampling or truncation, or it's a
            # special token, etc.
            #
            # this can happen when the tokenizer tokenizes a string
            # differently than how we encoded it.
            raise ValueError(f"token id {token_id} not permitted at this time")
        idx = self._sorted_tok_id_indices[sorted_idx]
        low = self._probs[idx - 1].item() if idx > 0 else 0
        high = self._probs[idx].item()
        return low, high # type: ignore

    def get_symbol(self, p: int) -> tuple[int, int, int]:
        if self._probs_dirty:
            self._recompute_probs()
        idx = torch.searchsorted(self._probs, p + 1)
        token_id = self._prob_tok_ids[idx].item()
        low = self._probs[idx - 1].item() if idx > 0 else 0
        high = self._probs[idx].item()
        return token_id, low, high # type: ignore

    def prob_one(self) -> int:
        return _TARGET_PROB_ONE