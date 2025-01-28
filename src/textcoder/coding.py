import typing

from transformers import AutoModelForCausalLM, AutoTokenizer

from textcoder.arithmetic_decoder import ArithmeticDecoder
from textcoder.arithmetic_encoder import ArithmeticEncoder
from textcoder.foio import FOBitInputStream, FOBitOutputStream
from textcoder.model import LLMArithmeticModel

_HF_MODEL_NAME = "meta-llama/Llama-3.2-1B-Instruct"
_INITIAL_CONVERSATION = [
    {
        "role": "system",
        "content": "You are a typical Twitter user. Respond with your tweet.",
    },
]


class LLMArithmeticCoder:
    def __init__(self, accelerated: bool = True, block_size: int = 1):
        self._accelerated = accelerated
        self._block_size = block_size

    def _get_tokenizer_and_arithmetic_model(self):
        tokenizer = AutoTokenizer.from_pretrained(_HF_MODEL_NAME)
        hf_model_options = {"device_map": "auto"} if self._accelerated else {}
        hf_model = AutoModelForCausalLM.from_pretrained(
            _HF_MODEL_NAME, torch_dtype="auto", **hf_model_options
        )
        initial_input = tokenizer.apply_chat_template(
            _INITIAL_CONVERSATION,
            add_generation_prompt=True,
            return_tensors="pt",
            return_dict=True,
        ).to(hf_model.device)  # type: ignore
        arithmetic_model = LLMArithmeticModel(
            hf_model,
            initial_input,  # type: ignore
            list(tokenizer.added_tokens_decoder.keys()),
        )
        return tokenizer, arithmetic_model

    def encode(
        self,
        input_string: str,
        output_stream: typing.BinaryIO,
    ):
        tokenizer, arithmetic_model = self._get_tokenizer_and_arithmetic_model()
        outbits = FOBitOutputStream(output_stream, self._block_size)
        encoder = ArithmeticEncoder(outbits)

        input_tokens = tokenizer(input_string)
        for i, input_token in enumerate(input_tokens.input_ids):
            if i == 0 and input_token == tokenizer.bos_token_id:
                # skip bos token
                continue
            encoder.encode(arithmetic_model, input_token, True)
            arithmetic_model.update(input_token)

        encoder.end()
        outbits.end()

    def decode(self, input_stream: typing.BinaryIO) -> str:
        tokenizer, arithmetic_model = self._get_tokenizer_and_arithmetic_model()
        decoder = ArithmeticDecoder(FOBitInputStream(input_stream, self._block_size))

        output_toks = []
        while True:
            sym = decoder.decode(arithmetic_model, True)
            if sym < 0:
                break
            output_toks.append(sym)
            arithmetic_model.update(sym)

        return tokenizer.decode(output_toks)
