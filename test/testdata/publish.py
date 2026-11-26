#!/usr/bin/env python3
###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
"""Publish a test data set to R2 as a new version, and pin it.

    publish.py <name> <dir> [--private] [--version N] [--dry-run]
               [--profile P]

Packs every file under <dir> into <name>_v<N>.zip, byte-identical for the
same files (sorted, fixed timestamps), and computes its SHA-256. N is one
past the version pinned in sets.txt, or 1 for a new set. Then, unless
--dry-run, it refuses to go on if that version already exists on R2,
uploads it with the aws command and your R2 credentials, and pins the new
version in sets.txt. A published version is never overwritten.

    public   s3://cycfi-media/testdata/<name>/<name>_v<N>.zip
    private  s3://cycfi-testdata/<name>/<name>_v<N>.zip
"""
import argparse
import subprocess
import sys
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import testdata  # noqa: E402

PUBLIC_BUCKET = "cycfi-media"
PUBLIC_PREFIX = "testdata/"


def pack(src, dest):
    files = sorted(p for p in src.rglob("*") if p.is_file()
                   and not p.name.startswith("."))
    with zipfile.ZipFile(dest, "w", zipfile.ZIP_DEFLATED,
                         compresslevel=9) as z:
        for p in files:
            info = zipfile.ZipInfo(p.relative_to(src).as_posix(),
                                   date_time=(1980, 1, 1, 0, 0, 0))
            info.external_attr = 0o644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, p.read_bytes(), compresslevel=9)
    return len(files)


def aws(args, profile, check=True):
    cmd = ["aws"] + args + ["--endpoint-url", testdata.ENDPOINT]
    if profile:
        cmd += ["--profile", profile]
    return subprocess.run(cmd, check=check, capture_output=True, text=True)


def pin(name, line):
    text = testdata.SETS.read_text().splitlines()
    out = [l for l in text if l.split()[:1] != [name]]
    out.append(line)
    testdata.SETS.write_text("\n".join(out) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("dir", type=Path)
    ap.add_argument("--private", action="store_true")
    ap.add_argument("--version", type=int)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--profile")
    a = ap.parse_args()
    if not a.dir.is_dir() or not any(p.is_file() for p in a.dir.rglob("*")):
        sys.exit(f"{a.dir}: no such folder, or no files in it")

    have = testdata.sets().get(a.name)
    version = a.version or (have[0] + 1 if have else 1)
    access = "private" if a.private else "public"
    zp = Path.cwd() / f"{a.name}_v{version}.zip"
    n = pack(a.dir, zp)
    digest = testdata.sha256(zp)
    line = f"{a.name} {version} {access} {digest}"
    print(f"{zp.name}: {n} files, {zp.stat().st_size / 1e6:.1f} MB")

    if a.dry_run:
        print(line)
        return

    bucket = testdata.PRIVATE_BUCKET if a.private else PUBLIC_BUCKET
    key = ("" if a.private else PUBLIC_PREFIX) + f"{a.name}/{zp.name}"
    if aws(["s3api", "head-object", "--bucket", bucket, "--key", key],
           a.profile, check=False).returncode == 0:
        sys.exit(f"s3://{bucket}/{key} already exists; versions are never "
                 "overwritten")
    aws(["s3", "cp", str(zp), f"s3://{bucket}/{key}",
         "--content-type", "application/zip"], a.profile)
    pin(a.name, line)
    zp.unlink()
    print(f"uploaded s3://{bucket}/{key}")
    print(f"pinned in {testdata.SETS}: {line}")


if __name__ == "__main__":
    main()
