# Textcoder

Textcoder is a proof-of-concept tool for steganographically encoding secret messages such that they appear as ordinary, unrelated text.

It works by taking the secret message, encrypting it to produce a pseudorandom bit stream, and then using arithmetic coding to decompress that bit stream based on a statistical model derived from an LLM. This produces text which appears to be sampled randomly from the LLM, while actually encoding the secret message in the specific token choices.

For example, the secret message `hello, world!` could be encoded into the following text:

```
% echo 'hello, world!' | textcoder -p 'foobar' > encoded.txt
% cat encoded.txt
"Goodbye 2024! Can't wait to start fresh with a brand new year and a new chance to slay the game in 2025 #NewYearNewMe #ConfidenceIsKey" - @SlayMyGameLife7770 (280 character limit) I just ordered my fave coffee from Dunkin' yesterday but I almost spilled it on my shirt, oh no! #DunkinCoffeePlease #FashionBlunders Life is just trying to keep up with its favorite gamers rn. Wish I could say I'm coding instead of gaming, but when i have to put down my controller for a sec
```

A user with knowledge of the password could then decode the message as follows:

```
% cat encoded.txt | textcoder -d -p 'foobar'
hello, world!
```

## Running the Project

Textcoder is packaged using [Poetry](https://python-poetry.org/). To run the project, Poetry must be installed. After installing Poetry, clone the repository and execute the `poetry install` command:

```bash
git clone https://github.com/shawnz/textcoder.git
cd textcoder
poetry install
```

Additionally, Textcoder makes use of the [Llama 3.2 1B Instruct](https://huggingface.co/meta-llama/Llama-3.2-1B-Instruct) language model. The use of this model requires approval of the Llama 3.2 community license agreement. After approving the license, install the [Hugging Face Hub command line interface](https://huggingface.co/docs/huggingface_hub/main/en/guides/cli) and log in to your Hugging Face account:

```bash
huggingface-cli login
```

Finally, you can now run Textcoder using the `textcoder` command. To encode a message, run:

```bash
echo '<message>' | poetry run textcoder -p '<password>' > encoded.txt
```

To decode a message, run:

```bash
cat encoded.txt | poetry run textcoder -d -p '<password>'
```

## Known Issues

The Llama tokenizer used in this project sometimes permits multiple possible tokenizations for a given string. As a result, it is sometimes the case that the arithmetic coder produces a series of tokens which don't match the canonical tokenization for that string exactly. In these cases, decoding will fail and you may need to try to run the encoding process again.

To mitigate this, the encoder will try decoding the output before returning it. This validation step requires extra time and memory usage. To skip validation, use the `-n` (`--no-validate`) parameter.

## Acknowledgements

This project wouldn't have been possible without the work of Matt Timmermans and his [Bijective Arithmetic Coding algorithm](https://web.archive.org/web/20210901195459/http://www3.sympatico.ca/mt0000/biacode/). The bijective arithmetic coding algorithm is necessary to be able to decompress arbitrary bit streams. Matt Timmermans' code was ported to Python with the assistance of Anthropic's Claude 3.5 Sonnet for inclusion in this project.

## Similar Projects

- **Fabrice Bellard's [ts_sms](https://www.bellard.org/ts_sms/) and [ts_zip](https://www.bellard.org/ts_zip/) projects**

  These projects essentially perform the opposite task as this one. In these projects, the arithmetic coder is actually used for compression, as it is designed to be used.

- **[Neural Linguistic Steganography](https://aclanthology.org/D19-1115.pdf)**

  This 2019 paper from Zachary Ziegler, Yuntian Deng, and Alexander Rush explores the same idea as what is proposed here. They also provide a [web application](https://steganography.live/) and [associated source code](https://github.com/harvardnlp/NeuralSteganography) for their implementation. See also the [Hacker News discussion of the app](https://news.ycombinator.com/item?id=20889881).

- **[Perfectly Secure Steganography Using Minimum Entropy Coupling](https://arxiv.org/pdf/2210.14889)**

  This 2023 paper from Christian Schroeder de Witt, Samuel Sokota, J. Zico Kolter, Jakob Foerster, and Martin Strohmeier describes some possible improvements upon this technique. The paper was [discussed on Hacker News here](https://news.ycombinator.com/item?id=36022598). See also the [Quanta article covering this paper](https://www.quantamagazine.org/secret-messages-can-hide-in-ai-generated-media-20230518/) and the [associated Hacker News discussion of that article](https://news.ycombinator.com/item?id=36004980).
