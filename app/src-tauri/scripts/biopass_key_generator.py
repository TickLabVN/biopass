#!/usr/bin/env python3
"""BioPass Key Generator.

Generates and stores a derived salt and master key hash in:
  /usr/lib/biopass/secret.yaml
"""

from __future__ import annotations

import base64
import hashlib
import os
import stat
import sys
import textwrap
import urllib.error
import urllib.request
from datetime import datetime, timezone
from getpass import getpass

try:
    from argon2.low_level import Type, hash_secret_raw
except ImportError as exc:  # pragma: no cover - dependency check
    print("ERROR: Missing dependency 'argon2-cffi'.")
    print("Install with: pip install argon2-cffi")
    raise SystemExit(1) from exc


ENTROPY_URL = "https://www.random.org/strings/?num=100&len=32&digits=on&upperalpha=on&loweralpha=on&unique=off&format=plain"
OUTPUT_DIR = "/usr/lib/biopass"
OUTPUT_PATH = f"{OUTPUT_DIR}/secret.yaml"


def xor_bytes(left: bytes, right: bytes) -> bytes:
    """XOR left with right, repeating right as needed."""
    if not right:
        raise ValueError("XOR right-hand operand cannot be empty")
    return bytes(left[i] ^ right[i % len(right)] for i in range(len(left)))


def require_root() -> None:
    if os.geteuid() != 0:
        print("ERROR: BioPass Key Generator must be run as root.")
        raise SystemExit(1)


def fetch_entropy_source(url: str) -> bytes:
    try:
        with urllib.request.urlopen(url, timeout=20) as response:
            return response.read()
    except urllib.error.URLError as exc:
        print(f"ERROR: Failed to download entropy source from {url}: {exc}")
        raise SystemExit(1) from exc


def prompt_master_key() -> bytes:
    master_key = getpass("Enter BioPass Master Key: ").strip()
    verify_key = getpass("Re-enter BioPass Master Key to validate: ").strip()

    if not master_key:
        print("ERROR: Master key cannot be empty.")
        raise SystemExit(1)
    if master_key != verify_key:
        print("ERROR: Master key validation failed.")
        raise SystemExit(1)
    return master_key.encode("utf-8")


def derive_salt(master_key: bytes, downloaded_entropy: bytes) -> tuple[bytes, bytes]:
    random_256 = os.urandom(32)
    download_digest = hashlib.sha256(downloaded_entropy).digest()
    mixed_material = xor_bytes(random_256, download_digest)

    salt_seed = hash_secret_raw(
        secret=mixed_material,
        salt=hashlib.blake2b(downloaded_entropy, digest_size=16).digest(),
        time_cost=3,
        memory_cost=65536,
        parallelism=2,
        hash_len=32,
        type=Type.ID,
    )

    salted_master = xor_bytes(master_key, salt_seed)
    master_hash = hash_secret_raw(
        secret=salted_master,
        salt=salt_seed[:16],
        time_cost=4,
        memory_cost=131072,
        parallelism=2,
        hash_len=32,
        type=Type.ID,
    )

    return salt_seed, master_hash


def save_secret_yaml(salt: bytes, master_hash: bytes) -> None:
    os.makedirs(OUTPUT_DIR, mode=0o755, exist_ok=True)
    os.chmod(OUTPUT_DIR, 0o755)

    now = datetime.now(timezone.utc).isoformat()
    yaml_text = textwrap.dedent(
        f"""\
        generator: \"BioPass Key Generator\"
        created_at_utc: \"{now}\"
        salt_b64: \"{base64.b64encode(salt).decode('ascii')}\"
        master_key_hash_b64: \"{base64.b64encode(master_hash).decode('ascii')}\"
        """
    )

    tmp_path = f"{OUTPUT_PATH}.tmp"
    with open(tmp_path, "w", encoding="utf-8") as handle:
        handle.write(yaml_text)

    os.chmod(
        tmp_path,
        stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH,
    )
    os.replace(tmp_path, OUTPUT_PATH)


def main() -> int:
    print("BioPass Key Generator")
    print("WARNING: Do not set this key until BioPass is working correctly.")

    require_root()
    master_key = prompt_master_key()
    entropy_payload = fetch_entropy_source(ENTROPY_URL)
    salt, master_hash = derive_salt(master_key, entropy_payload)
    save_secret_yaml(salt, master_hash)

    print(f"Secret material written to {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    sys.exit(main())