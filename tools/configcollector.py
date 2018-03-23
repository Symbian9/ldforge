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
#	EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
#	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING
#	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

from argparse import ArgumentParser
from collections import OrderedDict
import caseconversions
import outputfile

# These types are passed by value
passbyvalue = {'int', 'bool', 'float', 'double', 'qreal'}

def deduce_type(value):
	'''
		Try to determine the type of value from the value itself.
	'''
	if value in('true', 'false'):
		return 'bool'
	elif value.startswith('"') and value.endswith('"'):
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

	if value.endswith('f'):
		try:
			float(value[:-1])
			return 'float'
		except:
			pass

	raise ValueError('unable to deduce type of %r' % value)

class ConfigCollector:
	def __init__(self, args):
		self.declarations = OrderedDict()
		self.qtTypes = set()
		self.args = args

	def collect(self, filename):
		with open(filename) as file:
			for linenumber, line in enumerate(file, 1):
				try:
					line = line.strip()
					if line and not line.startswith('#'):
						from re import search
						match = search('^option (\w+) = (.+)$', line)
						if not match:
							raise ValueError('unable to parse: %r' % line)
						name, value = match.groups()
						match = search(r'^([a-zA-Z0-9_<>]+)\s*\{(.*)\}$', value)
						try:
							typename, value = match.groups()
							if not value:
								value = typename + ' {}'
						except:
							typename = deduce_type(value)
						self.declare(name, typename, value)
				except ValueError as error:
					from sys import stderr, exit
					print(str.format(
						'{file}:{line}: {error}',
						file = filename,
						line = linenumber,
						error = str(error),
					), file = stderr)
					exit(1)
		# Sort the declarations in alphabetical order
		self.declarations = OrderedDict(sorted(self.declarations.items(), key = lambda t: t[1]['name']))
		# Fill in additional information
		for declaration in self.declarations.values():
			declaration['readgate'] = caseconversions.convert_case(declaration['name'], style='java')
			declaration['writegate'] = 'set' + caseconversions.convert_case(declaration['name'], style='camel')
			declaration['togglefunction'] = 'toggle' + caseconversions.convert_case(declaration['name'], style='camel')
			if declaration['type'] in passbyvalue:
				declaration['typereference'] = declaration['type']
			else:
				declaration['typereference'] = 'const %s&' % declaration['type']

	def declare(self, name, typename, default):
		from re import findall
		if name in self.declarations:
			raise ValueError('Attempted to redeclare %r' % name)
		self.declarations[name] = {
			'name': name,
			'type': typename,
			'default': default
		}
		# Keep a file of any Qt types, we'll need to #include them.
		self.qtTypes.update(findall(r'Q\w+', typename))

	def writeHeader(self, device):
		device.write('#pragma once\n')
		device.write('#include <QMap>\n')
		for qtType in sorted(self.qtTypes):
			device.write('#include <%s>\n' % qtType)
		device.write('\n')
		formatargs = {}
		write = lambda value: device.write(value)
		write('class Configuration\n')
		write('{\n')
		write('public:\n')
		write('\tConfiguration();\n')
		write('\t~Configuration();\n')
		write('\tbool existsEntry(const QString& name);\n')
		write('\tQVariant defaultValueByName(const QString& name);\n')
		for declaration in self.declarations.values():
			write('\t{type} {readgate}() const;\n'.format(**declaration))
		for declaration in self.declarations.values():
			write('\tvoid {writegate}({typereference} value);\n'.format(**declaration))

		for declaration in filter(lambda declaration: declaration['type'] == 'bool', self.declarations.values()):
			write('\tvoid {togglefunction}();\n'.format(**declaration))
		write('\n')
		write('private:\n')
		write('\tQMap<QString, QVariant> m_defaults;\n')
		write('\tclass QSettings* m_settings;\n')
		write('};\n')
	
	def writeSource(self, device, headername):
		device.write('#include <QSet>\n')
		device.write('#include <QSettings>\n')
		device.write('#include <QVariant>\n')
		device.write('#include "%s/mainwindow.h"\n' % (self.args.sourcedir))
		device.write('#include "%s"\n' % headername)
		device.write(
			'\n'
			'Configuration::Configuration() :\n'
			'\tm_settings(MainWindow::makeSettings(nullptr))\n'
			'{\n')
		for declaration in self.declarations.values():
			device.write('\tm_defaults["{name}"] = QVariant::fromValue<{type}>({default});\n'.format(**declaration))
		device.write('}\n'
			'\n'
			'Configuration::~Configuration()\n'
			'{\n'
			'\tm_settings->deleteLater();\n'
			'}\n'
			'\n')
		device.write('QVariant Configuration::defaultValueByName(const QString& name)\n')
		device.write('{\n')
		device.write('\tQMap<QString, QVariant>::iterator it = m_defaults.find(name);\n')
		device.write('\tif(it != m_defaults.end())\n')
		device.write('\t\treturn *it;\n')
		device.write('\telse\n')
		device.write('\t\treturn {};\n')
		device.write('}\n')
		device.write('\n')
		device.write('bool Configuration::existsEntry(const QString& name)\n')
		device.write('{\n')
		device.write('\treturn m_defaults.find(name) != m_defaults.end();\n')
		device.write('}\n')
		device.write('\n')
		for declaration in self.declarations.values():
			device.write('{type} Configuration::{readgate}() const\n'.format(**declaration))
			device.write('{\n')
			device.write('\tstatic const QVariant defaultvalue = QVariant::fromValue<{type}>({default});\n'.format(**declaration))
			device.write('\treturn m_settings->value("{name}", defaultvalue).value<{type}>();\n'.format(**declaration))
			device.write('}\n')
			device.write('\n')
		for declaration in self.declarations.values():
			device.write('void Configuration::{writegate}({typereference} value)\n'.format(**declaration))
			device.write('{\n')
			device.write('\tif(value != {default})\n'.format(**declaration))
			device.write('\t\tm_settings->setValue("{name}", QVariant::fromValue<{type}>(value));\n'.format(**declaration))
			device.write('\telse\n')
			device.write('\t\tm_settings->remove("{name}");\n'.format(**declaration))
			device.write('}\n')
			device.write('\n')
		for declaration in filter(lambda declaration: declaration['type'] == 'bool', self.declarations.values()):
			device.write('void Configuration::{togglefunction}()\n'.format(**declaration))
			device.write('{\n')
			device.write('\t{writegate}(not {readgate}());\n'.format(**declaration))
			device.write('}\n')
			device.write('\n')

def main():
	parser = ArgumentParser(description='Collects a list of configuration objects')
	parser.add_argument('input')
	parser.add_argument('--header', required=True)
	parser.add_argument('--source', required=True)
	parser.add_argument('--sourcedir', required=True)
	args = parser.parse_args()
	collector = ConfigCollector(args)
	collector.collect(args.input)
	header = outputfile.OutputFile(args.header)
	source = outputfile.OutputFile(args.source)
	collector.writeSource(source, headername=args.header)
	collector.writeHeader(header)
	header.save(verbose = True)
	source.save(verbose = True)

if __name__ == '__main__':
	main()
