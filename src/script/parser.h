#pragma once
#include <QString>
#include "ast.h"

namespace Script
{
	enum TokenType
	{
		TOK_Dollar,
		TOK_Semicolon,
		TOK_BraceLeft,
		TOK_BraceRight,
		TOK_BracketLeft,
		TOK_BracketRight,
		TOK_ParenLeft,
		TOK_ParenRight,
		TOK_String,
		TOK_Symbol,
		TOK_Number,
		TOK_Any
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

	private:
		QString m_data;
		SavedPosition m_position;
		Token m_token;
		AstNode* m_astRoot;
	};
}
