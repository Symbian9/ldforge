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

import md5
import re
import sys
import os

class OutputFile:
	def __init__(self, filename):
		self.filename = filename
		try:
			with open (self.filename, "r") as fp:
				self.oldsum = fp.readline().replace ('\n', '').replace ('// ', '')
		except IOError:
			self.oldsum = ''

		self.body = ''

	def write(self, a):
		self.body += a

	def save(self):
		checksum = md5.new (self.body).hexdigest()

		if checksum == self.oldsum:
			print '%s is up to date' % self.filename
			return False

		with open (self.filename, "w") as fp:
			fp.write ('// %s\n' % checksum)
			fp.write (self.body)
			return True

def prog_configaux():
	class CfgEntry:
		def __init__(self, type, name, defvalue):
			self.type = type
			self.name = name
			self.defvalue = defvalue

	if len (sys.argv) < 3:
		print 'Usage: %s <output> <input1> [input2] [input3] ... [input-n]' % sys.argv[0]
		quit(1)

	f = OutputFile (sys.argv[1])
	f.write ('#pragma once\n')

	entries = []

	for inputname in sys.argv[2:]:
		fp = open (inputname, 'r')

		for line in fp.readlines():
			match = re.match (r'CFGENTRY\s*\(([^,]+),\s*([^,]+),\s*([^)]+)\)', line)
			if match:
				entries.append (CfgEntry (match.group (1), match.group (2), match.group (3)))

	for e in entries:
		f.write ('EXTERN_CFGENTRY (%s, %s)\n' % (e.type, e.name))

	f.write ('\n')
	f.write ('static void InitConfigurationEntry (class AbstractConfigEntry* entry);\n')
	f.write ('static void SetupConfigurationLists()\n')
	f.write ('{\n')

	for e in entries:
		f.write ('\tInitConfigurationEntry (new %sConfigEntry (&cfg::%s, \"%s\", %s));\n'
			% (e.type, e.name, e.name, e.defvalue))

	f.write ('}\n')

	if f.save():
		print 'Wrote configuration aux code to %s' % f.filename

def prog_hginfo():
	import subprocess
	from datetime import datetime

	if len (sys.argv) != 2:
		print 'usage: %s <output>' % sys.argv[0]
		quit (1)

	f = OutputFile (sys.argv[1])
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

def main():
	if len(sys.argv) < 2:
		print 'You must give a program name'
		quit(1)

	progname = sys.argv[1]
	sys.argv[0] = '%s %s' % (sys.argv[0], sys.argv[1])
	sys.argv.pop (1)

	impl = globals().copy()
	impl.update (locals())
	method = impl.get ('prog_' + progname)

	if method:
		method()
	else:
		print 'no such program %s' % progname

if __name__ == '__main__':
	main()