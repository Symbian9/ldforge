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

#include <QTimer>
#include <QDate>
#include "messageLog.h"
#include "glrenderer.h"
#include "mainwindow.h"

enum
{
	MaxMessages = 5,
	ExpireTime = 5000,
	FadeTime = 500
};

// -------------------------------------------------------------------------------------------------
//
MessageManager::MessageManager(QObject* parent) :
    QObject {parent}
{
	m_ticker = new QTimer;
	m_ticker->start (100);
	connect (m_ticker, SIGNAL (timeout()), this, SLOT (tick()));
}

// -------------------------------------------------------------------------------------------------
//
MessageManager::Line::Line (QString text) :
	text (text),
	alpha (1.0f),
	expiry (QDateTime::currentDateTime().addMSecs (ExpireTime)) {}

// -------------------------------------------------------------------------------------------------
//
bool MessageManager::Line::update (bool& changed)
{
	changed = false;
	QDateTime now = QDateTime::currentDateTime();
	int msec = now.msecsTo (expiry);

	if (now >= expiry)
	{
		// Message line has expired
		changed = true;
		return false;
	}

	if (msec <= FadeTime)
	{
		// Message line has not expired but is fading out
		alpha = ( (float) msec) / FadeTime;
		changed = true;
	}

	return true;
}

// -------------------------------------------------------------------------------------------------
//
//	Add a line to the message manager.
//
void MessageManager::addLine (QString line)
{
	// If there's too many entries, pop the excess out
	while (countof(m_lines) >= MaxMessages)
		m_lines.removeFirst();

	m_lines.append(Line {line});
	emit changed();
}

// -------------------------------------------------------------------------------------------------
//
//	Ticks the message manager. All lines are ticked and the renderer scene is redrawn if something
//	changed.
//
void MessageManager::tick()
{
	if (m_lines.isEmpty())
		return;

	bool changed = false;

	for (int i = 0; i < countof(m_lines); ++i)
	{
		bool lineChanged;

		if (not m_lines[i].update (lineChanged))
			m_lines.removeAt (i--);

		changed |= lineChanged;
	}

	if (changed and parent())
		emit this->changed();
}

// =============================================================================
//
const QList<MessageManager::Line>& MessageManager::getLines() const
{
	return m_lines;
}
