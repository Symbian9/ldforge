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

#pragma once
#include "toolset.h"

enum ExtProgramType
{
	Isecalc,
	Intersector,
	Coverer,
	Ytruder,
	Rectifier,
	Edger2,

	NumExternalPrograms,
};

struct ExtProgramInfo
{
	QString name;
};

class ExtProgramToolset : public Toolset
{
	Q_OBJECT

public:
	ExtProgramToolset (MainWindow* parent);

	Q_INVOKABLE void coverer();
	Q_INVOKABLE void edger2();
	Q_INVOKABLE void intersector();
	Q_INVOKABLE void isecalc();
	Q_INVOKABLE void rectifier();
	Q_INVOKABLE void ytruder();
	
	bool programUsesWine (ExtProgramType program);
	QString externalProgramName (ExtProgramType program);
	QString getPathSetting (ExtProgramType program);
	bool getWineSetting (ExtProgramType program);
	void setPathSetting (ExtProgramType program, QString value);
	void setWineSetting (ExtProgramType program, bool value);

private:
	bool checkExtProgramPath(ExtProgramType program);
	bool makeTempFile (class QTemporaryFile& tmp, QString& fname);
	bool runExtProgram (ExtProgramType prog, QString argvstr);
	QString errorCodeString (ExtProgramType program, class QProcess& process);
	void insertOutput (QString fname, bool replace, QList<LDColor> colorsToReplace);
	void writeColorGroup (LDColor color, QString fname);
	void writeObjects (const LDObjectList& objects, QFile& f);
	void writeObjects (const LDObjectList& objects, QString fname);
	void writeSelection (QString fname);

	ExtProgramInfo extProgramInfo[NumExternalPrograms];
};
