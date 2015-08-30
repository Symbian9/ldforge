/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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

#include "../mainwindow.h"
#include "algorithmtoolset.h"
#include "basictoolset.h"
#include "extprogramtoolset.h"
#include "filetoolset.h"
#include "movetoolset.h"
#include "toolset.h"
#include "viewtoolset.h"

Toolset::Toolset (MainWindow* parent) :
	QObject (parent),
	m_window (parent) {}

QVector<Toolset*> Toolset::createToolsets (MainWindow* parent)
{
	QVector<Toolset*> tools;
	tools << new AlgorithmToolset (parent);
	tools << new BasicToolset (parent);
	tools << new ExtProgramToolset (parent);
	tools << new FileToolset (parent);
	tools << new MoveToolset (parent);
	tools << new ViewToolset (parent);
	return tools;
}
