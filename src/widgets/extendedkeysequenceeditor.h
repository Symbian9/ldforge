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
#include <QWidget>

/*
 * A widget which contains not only a key sequence editor, but also buttons for resetting and
 * clearing the key sequence.
 */
class ExtendedKeySequenceEditor : public QWidget
{
	Q_OBJECT

public:
	explicit ExtendedKeySequenceEditor(
		const QKeySequence& initialSequence = {},
		const QKeySequence& defaultSequence = {},
		QWidget *parent = nullptr);
	~ExtendedKeySequenceEditor();

	QKeySequence keySequence() const;

public slots:
	void setKeySequence(const QKeySequence& newSequence);
	void clear();

private:
	const QKeySequence defaultSequence;
	class QKeySequenceEdit* editor;
	class QPushButton* clearButton;
	class QPushButton* resetButton;
	class QHBoxLayout* layout;
};
