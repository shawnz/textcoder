import argparse
import gc
import getpass
import io
import logging
import sys

from cryptography.exceptions import InvalidTag
from torch import OutOfMemoryError

from textcoder.coding import decode, encode
from textcoder.crypto import decrypt, encrypt

_logger = logging.getLogger(__name__)


def main():
    parser = argparse.ArgumentParser(
        prog="textcoder",
        description="Stegangraphic message encoder using LLMs",
    )
    parser.add_argument(
        "-p",
        "--password",
        help="password for encryption of message",
    )
    parser.add_argument(
        "-n",
        "--no-validate",
        action="store_true",
        help="disable validation of encoded message",
    )
    parser.add_argument(
        "-d",
        "--decode",
        action="store_true",
        help="decoding mode",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="verbose mode",
    )
    args = parser.parse_args()
    if args.decode and args.no_validate:
        parser.error("validation is only supported when encoding, not decoding")
    if args.verbose:
        logging.basicConfig(
            level=logging.DEBUG, format="%(created)f:%(levelname)s:%(name)s:%(message)s"
        )
    _logger.info("script is starting")
    password = (
        args.password if args.password is not None else getpass.getpass()
    ).encode()
    if not args.decode:
        _logger.info("encoding starting")
        try:
            stdin_bytes = sys.stdin.buffer.read()
            encrypted_bytes = encrypt(password, stdin_bytes)
            with io.BytesIO(encrypted_bytes) as input_stream:
                stegotext = decode(input_stream)
        except OutOfMemoryError:
            _logger.error(
                "ran out of memory while encrypting message", exc_info=args.verbose
            )
            return
        except Exception:
            _logger.error("unable to encrypt message", exc_info=args.verbose)
            return
        if not args.no_validate:
            _logger.info("validation starting")
            try:
                gc.collect()
                with io.BytesIO() as validation_stream:
                    encode(stegotext, validation_stream)
                    validation_bytes = validation_stream.getvalue()
                decrypt(password, validation_bytes)
            except OutOfMemoryError:
                _logger.error(
                    "ran out of memory while validating encrypted message",
                    exc_info=args.verbose,
                )
                return
            except (ValueError, InvalidTag):
                _logger.error(
                    "validation of encrypted message failed. this could be "
                    "due to an ambiguous tokenization or non-deterministic "
                    "behaviour in the LLM.",
                    exc_info=args.verbose,
                )
                return
            except Exception:
                _logger.error(
                    "unable to validate encrypted message", exc_info=args.verbose
                )
                return
        print(stegotext)
    else:
        _logger.info("decoding starting")
        try:
            stdin_str = sys.stdin.read().removesuffix("\n")
            with io.BytesIO() as output_stream:
                encode(stdin_str, output_stream)
                output_bytes = output_stream.getvalue()
            plaintext = decrypt(password, output_bytes)
            sys.stdout.buffer.write(plaintext)
        except OutOfMemoryError:
            _logger.error(
                "ran out of memory while decrypting message", exc_info=args.verbose
            )
            return
        except (TypeError, ValueError, InvalidTag):
            _logger.error(
                "decryption failed. this could be due to an incorrect "
                "password or the data being corrupt.",
                exc_info=args.verbose,
            )
            return
        except Exception:
            _logger.error("unable to decrypt encrypted message", exc_info=args.verbose)
            return
