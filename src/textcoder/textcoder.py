import io
import os

from cryptography.hazmat.primitives.ciphers.aead import AESGCMSIV
from cryptography.hazmat.primitives.kdf.argon2 import Argon2id
from transformers import AutoModelForCausalLM, AutoTokenizer

from textcoder.arithmetic_decoder import ArithmeticDecoder
from textcoder.arithmetic_encoder import ArithmeticEncoder
from textcoder.foio import FOBitInputStream, FOBitOutputStream
from textcoder.llm_model import LLMModel

_MODEL = "meta-llama/Llama-3.2-1B-Instruct"
_PROMPT = [
    {
        "role": "system",
        "content": "You are a typical Twitter user. Respond with your tweet.",
    },
]


def compress(input_string, output_stream, block_size=1):
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

    input_tokens = tokenizer(input_string)
    for i, input_token in enumerate(input_tokens.input_ids):
        if i == 0 and input_token == tokenizer.bos_token_id:
            continue
        encoder.encode(model, input_token, True)
        model.update(input_token)
        
    encoder.end()
    outbits.end()


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
    while True:
        sym = decoder.decode(model, True)
        if sym < 0:
            break
        output_toks.append(sym)
        model.update(sym)

    return tokenizer.decode(output_toks)


def main():
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

    encoded = decompress(io.BytesIO(input_bytes))

    output_stream = io.BytesIO()
    
    compress(encoded, output_stream)

    output_bytes = output_stream.getvalue()
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
