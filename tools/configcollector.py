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
import caseconversions
import collections
import outputfile
import re
from pprint import pprint

passbyvalue = {'int', 'bool', 'float', 'double' }
variantconversions = {
	'QString': 'toString',
	'bool': 'toBool',
	'int': 'toInt',
	'float': 'toFloat',
	'double': 'toDouble',
	'QChar': 'toChar',
	'QBitArray': 'toBitArray',
	'QDate': 'toDate',
	'QDateTime': 'toDateTime',
	'uint': 'toUInt',
	'unsigned int': 'toUInt',
	'QUrl': 'toUrl',
	'QTime': 'toTime',
	'QPoint': 'toPoint',
	'QPointF': 'toPointF',
	'QSize': 'toSize',
	'QSizeF': 'toSizeF',
	'qreal': 'toReal',
	'QRect': 'toRect',
	'QRectF': 'toRectF',
	'QLine': 'toLine',
	'QLineF': 'toLineF',
	'QEasingCurve': 'toEasingCurve',
	'qlonglong': 'toLongLong',
	'qulonglong': 'toULongLong',
}

class ConfigCollector (object):
	def __init__ (self, args):
		self.pattern = re.compile (r'\s*ConfigOption\s*\((.+)\)\s*')
		self.declpattern = re.compile (r'^([A-Za-z0-9,<>\[\]\(\)\{\}\s]+)\s+(\w+)(\s*=\s*(.+))?$')
		self.decls = []
		self.qttypes = set()
		self.args = args

	def collect (self, filenames):
		for filename in filenames:
			with open (filename, 'r') as fp:
				lines = fp.read().splitlines()

			for line in lines:
				matches = self.pattern.findall (line)
				for match in matches:
					match = match.strip()
					declarations = self.declpattern.findall (match)
					for decl in declarations:
						self.add_config_declaration (decl)

		self.decls.sort (key=lambda x: x['name'].upper())

	def add_config_declaration (self, decl):
		decltype, declname, junk, decldefault = decl
		if not decldefault:
			if decltype == 'int':
				decldefault = '0'
			elif decltype == 'float':
				decldefault = '0.0f'
			elif decltype == 'double':
				decldefault = '0.0'
			elif decltype == 'bool':
				raise TypeError ('bool entries must provide a default value')
			else:
				decldefault = decltype + '()'

		self.decls.append (dict (name=declname, type=decltype, default=decldefault))

		# Take note of any Qt types we may want to #include in our source file (we'll need to #include them).
		self.qttypes.update (re.findall (r'(Q[A-Za-z]+)', decltype))

	def make_config_key_type (self, basetype):
		result = 'ConfigTypeKey_' + basetype
		result = re.sub (r'[^\w]+', '_', result)
		return result

	def make_enums (self):
		self.enums = collections.OrderedDict()

		for decl in self.decls:
			enumname = self.make_config_key_type (decl['type'])
			decl['enumerator'] = caseconversions.convert_case (decl['name'], style='upper')
			decl['enumname'] = enumname
			decl['getter'] = caseconversions.convert_case (decl['name'], style='java')
			decl['varname'] = 'm_' + decl['getter']
			decl['setter'] = 'set' + caseconversions.convert_case (decl['name'], style='camel')
			decl['typecref'] = 'const %s&' % decl['type'] if decl['type'] not in passbyvalue else decl['type']

			try:
				decl['valuefunc'] = variantconversions[decl['type']]
			except KeyError:
				decl['valuefunc'] = 'value<' + decl['type'] + '>'

			if enumname not in self.enums:
				self.enums[enumname] = dict (
					name = enumname,
					typename = decl['type'],
					values = [],
					numvalues = 0,
					highestkey = -1)

			self.enums[enumname]['values'].append (decl)
			self.enums[enumname]['numvalues'] += 1
			self.enums[enumname]['highestkey'] += 1

		self.enums = collections.OrderedDict (sorted (self.enums.items(), key=lambda x: x[0].upper()))

		for name, enum in self.enums.items():
			for i, value in enumerate (enum['values']):
				value['index'] = i

	def write_header (self, fp):
		fp.write ('#pragma once\n')
		fp.write ('#include <QMap>\n')
		for qttype in sorted (self.qttypes):
			fp.write ('#include <%s>\n' % qttype)
		formatargs = {}
		write = lambda value: fp.write (value)
		write ('class ConfigurationValueBag\n')
		write ('{\n')
		write ('public:\n')
		write ('\tConfigurationValueBag();\n')
		write ('\t~ConfigurationValueBag();\n')
		write ('\tbool existsEntry (const QString& name);\n')
		write ('\tQVariant defaultValueByName (const QString& name);\n')

		for decl in self.decls:
			write ('\t{type} {getter}() const;\n'.format (**decl))
		for decl in self.decls:
			write ('\tvoid {setter} ({typecref} value);\n'.format (**decl))

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
			'ConfigurationValueBag::ConfigurationValueBag() :\n'
			'\tm_settings (makeSettings (nullptr))\n'
			'{\n')

		for decl in self.decls:
			fp.write ('\tm_defaults["{name}"] = QVariant::fromValue<{type}> ({default});\n'.format (**decl))

		fp.write ('}\n'
			'\n'
			'ConfigurationValueBag::~ConfigurationValueBag()\n'
			'{\n'
			'\tm_settings->deleteLater();\n'
			'}\n'
			'\n')

		maptype = 'QMap<QString, QVariant>'
		fp.write ('QVariant ConfigurationValueBag::defaultValueByName (const QString& name)\n')
		fp.write ('{\n')
		fp.write ('\t%s::iterator it = m_defaults.find (name);\n' % maptype)
		fp.write ('\tif (it != m_defaults.end())\n')
		fp.write ('\t\treturn *it;\n')
		fp.write ('\treturn QVariant();\n')
		fp.write ('}\n')
		fp.write ('\n')
		fp.write ('bool ConfigurationValueBag::existsEntry (const QString& name)\n')
		fp.write ('{\n')
		fp.write ('\treturn m_defaults.find (name) != m_defaults.end();\n')
		fp.write ('}\n')
		fp.write ('\n')

		for decl in self.decls:
			fp.write ('{type} ConfigurationValueBag::{getter}() const\n'.format (**decl))
			fp.write ('{\n')
			fp.write ('\tstatic const QVariant defaultvalue = QVariant::fromValue<{type}> ({default});\n'.format (**decl))
			fp.write ('\treturn m_settings->value ("{name}", defaultvalue).{valuefunc}();\n'.format (**decl))
			fp.write ('}\n')
			fp.write ('\n')

		for decl in self.decls:
			fp.write ('void ConfigurationValueBag::{setter} ({typecref} value)\n'.format (**decl))
			fp.write ('{\n')
			fp.write ('\tif (value != {default})\n'.format (**decl))
			fp.write ('\t\tm_settings->setValue ("{name}", QVariant::fromValue<{type}> (value));\n'.format (**decl))
			fp.write ('\telse\n')
			fp.write ('\t\tm_settings->remove ("{name}");\n'.format (**decl))
			fp.write ('}\n')
			fp.write ('\n')

def main():
	parser = argparse.ArgumentParser (description='Collects a list of configuration objects')
	parser.add_argument ('inputs', nargs='+')
	parser.add_argument ('--header', required=True)
	parser.add_argument ('--source', required=True)
	parser.add_argument ('--sourcedir', required=True)
	args = parser.parse_args()
	collector = ConfigCollector (args)
	collector.collect (args.inputs)
	collector.make_enums()
	header = outputfile.OutputFile (args.header)
	source = outputfile.OutputFile (args.source)
	collector.write_source (source, headername=args.header)
	collector.write_header (header)
	header.save (verbose = True)
	source.save (verbose = True)

if __name__ == '__main__':
	main()
