#!/usr/bin/env python
# coding: utf-8

'''
Provides facilities to converting identifier cases.
'''

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

import string
import re

def to_one_case (name, tolower):
	'''
	Convers name to either all uppercase or all lowercase (depending on the truth value of tolower), using underscores
	as delimiters.
	'''
	result = ""
	targetSet = (string.ascii_lowercase if tolower else string.ascii_uppercase) + string.digits
	inverseSet = (string.ascii_uppercase if tolower else string.ascii_lowercase)
	isUnderscorable = lambda ch: ch in string.ascii_uppercase \
		or ch in string.whitespace or ch in string.punctuation
	previousWasUnderscorable = isUnderscorable (name[0])

	for ch in name:
		if isUnderscorable (ch) and result != "" and not previousWasUnderscorable:
			result += "_"

		if ch in inverseSet:
			result += (ch.lower() if tolower else ch.upper())
		elif ch in targetSet:
			result += ch
		previousWasUnderscorable = isUnderscorable (ch)

	return result

def to_camel_case (name, java = False):
	'''
	Converts name to camelcase. If java is False, the first letter will be lowercase, otherwise it will be uppercase.
	'''
	result = ""
	wantUpperCase = False

	# If it's all uppercase, make it all lowercase so that this algorithm can digest it
	if name == name.upper():
		name = name.lower()

	for ch in name:
		if ch == '_':
			wantUpperCase = True
			continue

		if wantUpperCase:
			if ch in string.ascii_lowercase:
				ch = ch.upper()
			wantUpperCase = False

		result += ch

	if java:
		match = re.match (r'^([A-Z]+)([A-Z].+)$', result)
		if match:
			result = match.group (1).lower() + match.group (2)
		else:
			result = result[0].lower() + result[1:]
	else:
		result = result[0].upper() + result[1:]
	return result

case_styles = {
	'lower': lambda name: to_one_case (name, tolower=True),
	'upper': lambda name: to_one_case (name, tolower=False),
	'camel': lambda name: to_camel_case (name, java=False),
	'java': lambda name: to_camel_case (name, java=True),
}

def convert_case (name, style):
	'''
	Converts name to the given style. Style may be one of:
		- 'lower': 'foo_barBaz' -> 'foo_bar_baz'
		- 'upper': 'foo_barBaz' -> 'FOO_BAR_BAZ'
		- 'camel': 'foo_barBaz' -> 'FooBarBaz'
		- 'java': 'foo_barBaz' -> 'fooBarBaz'
	'''
	try:
		stylefunc = case_styles[style]
	except KeyError:
		validstyles = ', '.join (sorted (case_styles.keys()))
		raise ValueError ('Unknown style "%s", should be one of: %s' % (style, validstyles))
	else:
		return stylefunc (name)