import argparse
import getpass
import io
import sys

from textcoder.crypto import decrypt, encrypt
from textcoder.coding import decode, encode


def main():
    parser = argparse.ArgumentParser(
        prog="textcoder",
        description="Stegangraphic message encoder using LLMs",
    )
    parser.add_argument(
        "-p",
        "--password",
        help="Password for encryption of message",
    )
    parser.add_argument(
        "-n",
        "--no-validate",
        action="store_true",
        help="Disable validation of encoded message",
    )
    parser.add_argument(
        "-d",
        "--decode",
        action="store_true",
        help="Decoding mode",
    )
    args = parser.parse_args()
    if args.decode and args.no_validate:
        parser.error("validation is only supported when encoding, not decoding")
    password = (
        args.password if args.password is not None else getpass.getpass()
    ).encode()
    if not args.decode:
        stdin_bytes = sys.stdin.buffer.read()
        encrypted_bytes = encrypt(password, stdin_bytes)
        with io.BytesIO(encrypted_bytes) as input_stream:
            stegotext = decode(input_stream)
        if not args.no_validate:
            try:
                gc.collect()
                with io.BytesIO() as validation_stream:
                    encode(stegotext, validation_stream)
                    validation_bytes = validation_stream.getvalue()
                decrypt(password, validation_bytes)
            except Exception as ex:
                parser.error(f"unable to validate encrypted message: {ex}")
        print(stegotext)
    else:
        try:
            stdin_str = sys.stdin.read().removesuffix("\n")
            with io.BytesIO() as output_stream:
                encode(stdin_str, output_stream)
                output_bytes = output_stream.getvalue()
            plaintext = decrypt(password, output_bytes)
            sys.stdout.buffer.write(plaintext)
        except Exception as ex:
            parser.error(f"unable to decrypt encrypted message: {ex}")
