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
#include "toolset.h"

class FileToolset : public Toolset
{
	Q_OBJECT

public:
	FileToolset (class MainWindow* parent);

	Q_INVOKABLE void about();
	Q_INVOKABLE void aboutQt();
	Q_INVOKABLE void downloadFrom();
	Q_INVOKABLE void close();
	Q_INVOKABLE void closeAll();
	Q_INVOKABLE void exit();
	Q_INVOKABLE void exportTo();
	Q_INVOKABLE void help();
	Q_INVOKABLE void insertFrom();
	Q_INVOKABLE void makePrimitive();
	Q_INVOKABLE void newFile();
	Q_INVOKABLE void newPart();
	Q_INVOKABLE void open();
	Q_INVOKABLE void openSubfiles();
	Q_INVOKABLE void save();
	Q_INVOKABLE void saveAll();
	Q_INVOKABLE void saveAs();
	Q_INVOKABLE void scanPrimitives();
	Q_INVOKABLE void setLDrawPath();
	Q_INVOKABLE void settings();
};
