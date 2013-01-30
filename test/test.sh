#!/bin/sh

set -e

rm -f test.txt_ test.new
../src/mscompress test.txt
../src/msexpand < test.txt_ > test.new
diff -q test.compressed test.txt_
diff -q test.new test.txt
