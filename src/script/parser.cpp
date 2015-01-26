#include "parser.h"

static const char* TokenNames[] =
{
	"==",
	"<=",
	">=",
	"&&",
	"||",
	"$",
	":",
	";",
	".",
	",",
	"=",
	"<",
	">",
	"?",
	"{",
	"}",
	"[",
	"]",
	"(",
	")",
	"-",
	"+",
	"*",
	"/",
	"\\",
	"&",
	"^",
	"|",
	"!",
	"@",
	"#",
	"~",
	"`",
	"%",
	"<string>",
	"<symbol>",
	"<number>",
	"<any>",
};

Script::Parser::Parser(QString text) :
	m_script(text) {}

void Script::Parser::parse()
{
	bool inString = false;
	bool inComment = false;
	bool inBackslash = false;
	int ln = 1;
	int pos = 0;

	// Preprocess
	for (QChar qch : text)
	{
		char ch = qch.toAscii();

		if (not inComment && not inString && ch == '\0')
			scriptError ("bad character %s in script text on line %d", qch, ln);

		if (ch == '\\')
		{
			inBackslash = true;
			continue;
		}

		if (inBackslash)
		{
			if (inString)
			{
				switch (ch)
				{
				case 'n': data << '\n'; break;
				case 't': data << '\t'; break;
				case 'b': data << '\b'; break;
				case '\\': data << '\\'; break;
				default: scriptError ("misplaced backslash on line %d", ln);
				}

				++pos;
				inBackslash == false;
				continue;
			}
			else if (ch != '\n')
			{
				scriptError ("misplaced backslash on line %d", ln);
			}
		}

		if (ch == '\n')
		{
			if (inString)
				scriptError ("unterminated string on line %d", ln);

			if (not inBackslash)
			{
				m_data << ';';
				++pos;
				inComment = false;
				m_lineEndings << pos;
				++ln;
			}
			else
			{
				inBackslash = false;
			}
			continue;
		}

		if (ch == '#' && not inString)
		{
			inComment = true;
			continue;
		}

		m_data << ch;
		++pos;
	}

	m_position.reset();
}

bool Script::Parser::next(TokenType desiredType)
{
	SavedPosition oldpos = position();
	return false;
}

void Script::Parser::mustGetNext(TokenType desiredType)
{

}

bool Script::Parser::peekNext(Token& tok)
{
	return false;
}

const Script::SavedPosition& Script::Parser::position() const
{
	return m_position;
}

void Script::Parser::setPosition(const SavedPosition& pos)
{
	m_position = pos;
}
