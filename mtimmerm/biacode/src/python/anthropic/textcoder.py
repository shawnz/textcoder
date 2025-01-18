import io

from arithmetic_decoder import ArithmeticDecoder
from arithmetic_encoder import ArithmeticEncoder
from foio import FOBitInputStream, FOBitOutputStream
from llm_model import LLMModel
from transformers import AutoModelForCausalLM, AutoTokenizer

_PROMPT = "You are a Twitter user, about to post a tweet. What does the tweet say? Please respond with just the tweet contents with no quotes."


def compress(input_stream, output_stream, block_size=1):
    tokenizer = AutoTokenizer.from_pretrained("gpt2")
    model = AutoModelForCausalLM.from_pretrained("gpt2")
    initial_input = tokenizer(_PROMPT, return_tensors="pt")
    model = LLMModel(model, initial_input)
    outbits = FOBitOutputStream(output_stream, block_size)
    encoder = ArithmeticEncoder(outbits)

    input_tokens = tokenizer(input_stream)
    for input_token in input_tokens.input_ids:
        encoder.encode(model, input_token, True)
        model.update(input_token)

    encoder.end()
    outbits.end()


def decompress(input_stream, block_size=1):
    tokenizer = AutoTokenizer.from_pretrained("gpt2")
    model = AutoModelForCausalLM.from_pretrained("gpt2")
    initial_input = tokenizer(_PROMPT, return_tensors="pt")
    model = LLMModel(model, initial_input)
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

    encoded = decompress(io.BytesIO(b"what if the string is really long? hello goodbye"))

    print(f"{encoded=}")

    output_stream = io.BytesIO()
    compress(encoded, output_stream)

    decoded_bytes = output_stream.getvalue()
    
    print(f"{decoded_bytes=}")
    
    decoded = decoded_bytes.decode("utf-8")

    print (f"{decoded=}")



if __name__ == "__main__":
    main()
