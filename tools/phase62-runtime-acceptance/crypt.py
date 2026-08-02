"""Minimal libcrypt compatibility for Python runtimes without stdlib crypt.

The Phase 62 runtime acceptance runner needs only crypt.crypt() for temporary
SHA-512 verifier hashes.  Python 3.13 removed the deprecated standard-library
module, while the daemon and target system still provide libcrypt.
"""

from __future__ import annotations

import ctypes
import ctypes.util
import threading


_library_name = ctypes.util.find_library("crypt")
if not _library_name:
    raise ImportError("libcrypt is unavailable")

_library = ctypes.CDLL(_library_name, use_errno=True)
_library.crypt.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_library.crypt.restype = ctypes.c_char_p
_lock = threading.Lock()


def crypt(word: str, salt: str) -> str | None:
    """Return the libcrypt result using the legacy Python crypt API shape."""

    if not isinstance(word, str) or not isinstance(salt, str):
        raise TypeError("word and salt must be strings")
    if "\0" in word or "\0" in salt:
        raise ValueError("embedded null byte")

    word_bytes = word.encode("utf-8")
    salt_bytes = salt.encode("utf-8")
    with _lock:
        result = _library.crypt(word_bytes, salt_bytes)
        if result is None:
            return None
        return result.decode("utf-8")
