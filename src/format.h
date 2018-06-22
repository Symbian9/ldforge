/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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
#include <QModelIndex>
#include "basics.h"
#include "colors.h"
#include "types/vertex.h"

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

				m_text += StringFormatArg {matrix(row, column)}.text();
			}

			m_text += "}";
		}
		m_text += "}";
	}

	StringFormatArg(const QModelIndex& index)
	{
		m_text = QString("{%1, %2}").arg(index.row(), index.column());
	}

	inline QString text() const
	{
		return m_text;
	}

private:
	QString m_text;
};

// Format the message with the given args.
//
// The formatting ultimately uses String's arg() method to actually format the args so the format string should be
// prepared accordingly, with %1 referring to the first arg, %2 to the second, etc.
template<typename T, typename... Rest>
QString format(const QString& formatString, const T& arg, Rest... rest)
{
	return format(formatString.arg(StringFormatArg(arg).text()), rest...);
}

// Recursion base
inline QString format(const QString& formatString)
{
	return formatString;
}

template<typename... Args>
void fprint(FILE* fp, const QString& formatString, Args... args)
{
	fprintf(fp, "%s", qPrintable(format(formatString, args...)));
}


template<typename... Args>
void fprint (QIODevice& dev, const QString& formatString, Args... args)
{
	dev.write(format(formatString, args...).toUtf8());
}

class Printer : public QObject
{
	Q_OBJECT

public:
	void printLine(const QString& line)
	{
		printf("%s\n", line.toUtf8().constData());
		emit linePrinted(line);
	}

signals:
	void linePrinted(const QString& line);
};

/*
 * Format and print the given args to stdout. Also is reflected to the status bar.
 */
template<typename... Args>
void print(QString formatString, Args&&... args)
{
	singleton<Printer>().printLine(format(formatString, args...));
}
