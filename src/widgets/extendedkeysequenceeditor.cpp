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

#include <QKeySequenceEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include "extendedkeysequenceeditor.h"

/*
 * Constructs a new extended key sequence editor.
 */
ExtendedKeySequenceEditor::ExtendedKeySequenceEditor(
	const QKeySequence& initialSequence,
	const QKeySequence& defaultSequence,
	QWidget* parent) :
	QWidget {parent},
	defaultSequence {defaultSequence},
	editor {new QKeySequenceEdit {initialSequence, this}},
	clearButton {new QPushButton {"×", this}},
	resetButton {new QPushButton {"↺", this}},
	layout {new QHBoxLayout {this}}
{
	layout->addWidget(editor, 1);
	layout->addWidget(clearButton, 0);
	layout->addWidget(resetButton, 0);
	layout->setContentsMargins({});
	setLayout(layout);

	// Set up focus proxies so that the actual editing widget gets focused when focus
	// is applied to this widget.
	setFocusProxy(editor);
	clearButton->setFocusProxy(editor);
	resetButton->setFocusProxy(editor);

	connect(clearButton, &QPushButton::clicked, editor, &QKeySequenceEdit::clear);
	connect(resetButton, &QPushButton::clicked, [this]()
	{
		editor->setKeySequence(this->defaultSequence);
	});
}

/*
 * Destroys an extended key sequence editor.
 */
ExtendedKeySequenceEditor::~ExtendedKeySequenceEditor()
{
	delete editor;
	delete clearButton;
	delete resetButton;
	delete layout;
}

/*
 * Returns the current key sequence in the editor.
 */
QKeySequence ExtendedKeySequenceEditor::keySequence() const
{
	return editor->keySequence();
}

/*
 * Changes the key sequence in the editor.
 */
void ExtendedKeySequenceEditor::setKeySequence(const QKeySequence& newSequence)
{
	editor->setKeySequence(newSequence);
}

/*
 * Clears the key sequence.
 */
void ExtendedKeySequenceEditor::clear()
{
	editor->clear();
}
