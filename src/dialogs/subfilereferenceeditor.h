/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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
#include "../main.h"

class SubfileReferenceEditor : public QDialog
{
	Q_OBJECT

public:
	SubfileReferenceEditor(class LDSubfileReference* reference, QWidget *parent = nullptr);
	~SubfileReferenceEditor();

	void accept() override;
	void setPrimitivesTree(class PrimitiveManager* primitives);

private:
	class Ui_SubfileReferenceEditor& ui;
	class LDSubfileReference* const reference;
	LDColor color;
};
