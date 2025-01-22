import io
import os
import time

from arithmetic_decoder import ArithmeticDecoder
from arithmetic_encoder import ArithmeticEncoder
from cryptography.hazmat.primitives.ciphers.aead import AESGCMSIV
from cryptography.hazmat.primitives.kdf.argon2 import Argon2id
from foio import FOBitInputStream, FOBitOutputStream
from llm_model import LLMModel
from transformers import AutoModelForCausalLM, AutoTokenizer

_MODEL = "meta-llama/Llama-3.2-1B-Instruct"
_PROMPT = [
    {
        "role": "system",
        "content": "You are a typical Twitter user. Respond with your tweet.",
    },
]


def compress(input_stream, output_stream, block_size=1):
    tokenizer = AutoTokenizer.from_pretrained(_MODEL)
    model = AutoModelForCausalLM.from_pretrained(
        _MODEL, torch_dtype="auto", device_map="auto"
    )
    initial_input = tokenizer.apply_chat_template(
        _PROMPT, add_generation_prompt=True, return_tensors="pt"
    ).to(model.device)
    model = LLMModel(model, initial_input, list(tokenizer.added_tokens_decoder.keys()))
    outbits = FOBitOutputStream(output_stream, block_size)
    encoder = ArithmeticEncoder(outbits)

    input_tokens = tokenizer(input_stream)
    input_toks_processed = []
    for i, input_token in enumerate(input_tokens.input_ids):
        if i == 0 and input_token == tokenizer.bos_token_id:
            print(f"{time.time()}: skipping bos token")
            continue

        print(
            f"{time.time()}: encoding token {input_token} ({repr(tokenizer.decode([input_token]))}, #{i + 1} of {len(input_tokens.input_ids)})"
        )
        encoder.encode(model, input_token, True)
        print(
            f"{time.time()}: encoded token {input_token} ({repr(tokenizer.decode([input_token]))})"
        )

        input_toks_processed.append(input_token)
        print(
            f"{time.time()}: input processed so far: {repr(tokenizer.decode(input_toks_processed))}"
        )
        model.update(input_token)
        print(f"{time.time()}: updated model")

    encoder.end()
    outbits.end()

    return input_toks_processed


def decompress(input_stream, block_size=1):
    tokenizer = AutoTokenizer.from_pretrained(_MODEL)
    model = AutoModelForCausalLM.from_pretrained(
        _MODEL, torch_dtype="auto", device_map="auto"
    )
    initial_input = tokenizer.apply_chat_template(
        _PROMPT, add_generation_prompt=True, return_tensors="pt"
    ).to(model.device)
    model = LLMModel(model, initial_input, list(tokenizer.added_tokens_decoder.keys()))
    decoder = ArithmeticDecoder(FOBitInputStream(input_stream, block_size))

    output_toks = []
    i = 0
    while True:
        print(f"{time.time()}: decoding token #{i}")
        i += 1
        sym = decoder.decode(model, True)
        print(f"{time.time()}: decoded token {sym}")
        if sym < 0:
            break
        print(f"{time.time()}: token {sym} has text {repr(tokenizer.decode([sym]))}")

        print(
            f"{time.time()}: ingested {input_stream.tell()} bytes of {len(input_stream.getvalue())}"
        )

        output_toks.append(sym)
        print(
            f"{time.time()}: complete output so far: {repr(tokenizer.decode(output_toks))}"
        )
        model.update(sym)
        print(f"{time.time()}: updated model")

    return output_toks, tokenizer.decode(output_toks)


def main():
    print(f"{time.time()}: started script")

    password = b"password"
    plaintext = b"hello world"
    associated_data = None
    salt = os.urandom(16)
    nonce = salt[:12]

    kdf = Argon2id(salt=salt, length=32, iterations=1, lanes=4, memory_cost=64 * 1024)
    key = kdf.derive(password)

    aesgcmsiv = AESGCMSIV(key)
    ct = aesgcmsiv.encrypt(nonce, plaintext, associated_data)
    input_bytes = salt + ct

    print(f"input_bytes=0x{input_bytes.hex()}")

    output_toks, encoded = decompress(io.BytesIO(input_bytes))

    print(f"input_bytes=0x{input_bytes.hex()}")
    print(f"{output_toks=}")

    print(f"{time.time()}: decompressed input")

    print(f"{encoded=}")

    output_stream = io.BytesIO()
    input_toks_processed = compress(encoded, output_stream)

    print(f"{time.time()}: compressed output")

    output_bytes = output_stream.getvalue()

    print(f"{encoded=}")
    print(f" input_bytes=0x{input_bytes.hex()}")
    print(f"output_bytes=0x{output_bytes.hex()}")
    print(f"         {output_toks=}")
    print(f"{input_toks_processed=}")

    salt = output_bytes[:16]
    kdf = Argon2id(salt=salt, length=32, iterations=1, lanes=4, memory_cost=64 * 1024)
    key = kdf.derive(password)

    nonce = output_bytes[:12]
    aesgcmsiv = AESGCMSIV(key)

    ct = output_bytes[16:]
    decoded_bytes = aesgcmsiv.decrypt(nonce, ct, associated_data)

    decoded = decoded_bytes.decode("utf-8")

    print(f"{decoded=}")


if __name__ == "__main__":
    main()
