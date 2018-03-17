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

#include "parser.h"
#include "lddocument.h"
#include "linetypes/comment.h"
#include "linetypes/conditionaledge.h"
#include "linetypes/edgeline.h"
#include "linetypes/empty.h"
#include "linetypes/quadrilateral.h"
#include "linetypes/triangle.h"

/*
 * Constructs an LDraw parser
 */
Parser::Parser(QIODevice& device, QObject* parent) :
	QObject {parent},
	device {device} {}

/*
 * Reads a single line from the device.
 */
QString Parser::readLine()
{
	return QString::fromUtf8(this->device.readLine()).trimmed();
}

const QMap<QString, decltype(LDHeader::type)> Parser::typeStrings {
	{"Part", LDHeader::Part},
	{"Subpart", LDHeader::Subpart},
	{"Shortcut", LDHeader::Shortcut},
	{"Primitive", LDHeader::Primitive},
	{"8_Primitive", LDHeader::Primitive_8},
	{"48_Primitive", LDHeader::Primitive_48},
	{"Configuration", LDHeader::Configuration},
};

/*
 * Parses a single line of the header.
 * Possible parse results:
 *   · ParseSuccess: the header line was parsed successfully.
 *   · ParseFailure: the header line was parsed incorrectly and needs to be handled otherwise.
 *   · StopParsing: the line does not belong in the header and header parsing needs to stop.
 */
Parser::HeaderParseResult Parser::parseHeaderLine(LDHeader& header, const QString& line)
{
	if (line.isEmpty())
	{
		return ParseSuccess;
	}
	else if (not line.startsWith("0") or line.startsWith("0 //"))
	{
		return StopParsing;
	}
	else if (line.startsWith("0 !LDRAW_ORG "))
	{
		QStringList tokens = line
			.mid(strlen("0 !LDRAW_ORG "))
			.split(" ", QString::SkipEmptyParts);

		if (not tokens.isEmpty())
		{
			QString partTypeString = tokens[0];
			// Anything that enters LDForge becomes unofficial in any case if saved.
			// Therefore we don't need to give the Unofficial type any special
			// consideration.
			if (partTypeString.startsWith("Unofficial_"))
				partTypeString = partTypeString.mid(strlen("Unofficial_"));
			header.type = Parser::typeStrings.value(partTypeString, LDHeader::Part);
			header.qualfiers = 0;
			if (tokens.contains("Alias"))
				header.qualfiers |= LDHeader::Alias;
			if (tokens.contains("Physical_Color"))
				header.qualfiers |= LDHeader::Physical_Color;
			if (tokens.contains("Flexible_Section"))
				header.qualfiers |= LDHeader::Flexible_Section;
			return ParseSuccess;
		}
		else
		{
			return ParseFailure;
		}
	}
	else if (line == "0 BFC CERTIFY CCW")
	{
		header.winding = CounterClockwise;
		return ParseSuccess;
	}
	else if (line == "0 BFC CERTIFY CW")
	{
		header.winding = Clockwise;
		return ParseSuccess;
	}
	else if (line == "0 BFC NOCERTIFY")
	{
		header.winding = NoWinding;
		return ParseSuccess;
	}
	else if (line.startsWith("0 !HISTORY "))
	{
		static const QRegExp historyRegexp {
			R"(0 !HISTORY\s+(\d{4}-\d{2}-\d{2})\s+)"
			R"((\{[^}]+|\[[^]]+)[\]}]\s+(.+))"
		};
		if (historyRegexp.exactMatch(line))
		{
			QString dateString = historyRegexp.capturedTexts().value(1);
			QString authorWithPrefix = historyRegexp.capturedTexts().value(2);
			QString description = historyRegexp.capturedTexts().value(3);
			LDHeader::HistoryEntry historyEntry;
			historyEntry.date = QDate::fromString(dateString, Qt::ISODate);
			historyEntry.description = description;

			if (authorWithPrefix[0] == '{')
				historyEntry.author = authorWithPrefix + "}";
			else
				historyEntry.author = authorWithPrefix.mid(1);

			header.history.append(historyEntry);
			return ParseSuccess;
		}
		else
		{
			return ParseFailure;
		}
	}
	else if (line.startsWith("0 Author: "))
	{
		header.author = line.mid(strlen("0 Author: "));
		return ParseSuccess;
	}
	else if (line.startsWith("0 Name: "))
	{
		header.name = line.mid(strlen("0 Name: "));
		return ParseSuccess;
	}
	else if (line.startsWith("0 !HELP "))
	{
		if (not header.help.isEmpty())
			header.help += "\n";
		header.help += line.mid(strlen("0 !HELP "));
		return ParseSuccess;
	}
	else if (line.startsWith("0 !KEYWORDS "))
	{
		if (not header.keywords.isEmpty())
			header.keywords += "\n";
		header.keywords += line.mid(strlen("0 !KEYWORDS "));
		return ParseSuccess;
	}
	else if (line.startsWith("0 !CATEGORY "))
	{
		header.category = line.mid(strlen("0 !CATEGORY "));
		return ParseSuccess;
	}
	else if (line.startsWith("0 !CMDLINE "))
	{
		header.cmdline = line.mid(strlen("0 !CMDLINE "));
		return ParseSuccess;
	}
	else if (line.startsWith("0 !LICENSE Redistributable under CCAL version 2.0"))
	{
		header.license = LDHeader::CaLicense;
		return ParseSuccess;
	}
	else if (line.startsWith("0 !LICENSE Not redistributable"))
	{
		header.license = LDHeader::NonCaLicense;
		return ParseSuccess;
	}
	else
	{
		return ParseFailure;
	}
}

/*
 * Parses the header from the device given at construction and returns
 * the resulting header structure.
 */
LDHeader Parser::parseHeader()
{
	LDHeader header = {};

	if (not this->device.atEnd())
	{
		// Parse the description
		QString descriptionLine = this->readLine();
		if (descriptionLine.startsWith("0 "))
		{
			header.description = descriptionLine.mid(strlen("0 ")).trimmed();

			// Parse the rest of the header
			while (not this->device.atEnd())
			{
				const QString& line = this->readLine();
				auto result = parseHeaderLine(header, line);

				if (result == ParseFailure)
				{
					// Failed to parse this header line, add it as a comment into the body later.
					this->bag.append(line);
				}
				else if (result == StopParsing)
				{
					// Header parsing stops, add this line to the body.
					this->bag.append(line);
					break;
				}
			}
		}
		else
		{
			this->bag.append(descriptionLine);
		}
	}

	return header;
}

/*
 * Parses the model body into the given model.
 */
void Parser::parseBody(Model& model)
{
	bool invertNext = false;

	while (not this->device.atEnd())
		this->bag.append(this->readLine());

	for (const QString& line : this->bag)
	{
		if (line == "0 BFC INVERTNEXT" or line == "0 BFC CERTIFY INVERTNEXT")
		{
			invertNext = true;
			continue;
		}

		LDObject* object = parseFromString(model, model.size(), line);

		/*
		// Check for parse errors and warn about them
		if (obj->type() == LDObjectType::Error)
		{
			emit parseErrorMessage(format(
				tr("Couldn't parse line #%1: %2"),
				progress() + 1, static_cast<LDError*> (obj)->reason()));
			++m_warningCount;
		}
		*/

		if (invertNext and object->type() == LDObjectType::SubfileReference)
			object->setInverted(true);

		invertNext = false;
	}
}

// =============================================================================
//
static void CheckTokenCount (const QStringList& tokens, int num)
{
	if (countof(tokens) != num)
		throw QString (format ("Bad amount of tokens, expected %1, got %2", num, countof(tokens)));
}

// =============================================================================
//
static void CheckTokenNumbers (const QStringList& tokens, int min, int max)
{
	bool ok;
	QRegExp scientificRegex ("\\-?[0-9]+\\.[0-9]+e\\-[0-9]+");

	for (int i = min; i <= max; ++i)
	{
		// Check for floating point
		tokens[i].toDouble (&ok);
		if (ok)
			return;

		// Check hex
		if (tokens[i].startsWith ("0x"))
		{
			tokens[i].mid (2).toInt (&ok, 16);

			if (ok)
				return;
		}

		// Check scientific notation, e.g. 7.99361e-15
		if (scientificRegex.exactMatch (tokens[i]))
			return;

		throw QString (format ("Token #%1 was `%2`, expected a number (matched length: %3)",
			(i + 1), tokens[i], scientificRegex.matchedLength()));
	}
}

static Vertex parseVertex(QStringList& tokens, const int n)
{
	return {tokens[n].toDouble(), tokens[n + 1].toDouble(), tokens[n + 2].toDouble()};
}

/*
 * Parses the given model body line and inserts the result into the given model.
 * The resulting object is also returned.
 *
 * If an error happens, an error object is created. Result is guaranteed to be a valid pointer.
 *
 * TODO: rewrite this using regular expressions
 */
LDObject* Parser::parseFromString(Model& model, int position, QString line)
{
	if (position == EndOfModel)
		position = model.size();

	try
	{
		QStringList tokens = line.split(" ", QString::SkipEmptyParts);

		if (tokens.isEmpty())
		{
			// Line was empty, or only consisted of whitespace
			return model.emplaceAt<LDEmpty>(position);
		}

		if (countof(tokens[0]) != 1 or not tokens[0][0].isDigit())
			throw QString ("Illogical line code");

		int num = tokens[0][0].digitValue();

		switch (num)
		{
			case 0:
			{
				// Comment
				QString commentText = line.mid(line.indexOf("0") + 2);
				QString commentTextSimplified = commentText.trimmed();

				// Handle BFC statements
				if (countof(tokens) > 2 and tokens[1] == "BFC")
				{
					for (BfcStatement statement : iterateEnum<BfcStatement>())
					{
						if (commentTextSimplified == format("BFC %1", LDBfc::statementToString(statement)))
							return model.emplaceAt<LDBfc>(position, statement);
					}

					// handle MLCAD nonsense
					if (commentTextSimplified == "BFC CERTIFY CLIP")
						return model.emplaceAt<LDBfc>(position, BfcStatement::Clip);
					else if (commentTextSimplified == "BFC CERTIFY NOCLIP")
						return model.emplaceAt<LDBfc>(position, BfcStatement::NoClip);
				}

				if (countof(tokens) > 2 and tokens[1] == "!LDFORGE")
				{
					// Handle LDForge-specific types, they're embedded into comments too
					if (tokens[2] == "BEZIER_CURVE")
					{
						CheckTokenCount (tokens, 16);
						CheckTokenNumbers (tokens, 3, 15);
						LDBezierCurve* obj = model.emplaceAt<LDBezierCurve>(position);
						obj->setColor(tokens[3].toInt(nullptr, 0));

						for (int i = 0; i < 4; ++i)
							obj->setVertex (i, parseVertex (tokens, 4 + (i * 3)));

						return obj;
					}
				}

				// Just a regular comment:
				return model.emplaceAt<LDComment>(position, commentText);
			}

			case 1:
			{
				// Subfile
				CheckTokenCount (tokens, 15);
				CheckTokenNumbers (tokens, 1, 13);

				Vertex referncePosition = parseVertex (tokens, 2);  // 2 - 4
				Matrix transform;

				for (int i = 0; i < 9; ++i)
					transform.value(i) = tokens[i + 5].toDouble(); // 5 - 13

				LDSubfileReference* obj = model.emplaceAt<LDSubfileReference>(position, tokens[14], transform, referncePosition);
				obj->setColor (tokens[1].toInt(nullptr, 0));
				return obj;
			}

			case 2:
			{
				CheckTokenCount (tokens, 8);
				CheckTokenNumbers (tokens, 1, 7);

				// Line
				LDEdgeLine* obj = model.emplaceAt<LDEdgeLine>(position);
				obj->setColor (tokens[1].toInt(nullptr, 0));

				for (int i = 0; i < 2; ++i)
					obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 7

				return obj;
			}

			case 3:
			{
				CheckTokenCount (tokens, 11);
				CheckTokenNumbers (tokens, 1, 10);

				// Triangle
				LDTriangle* obj = model.emplaceAt<LDTriangle>(position);
				obj->setColor (tokens[1].toInt(nullptr, 0));

				for (int i = 0; i < 3; ++i)
					obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 10

				return obj;
			}

			case 4:
			case 5:
			{
				CheckTokenCount (tokens, 14);
				CheckTokenNumbers (tokens, 1, 13);

				// Quadrilateral / Conditional line
				LDObject* obj;

				if (num == 4)
					obj = model.emplaceAt<LDQuadrilateral>(position);
				else
					obj = model.emplaceAt<LDConditionalEdge>(position);

				obj->setColor (tokens[1].toInt(nullptr, 0));

				for (int i = 0; i < 4; ++i)
					obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 13

				return obj;
			}

			default:
				throw QString {"Unknown line code number"};
		}
	}
	catch (QString& errorMessage)
	{
		// Strange line we couldn't parse
		return model.emplaceAt<LDError>(position, line, errorMessage);
	}
}
