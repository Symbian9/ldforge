/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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

#ifndef LDFORGE_LDCONFIG_H
#define LDFORGE_LDCONFIG_H

#include "Types.h"
#include <QStringList>

// =============================================================================
// LDConfigParser
//
// String parsing utility for parsing ldconfig.ldr
// =============================================================================
class LDConfigParser
{
	public:
		LDConfigParser (QString inText, char sep);

		bool isAtEnd();
		bool isAtBeginning();
		bool getNextToken (QString& val);
		bool peekNextToken (QString& val);
		bool getToken (QString& val, const int pos);
		bool findToken (int& result, char const* needle, int args);
		int getSize();
		void rewind();
		void seek (int amount, bool rel);
		bool tokenCompare (int inPos, const char* sOther);

		inline QString operator[] (const int idx)
		{
			return m_tokens[idx];
		}

	private:
		QStringList m_tokens;
		int m_pos;
};

void parseLDConfig();

#endif // LDFORGE_LDCONFIG_H
