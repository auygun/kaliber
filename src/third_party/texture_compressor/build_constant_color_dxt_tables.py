#!/usr/bin/env python
# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""code generator for texture compressor."""

import itertools
import os
import os.path
import sys
import re
import platform
from optparse import OptionParser
from subprocess import call

_LICENSE = """// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

"""

_DO_NOT_EDIT_WARNING = """// This file is auto-generated from
// build_constant_color_dxt_tables.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

"""

def Expand5(i):
  return (i << 3) | (i >> 2)

def Expand6(i):
  return (i << 2) | (i >> 4)

def Lerp13(a, b):
  """Linear interpolation at 1/3 point between a and b."""
  return ((2 * a + b) * 0xaaab) >> 17

class CWriter(object):
  """Writes to a file formatting it for Google's style guidelines."""

  def __init__(self, filename):
    self.filename = filename
    self.content = []

    self.Write(_LICENSE)
    self.Write(_DO_NOT_EDIT_WARNING)

  def Write(self, string):
    """Writes a string to a file spliting if it's > 80 characters."""
    lines = string.splitlines()
    num_lines = len(lines)
    for ii in range(0, num_lines):
      self.content.append(lines[ii])
      if ii < (num_lines - 1) or string[-1] == '\n':
        self.content.append('\n')

  def Close(self):
    """Close the file."""
    content = "".join(self.content)
    write_file = True
    if os.path.exists(self.filename):
      old_file = open(self.filename, "rb");
      old_content = old_file.read()
      old_file.close();
      if content == old_content:
        write_file = False
    if write_file:
      file = open(self.filename, "wb")
      file.write(content)
      file.close()

class CHeaderWriter(CWriter):
  """Writes a C Header file."""

  _non_alnum_re = re.compile(r'[^a-zA-Z0-9]')

  def __init__(self, filename, file_comment = None):
    CWriter.__init__(self, filename)

    base = os.path.abspath(filename)
    while os.path.basename(base) != 'src':
      new_base = os.path.dirname(base)
      assert new_base != base  # Prevent infinite loop.
      base = new_base

    hpath = os.path.relpath(filename, base)
    self.guard = self._non_alnum_re.sub('_', hpath).upper() + '_'

    if not file_comment == None:
      self.Write(file_comment)
    self.Write("#ifndef %s\n" % self.guard)
    self.Write("#define %s\n\n" % self.guard)

  def Close(self):
    self.Write("#endif  // %s\n\n" % self.guard)
    CWriter.Close(self)

class TableGenerator(object):
  """A class to generate table data."""

  def __init__(self):
    self.table = [0] * 256 * 2
    self.generated_cpp_filenames = []

  def GenerateOptTable(self, expand_max, expand_min, size_max, size_min):
    """Compute table to reproduce constant colors as accurately as possible."""
    for i in xrange(0, 256):
      best_err = 256
      for mn in xrange(0, size_min):
        for mx in xrange(0, size_max):
          mine = expand_min(mn)
          maxe = expand_max(mx)
          err = abs(Lerp13(maxe, mine) - i)

          # DX10 spec says that interpolation must be within 3% of "correct"
          # result, add this as error term. (normally we'd expect a random
          # distribution of +-1.5% error, but nowhere in the spec does it say
          # that the error has to be unbiased - better safe than sorry).
          err += abs(maxe - mine) * 3 / 100

          if err < best_err:
            self.table[i * 2 + 0] = mx
            self.table[i * 2 + 1] = mn
            best_err = err

  def WriteOptTable(self,file):
    """Writes the table data."""
    file.Write("{%d, %d}" % (self.table[0], self.table[1]))
    for i in xrange(1, 256):
      file.Write(",\n")
      file.Write("    {%d, %d}" %
                 (self.table[i * 2 + 0], self.table[i * 2 + 1]))

  def WriteConstantColorTables(self, filename):
    """Writes constant color tables."""

    file = CHeaderWriter(filename)

    file.Write("const uint8_t kDXTConstantColors55[256][2] = {")
    self.GenerateOptTable(Expand5, Expand5, 32, 32)
    self.WriteOptTable(file)
    file.Write("};\n")
    file.Write("\n")

    file.Write("const uint8_t kDXTConstantColors56[256][2] = {")
    self.GenerateOptTable(Expand5, Expand6, 32, 64)
    self.WriteOptTable(file)
    file.Write("};\n")
    file.Write("\n")

    file.Write("const uint8_t kDXTConstantColors66[256][2] = {")
    self.GenerateOptTable(Expand6, Expand6, 64, 64)
    self.WriteOptTable(file)
    file.Write("};\n")
    file.Write("\n")

    file.Close()
    self.generated_cpp_filenames.append(file.filename)

def Format(generated_files):
  formatter = "clang-format"
  if platform.system() == "Windows":
    formatter += ".bat"
  for filename in generated_files:
    call([formatter, "-i", "-style=chromium", filename])

def main(argv):
  """This is the main function."""
  parser = OptionParser()
  parser.add_option(
      "--output-dir",
      help="base directory for resulting files, under chrome/src. default is "
      "empty. Use this if you want the result stored under gen.")

  (options, args) = parser.parse_args(args=argv)

  # This script lives under cc/raster/, cd to base directory.
  # os.chdir(os.path.dirname(__file__) + "/../..")
  base_dir = os.getcwd()
  gen = TableGenerator()

  # Support generating files under gen/
  if options.output_dir != None:
    os.chdir(options.output_dir)

  gen.WriteConstantColorTables(
      "dxt_encoder_implementation_autogen.h")
  Format(gen.generated_cpp_filenames)

  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
