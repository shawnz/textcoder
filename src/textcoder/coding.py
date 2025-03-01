import io
import logging

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
_DEFAULT_CHAT_TEMPLATE = (
    "{% for message in messages %}"
    "{{message.content}}"
    "{% if not loop.last %} {% endif %}"
    "{% endfor %}"
)


_logger = logging.getLogger(__name__)


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
        chat_template = (
            _DEFAULT_CHAT_TEMPLATE if tokenizer.chat_template is None else None
        )
        initial_input = tokenizer.apply_chat_template(
            _INITIAL_CONVERSATION,
            chat_template=chat_template,
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
        output_stream: io.BytesIO,
    ):
        tokenizer, arithmetic_model = self._get_tokenizer_and_arithmetic_model()
        outbits = FOBitOutputStream(output_stream, self._block_size)
        encoder = ArithmeticEncoder(outbits)

        input_tokens = tokenizer(input_string)
        tokens_so_far = []
        for i, input_token in enumerate(input_tokens.input_ids):
            if i == 0 and input_token == tokenizer.bos_token_id:
                # skip bos token
                continue
            if _logger.isEnabledFor(logging.INFO):
                percent_completed = 100 * i / len(input_tokens.input_ids)
                _logger.info(
                    f"encoding token {i + 1} of {len(input_tokens.input_ids)} "
                    f"({percent_completed:.2f}% completed)"
                )
                _logger.info(f"token id: {input_token}")
                token_str = tokenizer.decode(input_token)
                _logger.info(f"token str: {repr(token_str)}")
                tokens_so_far.append(input_token)
                _logger.info(f"encoded ids so far: {tokens_so_far}")
                encoded_str_so_far = tokenizer.decode(tokens_so_far)
                _logger.info(f"encoded str so far: {repr(encoded_str_so_far)}")
            encoder.encode(arithmetic_model, input_token, True)
            arithmetic_model.update(input_token)

        encoder.end()
        outbits.end()

    def decode(self, input_stream: io.BytesIO) -> str:
        tokenizer, arithmetic_model = self._get_tokenizer_and_arithmetic_model()
        decoder = ArithmeticDecoder(FOBitInputStream(input_stream, self._block_size))

        if _logger.isEnabledFor(logging.INFO):
            total_bytes = len(input_stream.getvalue())

        output_toks = []
        i = 0
        while True:
            if _logger.isEnabledFor(logging.INFO):
                bytes_decoded = input_stream.tell()
                percent_completed = 100 * bytes_decoded / total_bytes
                _logger.info(
                    f"decoding token {i + 1} (at byte {bytes_decoded} of {total_bytes})"
                    f"({percent_completed:.2f}% completed)"
                )
            sym = decoder.decode(arithmetic_model, True)
            _logger.info(f"token id: {sym}")
            if sym < 0:
                break
            if _logger.isEnabledFor(logging.INFO):
                token_str = tokenizer.decode(sym)
                _logger.info(f"token str: {repr(token_str)}")
            output_toks.append(sym)
            if _logger.isEnabledFor(logging.INFO):
                _logger.info(f"decoded ids so far: {output_toks}")
                decoded_str_so_far = tokenizer.decode(output_toks)
                _logger.info(f"decoded str so far: {repr(decoded_str_so_far)}")
            arithmetic_model.update(sym)
            i += 1

        return tokenizer.decode(output_toks)
