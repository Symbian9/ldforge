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

class QDir;
class Configuration;

class LDPaths : public QObject
{
	Q_OBJECT

public:
	LDPaths(Configuration* config, QObject* parent = nullptr);
	void checkPaths();
	bool isValid (const class QDir& path) const;

	static QDir& baseDir();
	static QString& ldConfigPath();
	static QDir& primitivesDir();
	static QDir& partsDir();

public slots:
	bool configurePaths (QString path);

private:
	Configuration* m_config;
	mutable QString m_error;
	class LDrawPathDialog* m_dialog;
};