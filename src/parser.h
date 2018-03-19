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
#include "main.h"
#include "lddocument.h"

class LDHeader;
class Model;

class Parser : public QObject
{
	Q_OBJECT

public:
	enum { EndOfModel = -1 };

	Parser(QIODevice& device, QObject* parent = nullptr);

	LDHeader parseHeader(Winding& winding);
	void parseBody(Model& model);

	static LDObject* parseFromString(Model& model, int position, QString line);

	static const QMap<QString, decltype(LDHeader::type)> typeStrings;

/*
signals:
	void parseErrorMessage(QString message);
*/

private:
	enum HeaderParseResult {ParseSuccess, ParseFailure, StopParsing};

	QString readLine();
	HeaderParseResult parseHeaderLine(LDHeader& header, Winding& winding, const QString& line);

	QIODevice& device;
	QStringList bag;
};
