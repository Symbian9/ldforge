#include "parser.h"

Script::Parser::Parser(QString text) :
	m_data (text) {}

void Script::Parser::parse()
{
	m_position.reset();
}

bool Script::Parser::next(TokenType desiredType)
{
	SavedPosition oldpos = position();
}

void Script::Parser::mustGetNext(TokenType desiredType)
{

}

bool Script::Parser::peekNext(Token& tok)
{

}


const Script::SavedPosition& Script::Parser::position() const
{
	return m_position;
}

void Script::Parser::setPosition(const SavedPosition& pos)
{
	m_position = pos;
}
