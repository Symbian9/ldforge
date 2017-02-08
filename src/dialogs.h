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
#include <QDialog>
#include "main.h"
#include "basics.h"

class Ui_ExtProgPath;
class QRadioButton;
class QCheckBox;
class QProgressBar;
class QGroupBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QSpinBox;
class RadioGroup;
class QLabel;
class QAbstractButton;
class Ui_OverlayUI;
class Ui_LDPathUI;
class Ui_OpenProgressUI;

// =============================================================================
class ExtProgPathPrompt : public QDialog
{
	Q_OBJECT

public:
	explicit ExtProgPathPrompt (QString progName, QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~ExtProgPathPrompt();
	QString getPath() const;

public slots:
	void findPath();

private:
	Ui_ExtProgPath* ui;
};
