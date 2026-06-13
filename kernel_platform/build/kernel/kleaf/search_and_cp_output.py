#!/usr/bin/env python3

import argparse
import os
import shutil
import sys


def handle_outputs_with_slash(srcdir, dstdir, outputs):
  errors = []
  for out in outputs:
    found = False
    for sdir in srcdir:
      if os.path.exists(os.path.join(sdir, out)):
        shutil.copy(os.path.join(sdir, out), dstdir)
        os.makedirs(os.path.dirname(os.path.join(dstdir, out)), exist_ok=True)
        shutil.copy(os.path.join(sdir, out), os.path.join(dstdir, out))
        found = True
        break
    if not found:
      errors.append(
        f"Unable to find {out} in any of the following directories:\n  " + (
          "\n  ".join(srcdir)))

  return errors


def handle_outputs_without_slash(srcdir, dstdir, outputs):
  errors = []
  for out in outputs:
    found = False
    for sdir in srcdir:
      if os.path.exists(os.path.join(sdir, out)):
        shutil.copy(os.path.join(sdir, out), dstdir)
        found = True
        break
      if not found:
        ok, matches = search_and_cp_output_one(sdir, dstdir, out)
        if ok:
          found = True
          break
        if len(matches) > 1:
          found = True
          errors.append(
            f"In {sdir}, multiple files match '{out}', expected at most 1:\n  " + (
              "\n  ".join(matches)))
          break
    if not found:
      errors.append(
        f"Unable to find {out} in any of the following directories:\n  " + (
          "\n  ".join(srcdir)))

  return errors


def search_and_cp_output_one(srcdir, dstdir, out):
  """Implements the search and move logic for outputs that need to be located.

  For each output in <outputs>, searches <output> within <srcdir>, and moves it
  to <dstdir>/<output>. if there's exactly one match, the file is moved.
  Otherwise, nothing is performed.

  Return all matches.
  """
  matches = []
  for root, dirs, files in os.walk(srcdir):
    for f in files + dirs:
      if f == out:
        matches.append(os.path.join(root, f))

  # realpath() of each object in matches, deduplicated
  real_matches = set(os.path.realpath(f) for f in matches)
  ok = len(real_matches) == 1
  if ok:
    shutil.copy(next(iter(real_matches)), os.path.join(dstdir, out))

  # For readable error messages, return |matches| instead of the realpaths here.
  return ok, matches


def main(srcdir, dstdir, outputs):
  """Locates and moves outputs matching multiple naming conventions.

  If <output> contains a slash, try the following on each srcdir:

    copy <srcdir>/<output> to <dstdir>/$(basename <output>)
    move <srcdir>/<output> to <dstdir>/<output>.

  If <output> does not contain a slash, try the following on each srcdir:
    If the file exists at the top level of <srcdir>, it is immediately chosen.
    Otherwise, searches <output> under <srcdir>.
      - If there's exactly one match, move it to <dstdir>/<output>.
      - If there are multiple matches, fail
      - If there's no match, try the next srcdir.
  """
  for sdir in srcdir:
    if not os.path.isdir(sdir):
      sys.exit(f"ERROR: srcdir {sdir} is not a directory.")
  if not os.path.isdir(dstdir):
    sys.exit(f"ERROR: dstdir {dstdir} is not a directory.")

  with_slash = [out for out in outputs if "/" in out]
  errors = handle_outputs_with_slash(srcdir, dstdir, with_slash)

  without_slash = [out for out in outputs if "/" not in out]
  errors += handle_outputs_without_slash(srcdir, dstdir, without_slash)

  if errors:
    sys.exit("ERROR: " + ("\n".join(errors)))


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description=main.__doc__)
  parser.add_argument("--srcdir", action="append", required=True,
                      help="""Source directory to search from.

You may specify multiple source directories with `--srcdir <SRCDIR> --srcdir <SRCDIR>`.
Early ones in the list takes higher priority.""")
  parser.add_argument("--dstdir", required=True, help="destination directory")
  parser.add_argument(
      "outputs",
      nargs="+",
      metavar="output_file_name",
      help="A list of output file names. Must not contain slashes.")
  args = parser.parse_args()
  main(**vars(args))
