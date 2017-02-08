/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2017 Teemu Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <QIODevice>
#include <QGenericMatrix>
#include "basics.h"
#include "colors.h"

// Converts a given value into a string that can be retrieved with text().
// Used as the argument type to the formatting functions, hence its name.
class StringFormatArg
{
public:
	StringFormatArg (const QString& a) : m_text (a) {}
	StringFormatArg (char a) : m_text (a) {}
	StringFormatArg (uchar a) : m_text (a) {}
	StringFormatArg (QChar a) : m_text (a) {}
	StringFormatArg (int a) : m_text (QString::number (a)) {}
	StringFormatArg (long a) : m_text (QString::number (a)) {}
	StringFormatArg (float a) : m_text (QString::number (a)) {}
	StringFormatArg (double a) : m_text (QString::number (a)) {}
	StringFormatArg (const Vertex& a) : m_text (a.toString()) {}
	StringFormatArg (const Matrix& a) : m_text (a.toString()) {}
	StringFormatArg (const char* a) : m_text (a) {}
	StringFormatArg (LDColor a) : m_text (a.indexString()) {}

	StringFormatArg (const void* a)
	{
		m_text.sprintf ("%p", a);
	}

	template<typename T>
	StringFormatArg (const QVector<T>& a)
	{
		m_text = "{";

		for (const T& it : a)
		{
			if (&it != &a.first())
				m_text += ", ";

			StringFormatArg arg (it);
			m_text += arg.text();
		}

		m_text += "}";
	}

	template<int Columns, int Rows, typename Type>
	StringFormatArg(const QGenericMatrix<Columns, Rows, Type>& matrix)
	{
		m_text = "{";
		for (int row = 0; row < Rows; ++row)
		{
			if (row > 0)
				m_text += ", ";

			m_text += "{";

			for (int column = 0; column < Rows; ++column)
			{
				if (column > 0)
					m_text += ", ";

				m_text += StringFormatArg{matrix(row, column)}.text();
			}

			m_text += "}";
		}
	}

	inline QString text() const
	{
		return m_text;
	}

private:
	QString m_text;
};


// Helper function for format()
template<typename Arg1, typename... Rest>
void formatHelper (QString& str, Arg1 arg1, Rest... rest)
{
	str = str.arg (StringFormatArg (arg1).text());
	formatHelper (str, rest...);
}


static void formatHelper (QString& str) __attribute__ ((unused));
static void formatHelper (QString& str)
{
	(void) str;
}


// Format the message with the given args.
//
// The formatting ultimately uses String's arg() method to actually format the args so the format string should be
// prepared accordingly, with %1 referring to the first arg, %2 to the second, etc.
template<typename... Args>
QString format (QString fmtstr, Args... args)
{
	formatHelper (fmtstr, args...);
	return fmtstr;
}


// From messageLog.cc - declared here so that I don't need to include messageLog.h here.
void printToLog (const QString& msg);


// Format and print the given args to the message log.
template<typename... Args>
void print (QString fmtstr, Args... args)
{
	formatHelper (fmtstr, args...);
	printToLog (fmtstr);
}

template<typename... Args>
void fprint (FILE* fp, QString fmtstr, Args... args)
{
	formatHelper (fmtstr, args...);
	fprintf (fp, "%s", qPrintable (fmtstr));
}


template<typename... Args>
void fprint (QIODevice& dev, QString fmtstr, Args... args)
{
	formatHelper (fmtstr, args...);
	dev.write (fmtstr.toUtf8());
}


// Exactly like print() except no-op in release builds.
template<typename... Args>
#ifndef RELEASE
void dprint (QString fmtstr, Args... args)
{
	formatHelper (fmtstr, args...);
	printToLog (fmtstr);
}
#else
void dprint (QString, Args...) {}
#endif
