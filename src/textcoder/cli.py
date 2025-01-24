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
    stdin_str = sys.stdin.read()
    if not args.decode:
        encrypted_bytes = encrypt(password, stdin_str.encode())
        input_stream = io.BytesIO(encrypted_bytes)
        output_string = decode(input_stream)
        if not args.no_validate:
            try:
                output_stream = io.BytesIO()
                encode(output_string, output_stream)
                decrypt(password, output_stream.getvalue())
            except Exception as ex:
                parser.error(f"unable to validate encrypted message: {ex}")
        print(output_string)
    else:
        try:
            output_stream = io.BytesIO()
            encode(stdin_str, output_stream)
            plaintext = decrypt(password, output_stream.getvalue())
            print(plaintext.decode())
        except Exception as ex:
            parser.error(f"unable to decrypt encrypted message: {ex}")
