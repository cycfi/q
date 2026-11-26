#!/usr/bin/env python3
###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
"""The Python side of fetch.cmake: the same sets.txt, the same cache.

    from testdata import path
    d = path("multistring")     # a Path, or None when unavailable

    testdata.py multistring     # prints the directory, exit 77 if not
"""
import hashlib
import os
import shutil
import subprocess
import sys
import urllib.request
import uuid
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
SETS = HERE / "sets.txt"
PUBLIC = "https://media.cycfi.com"
PRIVATE_BUCKET = "cycfi-testdata"
ENDPOINT = ("https://24c350f8d000d722e6bcc12e3b03469a"
            ".r2.cloudflarestorage.com")


def cache():
    if os.environ.get("CYCFI_TESTDATA"):
        return Path(os.environ["CYCFI_TESTDATA"])
    if os.name == "nt":
        return Path(os.environ["LOCALAPPDATA"]) / "cycfi" / "testdata"
    return Path.home() / ".cache" / "cycfi" / "testdata"


def sets():
    out = {}
    for line in SETS.read_text().splitlines():
        f = line.split()
        if len(f) == 4 and not line.startswith("#"):
            out[f[0]] = (int(f[1]), f[2], f[3])
    return out


def sha256(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def _download(name, version, access, dest):
    obj = f"{name}/{name}_v{version}.zip"
    if access == "public":
        base = os.environ.get("CYCFI_TESTDATA_URL", PUBLIC)
        url = f"{base}/testdata/{obj}"
        req = urllib.request.Request(url,
                                     headers={"User-Agent": "q-testdata"})
        with urllib.request.urlopen(req, timeout=600) as r, \
                open(dest, "wb") as f:
            shutil.copyfileobj(r, f, 1 << 20)
    else:
        subprocess.run(["aws", "s3", "cp", f"s3://{PRIVATE_BUCKET}/{obj}",
                        str(dest), "--endpoint-url", ENDPOINT,
                        "--only-show-errors"], check=True)


def path(name, quiet=False):
    version, access, digest = sets()[name]
    d = cache() / f"{name}_v{version}"
    marker = d / ".complete"
    if marker.exists() and marker.read_text().strip() == digest:
        return d

    tag = uuid.uuid4().hex[:8]
    zp = cache() / f"{name}_v{version}.{tag}.zip"
    part = cache() / f"{name}_v{version}.{tag}.partial"
    try:
        cache().mkdir(parents=True, exist_ok=True)
        _download(name, version, access, zp)
        if sha256(zp) != digest:
            raise RuntimeError("SHA-256 mismatch")
        with zipfile.ZipFile(zp) as z:
            for m in z.infolist():
                parts = Path(m.filename).parts
                if m.filename.startswith("/") or ".." in parts:
                    raise RuntimeError(f"unsafe member {m.filename!r}")
            z.extractall(part)
        (part / ".complete").write_text(digest + "\n")
        shutil.rmtree(d, ignore_errors=True)
        part.rename(d)
        return d
    except Exception as e:
        if not quiet:
            print(f"testdata unavailable: {name}_v{version}: {e}",
                  file=sys.stderr)
        return None
    finally:
        zp.unlink(missing_ok=True)
        shutil.rmtree(part, ignore_errors=True)


if __name__ == "__main__":
    p = path(sys.argv[1])
    if p is None:
        sys.exit(77)
    print(p)
