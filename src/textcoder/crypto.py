import os

from cryptography.hazmat.primitives.ciphers.aead import AESGCMSIV
from cryptography.hazmat.primitives.kdf.argon2 import Argon2id

_ARGON2_ITERATIONS = 1
_ARGON2_LANES = 4
_ARGON2_MEMORY_COST = 64 * 1024


def _get_aesgcmsiv(password: bytes, salt: bytes):
    kdf = Argon2id(
        salt=salt,
        length=32,
        iterations=_ARGON2_ITERATIONS,
        lanes=_ARGON2_LANES,
        memory_cost=_ARGON2_MEMORY_COST,
    )
    key = kdf.derive(password)
    return AESGCMSIV(key)


def encrypt(password: bytes, plaintext: bytes):
    salt = os.urandom(16)
    nonce = salt[:12]
    aesgcmsiv = _get_aesgcmsiv(password, salt)
    ciphertext = aesgcmsiv.encrypt(nonce, plaintext, None)
    return salt + ciphertext


def decrypt(password: bytes, encrypted_bytes: bytes):
    salt = encrypted_bytes[:16]
    nonce = salt[:12]
    ciphertext = encrypted_bytes[16:]
    aesgcmsiv = _get_aesgcmsiv(password, salt)
    return aesgcmsiv.decrypt(nonce, ciphertext, None)
