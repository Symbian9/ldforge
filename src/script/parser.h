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

#pragma once
#include "../main.h"
#include "ast.h"

namespace Script
{
	enum TokenType
	{
		TOK_If,
		TOK_Then,
		TOK_Else,
		TOK_EndIf,
		TOK_EndMacro,
		TOK_Macro,
		TOK_For,
		TOK_While,
		TOK_Done,
		TOK_Do,
		TOK_DoubleEquals,		// ==
		TOK_AngleLeftEquals,	// <=
		TOK_AngleRightEquals,	// >=
		TOK_DoubleAmperstand,	// &&
		TOK_DoubleBar,			// ||
		TOK_NotEquals,			// !=
		TOK_Colon,				// :
		TOK_Semicolon,			// ;
		TOK_Dot,				// .
		TOK_Comma,				// ,
		TOK_Equals,				// =
		TOK_AngleLeft,			// <
		TOK_AngleRight,			// >
		TOK_QuestionMark,		// ?
		TOK_BraceLeft,			// {
		TOK_BraceRight,			// }
		TOK_BracketLeft,		// [
		TOK_BracketRight,		// ]
		TOK_ParenLeft,			// (
		TOK_ParenRight,			// )
		TOK_Minus,				// -
		TOK_Plus,				// +
		TOK_Asterisk,			// *
		TOK_Slash,				// /
		TOK_Backslash,			// \.
		TOK_Amperstand,			// &
		TOK_Caret,				// ^
		TOK_Bar,				// |
		TOK_Exclamation,		// !
		TOK_At,					// @
		TOK_Pound,				// #
		TOK_Tilde,				// ~
		TOK_GraveAccent,		// `
		TOK_Percent,			// %
		TOK_Variable,			// $var
		TOK_String,				// "foo"
		TOK_Symbol,				// bar
		TOK_Number,				// 42
		TOK_Any					// for next() and friends, a token never has this
	};

	enum
	{
		LastNamedToken = TOK_Percent
	};

	enum Function
	{
		FUNC_Abs,
		FUNC_Print,
		FUNC_Typeof,
	};

	struct Token
	{
		TokenType type;
		QString text;
		qint32 number;
		qreal real;
	};

	struct State
	{
		int position;
		int lineNumber;
		Token token;

		void reset()
		{
			position = 0;
			lineNumber = 1;
			token.number = token.real = 0;
			token.text.clear();
			token.type = TOK_Any;
		}
	};

	class ParseError
	{
	public:
		ParseError (QString text) : m_text (text) {}
		const QString& message() const { return m_text; }

	private:
		QString m_text;
	};

	class Parser
	{
	public:
		Parser(QString text);
		~Parser();

		bool isAtEnd() const { return m_state.position >= m_data.length(); }
		void mustGetNext (TokenType desiredType = TOK_Any);
		bool next (TokenType desiredType = TOK_Any);
		void parse();
		bool peekNext (Token& tok);
		void preprocess();
		QString preprocessedScript() const { return QString::fromAscii (m_data); }
		char read();
		void scriptError (QString text);
		void setState (const State& pos);
		void skipSpace();
		State state() const { return m_state; }
		void tokenMustBe (TokenType desiredType);
		bool tryMatch (const char* text, bool caseSensitive);
		void unread();

		template<typename... Args>
		void scriptError (QString text, Args... args)
		{
			scriptError (format (text, args...));
		}

	private:
		QString m_script;
		QByteArray m_data;
		QVector<int> m_lineEndings;
		State m_state;
		Ast::RootPointer m_astRoot;
		Token m_rejectedToken;

		void parseString();
		bool parseNumber();
		QString parseEscapeSequence();
		QString parseIdentifier();
		bool getNextToken();
	};
}
