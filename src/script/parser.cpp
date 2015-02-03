/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2015 Teemu Piippo
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

static const char* TokenNames[] =
{
	"if",
	"then",
	"else",
	"endif",
	"endmacro",
	"macro",
	"for",
	"while",
	"done",
	"do",
	"==",
	"<=",
	">=",
	"&&",
	"||",
	"!=",
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
	"<variable>",
	"<string>",
	"<symbol>",
	"<number>",
	"<any>",
};

//
// -------------------------------------------------------------------------------------------------
//
Script::Parser::Parser(QString text) :
	m_script(text) {}

//
// -------------------------------------------------------------------------------------------------
//
Script::Parser::~Parser() {}

//
// -------------------------------------------------------------------------------------------------
//
void Script::Parser::parse()
{
	preprocess();
	m_state.reset();
	m_astRoot = Ast::spawnRoot();

	while (next())
	{
		if (m_state.token.type == TOK_Semicolon)
			continue;

		tokenMustBe (TOK_Macro);
		mustGetNext (TOK_Symbol);
		Ast::MacroPointer macroAst = Ast::spawn<Ast::MacroNode> (m_astRoot, state().token.text);
		mustGetNext (TOK_Semicolon);

		do
		{
			mustGetNext();
		} while (m_state.token.type != TOK_EndMacro);
	}

	m_astRoot->dump();
}

//
// -------------------------------------------------------------------------------------------------
//
void Script::Parser::preprocess()
{
	bool inString = false;
	bool inComment = false;
	bool inBackslash = false;
	int ln = 1;
	int pos = 0;

	// Preprocess
	for (QChar qch : m_script)
	{
		char ch = qch.toAscii();

		if (not inComment and not inString and ch == '\0')
			scriptError ("bad character %1 in script text on line %2", qch, ln);

		if (not inString)
		{
			if (ch == '\\')
			{
				inBackslash = true;
				continue;
			}

			if (inBackslash and ch != '\n')
				scriptError ("misplaced backslash on line %1", ln);
		}

		if (ch == '"')
			inString ^= 1;

		if (ch == '\n')
		{
			if (inString)
				scriptError ("unterminated string on line %1", ln);

			if (not inBackslash)
			{
				m_data.append (';');
				m_data.append ('\n');
				pos += 2;
				inComment = false;
				m_lineEndings << pos;
				++ln;
			}
			else
				inBackslash = false;

			continue;
		}

		if (ch == '#' and not inString)
		{
			inComment = true;
			continue;
		}

		if (not inComment)
		{
			m_data.append (ch);
			++pos;
		}
	}
}

//
// -------------------------------------------------------------------------------------------------
//
namespace Script
{
	class UnexpectedEOF : public std::exception
	{
		const char* what() const throw()
		{
			return "unexpected EOF";
		}
	};
}

//
// -------------------------------------------------------------------------------------------------
//
char Script::Parser::read()
{
	if (m_state.position >= m_data.length())
		throw UnexpectedEOF();

	char ch = m_data[m_state.position];
	m_state.position++;

	if (m_state.position == m_lineEndings[m_state.lineNumber])
		m_state.lineNumber++;

	return ch;
}

//
// -------------------------------------------------------------------------------------------------
//
void Script::Parser::unread()
{
	if (m_state.position <= 0)
		return;

	if (m_state.lineNumber > 0
		and m_state.position == m_lineEndings[m_state.lineNumber - 1])
	{
		m_state.lineNumber--;
	}

	m_state.position--;
}

//
// -------------------------------------------------------------------------------------------------
//
// Takes a hexadecimal character and returns its numerical value. It is assumed that isxdigit(xd)
// is true (if not, result is undefined).
//
int parseXDigit (char xd)
{
	if (xd >= 'a')
		return xd - 'a';

	if (xd >= 'A')
		return xd - 'A';

	return xd - '0';
}

//
// -------------------------------------------------------------------------------------------------
//
bool Script::Parser::next (TokenType desiredType)
{
	SavedState oldpos = state();
	Token oldtoken = m_state.token;

	if (not getNextToken())
		return false;

	if (desiredType != TOK_Any and m_state.token.type != desiredType)
	{
		// Did not find the token we wanted, revert back
		m_rejectedToken = m_state.token;
		m_state.token = oldtoken;
		setState (oldpos);
		return false;
	}

	return true;
}

//
// -------------------------------------------------------------------------------------------------
//
bool Script::Parser::getNextToken()
{
	try
	{
		m_state.token.text.clear();
		m_state.token.number = 0;
		m_state.token.real = 0.0;
		skipSpace();

		const char* data = m_data.constData() + m_state.position;

		// Does this character start one of our tokens?
		for (int tt = 0; tt <= LastNamedToken; ++tt)
		{
			if (strncmp (data, TokenNames[tt], strlen (TokenNames[tt])) != 0)
				continue;

			m_state.position += strlen (TokenNames[tt]);
			m_state.token.text = QString::fromAscii (TokenNames[tt]);
			m_state.token.type = TokenType (tt);
			return true;
		}

		// Check for number
		if (parseNumber())
			return true;

		// Check for string
		if (*data == '"')
		{
			read();
			parseString();
			return true;
		}

		// Check for variable
		if (*data == '$')
		{
			read();
			m_state.token.text = parseIdentifier();
			m_state.token.type = TOK_Variable;
			return true;
		}

		// Must be a symbol of some sort then
		m_state.token.text = parseIdentifier();
		m_state.token.type = TOK_Symbol;
	}
	catch (UnexpectedEOF)
	{
		return false;
	}

	return true;
}

//
// -------------------------------------------------------------------------------------------------
//
bool Script::Parser::parseNumber()
{
	SavedState pos = state();
	char ch = read();
	unread();
	QString numberString;

	if (not isdigit (ch) and ch != '.')
	{
		setState (pos);
		return false;
	}

	int base = 10;
	bool gotDot = false;

	if (tryMatch ("0x", false))
		base = 16;
	elif (tryMatch ("0b", false))
		base = 2;

	int (*checkFunc)(int) = base == 16 ? isxdigit : isdigit;

	for (int n = 0; not isspace (ch = read()); ++n)
	{
		if (n == 0 && ch == '0')
			base = 8;

		if (ch == '.')
		{
			if (gotDot)
				scriptError ("multiple dots in numeric literal");

			// If reading numbers like 0.1234 where the first number is zero, the parser
			// will initially think the number is octal so we must take that into account here.
			// Note that even if you have numbers like 05.612, it will still be decimal.
			if (base != 10 and base != 8)
				scriptError ("real number constant must be decimal");

			base = 10;
			gotDot = true;
		}
		else if (checkFunc (ch))
		{
			if (base <= 10 and (ch - '0') >= base)
				scriptError ("bad base-%1 numeric literal", base);

			numberString += ch;
		}
		else if (isalpha (ch))
			scriptError ("invalid digit %1 in literal", ch);
		else
			break;
	}

	unread();
	bool ok;

	if (gotDot)
	{
		// Floating point number
		m_state.token.real = numberString.toFloat (&ok);
		m_state.token.number = m_state.token.real;
	}
	else
	{
		// Integral number
		m_state.token.number = numberString.toInt (&ok, base);
		m_state.token.real = m_state.token.number;
	}

	if (ok == false)
		scriptError ("invalid numeric literal '%1'", numberString);

	m_state.token.text = numberString;
	m_state.token.type = TOK_Number;

	return true;
}

//
// -------------------------------------------------------------------------------------------------
//
void Script::Parser::scriptError (QString text)
{
	throw ParseError (text);
}

//
// -------------------------------------------------------------------------------------------------
//
// Checks whether the parser is at the beginning of the given string in the code. The string is
// expected not to have newlines. If true, the parser jumps over the text.
//
bool Script::Parser::tryMatch (const char* text, bool caseSensitive)
{
	assert (strstr (text, "\n") == NULL);
	const char* data = m_data.constData() + m_state.position;
	int (*func) (const char*, const char*) = caseSensitive ? &strcmp : &strcasecmp;

	if ((*func) (data, text) == 0)
	{
		m_state.position += strlen (text);
		return true;
	}

	return false;
}

//
// -------------------------------------------------------------------------------------------------
//
QString Script::Parser::parseEscapeSequence()
{
	char ch = read();
	QString result;

	switch (ch)
	{
	case '"':
		result += "\"";
		break;

	case 'n':
		result += "\n";
		break;

	case 't':
		result += "\t";
		break;

	case '\\':
		result += "\\";
		break;

	case 'x':
	case 'X':
		{
			char n1 = read();
			char n2 = read();

			if (not isxdigit(n1) or not isxdigit(n2))
				scriptError ("bad hexa-decimal character \\x%1%2", n1, n2);

			unsigned char num = parseXDigit(n1) * 16 + parseXDigit(n2);
			result += char (num);
		}
		break;

	default:
		scriptError ("unknown escape sequence \\%1", ch);
	}

	return result;
}

//
// -------------------------------------------------------------------------------------------------
//
void Script::Parser::parseString()
{
	m_state.token.type = TOK_String;
	m_state.token.text.clear();

	try
	{
		char ch;

		while ((ch = read()) != '"')
		{
			if (ch == '\\')
				m_state.token.text += parseEscapeSequence();
			else
				m_state.token.text += ch;
		}
	}
	catch (UnexpectedEOF)
	{
		scriptError ("unterminated string");
	}
}

//
// -------------------------------------------------------------------------------------------------
//
void Script::Parser::skipSpace()
{
	while (isspace (read()))
		;

	unread();
}

//
// -------------------------------------------------------------------------------------------------
//
void Script::Parser::mustGetNext (TokenType desiredType)
{
	if (not next (desiredType))
	{
		scriptError ("Expected %1, got %2",
			TokenNames[m_rejectedToken.type],
			TokenNames[desiredType]);
	}
}

//
// -------------------------------------------------------------------------------------------------
//
bool Script::Parser::peekNext (Token& tok)
{
	SavedState pos = state();

	if (next (TOK_Any))
	{
		tok = m_state.token;
		setState (pos);
		return true;
	}

	return false;
}

//
// -------------------------------------------------------------------------------------------------
//
const Script::SavedState& Script::Parser::state() const
{
	return m_state;
}

//
// -------------------------------------------------------------------------------------------------
//
void Script::Parser::setState (const SavedState& pos)
{
	m_state = pos;
}

//
// -------------------------------------------------------------------------------------------------
//
QString Script::Parser::parseIdentifier()
{
	char ch;
	QString identifier;

	while (not isspace (ch = read()))
	{
		if (isalnum (ch) == false and ch != '_')
			break;

		identifier += QChar::fromAscii (ch);
	}

	unread();
	return identifier;
}

//
// -------------------------------------------------------------------------------------------------
//
void Script::Parser::tokenMustBe (TokenType desiredType)
{
	if (m_state.token.type != desiredType)
	{
		scriptError ("expected %1, got %2",
			TokenNames[desiredType],
			TokenNames[m_state.token.type]);
	}
}
