#pragma once
#include "../main.h"
#include "ast.h"

namespace Script
{
	enum TokenType
	{
		TOK_DoubleEquals,		// ==
		TOK_AngleLeftEquals,	// <=
		TOK_AngleRightEquals,	// >=
		TOK_DoubleAmperstand,	// &&
		TOK_DoubleBar,			// ||
		TOK_Dollar,				// $
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
		TOK_String,				// "foo"
		TOK_Symbol,				// bar
		TOK_Number,				// 42
		TOK_Any					// for next() and friends, a token never has this
	};

	enum
	{
		LastNamedToken = TOK_Percent
	};

	enum Type
	{
		TYPE_Var, // mixed
		TYPE_Int,
		TYPE_Real,
		TYPE_String,
		TYPE_Type, // heh
		TYPE_Vertex,
		TYPE_Object,
		TYPE_Line,
		TYPE_OptLine,
		TYPE_Triangle,
		TYPE_Quad,
		TYPE_Reference,
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
	};

	struct SavedPosition
	{
		int position;
		int lineNumber;

		void reset()
		{
			position = 0;
			lineNumber = 1;
		}
	};

	class ParseError : public std::exception
	{
	public:
		ParseError(QString text) : m_text (text) {}
		const char* what() const { return m_text; }

	private:
		QString m_text;
	};

	class Parser
	{
	public:
		Parser(QString text);
		~Parser();

		void parse();
		void scriptError(QString text) { throw ParseError(text); }
		bool next(TokenType desiredType = TOK_Any);
		void mustGetNext(TokenType desiredType = TOK_Any);
		bool peekNext(Token& tok);
		const SavedPosition& position() const;
		void setPosition(const SavedPosition& pos);
		QString preprocessedScript() const { return QString::fromAscii (m_data); }

	private:
		QString m_script;
		QByteArray m_data;
		QVector<int> m_lineEndings;
		SavedPosition m_position;
		Token m_token;
		AstNode* m_astRoot;
	};
}
