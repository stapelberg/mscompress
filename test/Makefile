clean:
	rm -f test.txt_ test.new

test:
	../mscompress test.txt
	../msexpand < test.txt_ > test.new
	diff -q test.compressed test.txt_
	diff -q test.new test.txt
