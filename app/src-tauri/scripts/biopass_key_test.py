#!/usr/bin/env python3
"""BioPass Key Tester.

Tests the derived salt and master key hash in:
  /usr/lib/biopass/secret.yaml
"""

from __future__ import annotations

import argparse
import base64
import hmac
import os
import re
import sys
from getpass import getpass

try:
    from argon2.low_level import Type, hash_secret_raw
except ImportError as exc:  # pragma: no cover - dependency check
    print("ERROR: Missing dependency 'argon2-cffi'.", file=sys.stderr)
    print("Install with: pip install argon2-cffi", file=sys.stderr)
    raise SystemExit(1) from exc

INPUT_DIR = "/usr/lib/biopass"
INPUT_PATH = f"{INPUT_DIR}/secret.yaml"


def print_error(message: str) -> None:
    print(message, file=sys.stderr)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Test a BioPass master key against /usr/lib/biopass/secret.yaml"
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--password",
        help="Master key provided directly (least secure; may be visible in process list)",
    )
    group.add_argument(
        "--password-stdin",
        action="store_true",
        help="Read master key from stdin (recommended for process-to-process usage)",
    )
    group.add_argument(
        "--password-env",
        metavar="ENV_VAR",
        help="Read master key from an environment variable name",
    )
    parser.add_argument(
        "--result-only",
        action="store_true",
        help="Print only 'success' or 'error' for easier automation",
    )
    return parser.parse_args()


def xor_bytes(left: bytes, right: bytes) -> bytes:
    """XOR left with right, repeating right as needed."""
    if not right:
        raise ValueError("XOR right-hand operand cannot be empty")
    return bytes(left[i] ^ right[i % len(right)] for i in range(len(left)))


def is_root() -> None:
    if os.geteuid() == 0:
        print_error("ERROR: BioPass Key Tester must not be run as root.")
        raise SystemExit(1)


def load_secret_yaml(path: str) -> dict:
    if not os.path.exists(path):
        print_error(f"ERROR: Secret file not found at {path}.")
        raise SystemExit(1)
    if not os.path.isfile(path):
        print_error(f"ERROR: Expected a file at {path}, but found a directory.")
        raise SystemExit(1)
    if not os.access(path, os.R_OK):
        print_error(f"ERROR: Secret file at {path} is not readable.")
        raise SystemExit(1)

    try:
        with open(path, "r", encoding="utf-8") as handle:
            content = handle.read()
            secret_data = {}
            for raw_line in content.splitlines():
                line = raw_line.strip()
                if not line or line.startswith("#") or ":" not in line:
                    continue

                key, value = line.split(":", 1)
                key = key.strip()
                value = value.strip()

                if len(value) >= 2 and value[0] == value[-1] and value[0] in {'"', "'"}:
                    value = value[1:-1]

                value = re.split(r"\s+#", value, maxsplit=1)[0].strip()
                secret_data[key] = value

            return secret_data
    except Exception as exc:
        print_error(f"ERROR: Failed to read or parse secret file at {path}: {exc}")
        raise SystemExit(1) from exc


def prompt_master_key() -> bytes:
    master_key = getpass("Enter BioPass Master Key: ").strip()
    if not master_key:
        print_error("ERROR: Master key cannot be empty.")
        raise SystemExit(1)
    return master_key.encode("utf-8")


def read_master_key_from_args(args: argparse.Namespace) -> bytes:
    if args.password is not None:
        master_key = args.password
    elif args.password_stdin:
        master_key = sys.stdin.read().rstrip("\r\n")
    elif args.password_env:
        master_key = os.environ.get(args.password_env, "")
    else:
        return prompt_master_key()

    if not master_key:
        print_error("ERROR: Master key cannot be empty.")
        raise SystemExit(1)

    return master_key.encode("utf-8")


def compare_hashes(derived_hash: bytes, expected_hash: bytes) -> bool:
    return hmac.compare_digest(derived_hash, expected_hash)


def derive_master_key_hash(master_key: bytes, salt: bytes) -> bytes:
    salted_master = xor_bytes(master_key, salt)
    return hash_secret_raw(
        secret=salted_master,
        salt=salt[:16],
        time_cost=4,
        memory_cost=131072,
        parallelism=2,
        hash_len=32,
        type=Type.ID,
    )


def main() -> int:
    args = parse_args()
    is_root()
    secret_data = load_secret_yaml(INPUT_PATH)

    salt_key = "salt_b64" if "salt_b64" in secret_data else "salt"
    hash_key = (
        "master_key_hash_b64"
        if "master_key_hash_b64" in secret_data
        else "master_key_hash"
    )

    if salt_key not in secret_data or hash_key not in secret_data:
        print_error(
            "ERROR: Secret file is missing required fields "
            "'salt_b64' and 'master_key_hash_b64'."
        )
        raise SystemExit(1)

    try:
        salt = base64.b64decode(secret_data[salt_key], validate=True)
        expected_master_key_hash = base64.b64decode(secret_data[hash_key], validate=True)
    except Exception as exc:
        print_error(f"ERROR: Failed to decode base64 values from secret file: {exc}")
        raise SystemExit(1) from exc

    master_key = read_master_key_from_args(args)

    if len(salt) < 16:
        print_error("ERROR: Decoded salt is too short; expected at least 16 bytes.")
        raise SystemExit(1)

    derived_master_key_hash = derive_master_key_hash(master_key, salt)

    if compare_hashes(derived_master_key_hash, expected_master_key_hash):
        if args.result_only:
            print("success")
        else:
            print("SUCCESS: Master key is valid.")
        return 0

    if args.result_only:
        print("error")
    else:
        print_error("ERROR: Master key is invalid.")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())