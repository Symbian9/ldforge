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

import hashlib
import sys

class OutputFile:
	def __init__ (self, filename):
		self.filename = filename
		try:
			with open (self.filename, "r") as fp:
				self.oldsum = fp.readline().replace ('\n', '').replace ('// ', '')
		except IOError:
			self.oldsum = ''

		self.body = ''

	def write (self, a):
		self.body += a

	def save (self, verbose = False):
		if sys.version_info >= (3, 0):
			checksum = hashlib.md5 (bytes (self.body, 'utf-8')).hexdigest()
		else:
			checksum = hashlib.md5 (self.body).hexdigest()

		if checksum == self.oldsum:
			print ('%s is up to date' % self.filename)
			return False

		with open (self.filename, "w") as fp:
			fp.write ('// %s\n' % checksum)
			fp.write ('// This file has been automatically generated. Do not edit by hand\n')
			fp.write ('\n')
			fp.write (self.body)

			if verbose:
				print ('%s written' % self.filename)
			return True
