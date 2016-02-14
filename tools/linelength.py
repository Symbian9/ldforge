#!/usr/bin/env python3
from sys import argv, stderr
from os.path import realpath

for filename in argv[1:]:
	with open(filename, 'r') as fp:
		try:
			exceeders = [(i + 1) for i, ln in enumerate(fp.read().splitlines()) if len(ln.replace('\t', '    ')) > 120]

			for linenumber in exceeders:
				stderr.write('%s:%d: warning: line length exceeds 120 characters\n' % (realpath(filename), linenumber))
		except Exception as e:
			stderr.write('%s: warning: %s: %s\n' % (realpath(filename), type(e).__name__, e))
