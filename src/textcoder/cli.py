import argparse
import gc
import getpass
import io
import logging
import os
import sys

import cryptography
import torch

from textcoder.coding import LLMArithmeticCoder
from textcoder.crypto import decrypt, encrypt

_logger = logging.getLogger(__name__)


def main():
    parser = argparse.ArgumentParser(
        prog="textcoder",
        description="Steganographic message encoder using LLMs",
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
        "-a",
        "--no-acceleration",
        action="store_true",
        help="disable hardware acceleration",
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
    os.environ["CUBLAS_WORKSPACE_CONFIG"] = ":4096:8"
    torch.use_deterministic_algorithms(True)
    coder = LLMArithmeticCoder(accelerated=not args.no_acceleration)
    if not args.decode:
        _logger.info("encoding starting")
        try:
            stdin_bytes = sys.stdin.buffer.read()
            encrypted_bytes = encrypt(password, stdin_bytes)
            if _logger.isEnabledFor(logging.INFO):
                _logger.info(f"encrypted bytes: 0x{encrypted_bytes.hex()}")
            with io.BytesIO(encrypted_bytes) as input_stream:
                stegotext = coder.decode(input_stream)
        except torch.OutOfMemoryError:
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
                    coder.encode(stegotext, validation_stream)
                    validation_bytes = validation_stream.getvalue()
                if _logger.isEnabledFor(logging.INFO):
                    _logger.info(f"encrypted bytes:  0x{encrypted_bytes.hex()}")
                    _logger.info(f"validation bytes: 0x{validation_bytes.hex()}")
                decrypt(password, validation_bytes)
            except torch.OutOfMemoryError:
                _logger.error(
                    "ran out of memory while validating encrypted message",
                    exc_info=args.verbose,
                )
                return
            except (ValueError, cryptography.exceptions.InvalidTag):
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
                coder.encode(stdin_str, output_stream)
                output_bytes = output_stream.getvalue()
            if _logger.isEnabledFor(logging.INFO):
                _logger.info(f"output bytes: 0x{output_bytes.hex()}")
            plaintext = decrypt(password, output_bytes)
            sys.stdout.buffer.write(plaintext)
        except torch.OutOfMemoryError:
            _logger.error(
                "ran out of memory while decrypting message", exc_info=args.verbose
            )
            return
        except (TypeError, ValueError, cryptography.exceptions.InvalidTag):
            _logger.error(
                "decryption failed. this could be due to an incorrect "
                "password or the data being corrupt.",
                exc_info=args.verbose,
            )
            return
        except Exception:
            _logger.error("unable to decrypt encrypted message", exc_info=args.verbose)
            return
