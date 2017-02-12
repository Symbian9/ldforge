#!/usr/bin/env python3
# coding: utf-8
#
#	Copyright 2015 - 2017 Teemu Piippo
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
import caseconversions
import collections
import outputfile
import re
from pprint import pprint

passbyvalue = {'int', 'bool', 'float', 'double', 'qreal'}

def deduce_type(value):
	'''
		Try to determine the type of value from the value itself.
	'''
	if value in ('true', 'false'):
		return 'bool'

	if value.startswith('"') and value.endswith('"'):
		return 'QString'

	try:
		int(value)
		return 'int'
	except:
		pass

	try:
		float(value)
		return 'double'
	except:
		pass

	if endswith(value, 'f'):
		try:
			float(value[:-1])
			return 'float'
		except:
			pass

	raise ValueError('unable to deduce type of %r' % value)

class ConfigCollector (object):
	def __init__ (self, args):
		self.decls = []
		self.qttypes = set()
		self.args = args

	def collect (self, filename):
		with open(filename, 'r') as file:
			for line in file:
				line = line.strip()
				if line and not line.startswith('#'):
					match = re.search('^option (\w+) = (.+)$', line)
					if not match:
						raise ValueError('unable to parse: %r' % line)
					name, value = match.groups()
					match = re.search(r'^(\w+)\s*\{(.*)\}$', value)
					try:
						type, value = match.groups()
						if not value:
							value = type + ' {}'
					except:
						type = deduce_type(value)
					self.add_config_declaration((name, type, value))

		self.decls.sort (key=lambda x: x['name'].upper())

		for decl in self.decls:
			decl['getter'] = caseconversions.convert_case (decl['name'], style='java')
			decl['varname'] = 'm_' + decl['getter']
			decl['setter'] = 'set' + caseconversions.convert_case (decl['name'], style='camel')
			decl['toggler'] = 'toggle' + caseconversions.convert_case (decl['name'], style='camel')
			decl['typecref'] = 'const %s&' % decl['type'] if decl['type'] not in passbyvalue else decl['type']
			decl['valuefunc'] = 'value<' + decl['type'] + '>'

	def add_config_declaration (self, decl):
		declname, decltype, decldefault = decl
		self.decls.append (dict (name=declname, type=decltype, default=decldefault))

		# Take note of any Qt types we may want to #include in our source file (we'll need to #include them).
		self.qttypes.update (re.findall (r'Q\w+', decltype))

	def write_header (self, fp):
		fp.write ('#pragma once\n')
		fp.write ('#include <QMap>\n')
		for qttype in sorted (self.qttypes):
			fp.write ('#include <%s>\n' % qttype)
		fp.write ('\n')
		formatargs = {}
		write = lambda value: fp.write (value)
		write ('class Configuration\n')
		write ('{\n')
		write ('public:\n')
		write ('\tConfiguration();\n')
		write ('\t~Configuration();\n')
		write ('\tbool existsEntry (const QString& name);\n')
		write ('\tQVariant defaultValueByName (const QString& name);\n')

		for decl in self.decls:
			write ('\t{type} {getter}() const;\n'.format (**decl))
		for decl in self.decls:
			write ('\tvoid {setter}({typecref} value);\n'.format (**decl))

		for decl in filter(lambda decl: decl['type'] == 'bool', self.decls):
			write('\tvoid {toggler}();\n'.format(**decl))

		write ('\n')
		write ('private:\n')
		write ('\tQMap<QString, QVariant> m_defaults;\n')
		write ('\tclass QSettings* m_settings;\n')
		write ('};\n')

	def write_source (self, fp, headername):
		fp.write ('#include <QSet>\n')
		fp.write ('#include <QSettings>\n')
		fp.write ('#include <QVariant>\n')
		fp.write ('#include "%s/mainwindow.h"\n' % (self.args.sourcedir))
		fp.write ('#include "%s"\n' % headername)

		fp.write (
			'\n'
			'Configuration::Configuration() :\n'
			'\tm_settings (makeSettings (nullptr))\n'
			'{\n')

		for decl in self.decls:
			fp.write ('\tm_defaults["{name}"] = QVariant::fromValue<{type}> ({default});\n'.format (**decl))

		fp.write ('}\n'
			'\n'
			'Configuration::~Configuration()\n'
			'{\n'
			'\tm_settings->deleteLater();\n'
			'}\n'
			'\n')

		maptype = 'QMap<QString, QVariant>'
		fp.write ('QVariant Configuration::defaultValueByName (const QString& name)\n')
		fp.write ('{\n')
		fp.write ('\t%s::iterator it = m_defaults.find (name);\n' % maptype)
		fp.write ('\tif (it != m_defaults.end())\n')
		fp.write ('\t\treturn *it;\n')
		fp.write ('\treturn QVariant();\n')
		fp.write ('}\n')
		fp.write ('\n')
		fp.write ('bool Configuration::existsEntry (const QString& name)\n')
		fp.write ('{\n')
		fp.write ('\treturn m_defaults.find (name) != m_defaults.end();\n')
		fp.write ('}\n')
		fp.write ('\n')

		for decl in self.decls:
			fp.write ('{type} Configuration::{getter}() const\n'.format (**decl))
			fp.write ('{\n')
			fp.write ('\tstatic const QVariant defaultvalue = QVariant::fromValue<{type}> ({default});\n'.format (**decl))
			fp.write ('\treturn m_settings->value ("{name}", defaultvalue).{valuefunc}();\n'.format (**decl))
			fp.write ('}\n')
			fp.write ('\n')

		for decl in self.decls:
			fp.write ('void Configuration::{setter}({typecref} value)\n'.format (**decl))
			fp.write ('{\n')
			fp.write ('\tif (value != {default})\n'.format (**decl))
			fp.write ('\t\tm_settings->setValue ("{name}", QVariant::fromValue<{type}> (value));\n'.format (**decl))
			fp.write ('\telse\n')
			fp.write ('\t\tm_settings->remove ("{name}");\n'.format (**decl))
			fp.write ('}\n')
			fp.write ('\n')

		for decl in filter(lambda decl: decl['type'] == 'bool', self.decls):
			fp.write('void Configuration::{toggler}()\n'.format(**decl))
			fp.write('{\n')
			fp.write('\t{setter}(not {getter}());\n'.format(**decl))
			fp.write('}\n')
			fp.write('\n')

def main():
	parser = argparse.ArgumentParser (description='Collects a list of configuration objects')
	parser.add_argument ('input')
	parser.add_argument ('--header', required=True)
	parser.add_argument ('--source', required=True)
	parser.add_argument ('--sourcedir', required=True)
	args = parser.parse_args()
	collector = ConfigCollector (args)
	collector.collect (args.input)
	header = outputfile.OutputFile (args.header)
	source = outputfile.OutputFile (args.source)
	collector.write_source (source, headername=args.header)
	collector.write_header (header)
	header.save (verbose = True)
	source.save (verbose = True)

if __name__ == '__main__':
	main()
