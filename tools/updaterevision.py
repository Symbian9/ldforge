#!/usr/bin/env python
# coding: utf-8
#
#	Copyright 2015 Teemu Piippo
#	All rights reserved.
#
#	Redistribution and use in source and binary forms, with or without
#	modification, are permitted provided that the following conditions
#	are met:
#
#	1. Redistributions of source code must retain the above copyright
#	   notice, this list of conditions and the following disclaimer.
#	2. Redistributions in binary form must reproduce the above copyright
#	   notice, this list of conditions and the following disclaimer in the
#	   documentation and/or other materials provided with the distribution.
#	3. Neither the name of the copyright holder nor the names of its
#	   contributors may be used to endorse or promote products derived from
#	   this software without specific prior written permission.
#
#	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
#	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#	PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
#	OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import argparse
import outputfile

def main():
	import subprocess
	from datetime import datetime
	parser = argparse.ArgumentParser (description='Writes a header file with Hg commit information')
	parser.add_argument ('output')
	args = parser.parse_args()
	f = outputfile.OutputFile (args.output)
	data = subprocess.check_output (['hg', 'log', '-r.', '--template',
		'{node|short} {branch} {date|hgdate}']).replace ('\n', '').split (' ')

	rev = data[0]
	branch = data[1]
	timestamp = int (data[2])
	date = datetime.utcfromtimestamp (timestamp)
	datestring = date.strftime ('%y%m%d-%H%M') if date.year >= 2000 else '000000-0000'

	if len(rev) > 7:
		rev = rev[0:7]

	if subprocess.check_output (['hg', 'id', '-n']).replace ('\n', '')[-1] == '+':
		rev += '+'

	f.write ('#define HG_NODE "%s"\n' % rev)
	f.write ('#define HG_BRANCH "%s"\n' % branch)
	f.write ('#define HG_DATE_VERSION "%s"\n' % datestring)
	f.write ('#define HG_DATE_STRING "%s"\n' % date.strftime ('%d %b %Y'))
	f.write ('#define HG_DATE_TIME %d\n' % int (timestamp))
	if f.save():
		print '%s updated to %s' % (f.filename, rev)

if __name__ == '__main__':
	main()
