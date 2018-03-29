#include "patterneditor.h"
#include "patternviewer.h"
#include "ui_patterneditor.h"

PatternEditor::PatternEditor(Pattern& pattern, QWidget* parent) :
	QMainWindow {parent},
	ui {*new Ui_PatternEditor},
	viewer {new PatternViewer {this}},
	pattern {pattern}
{
	ui.setupUi(this);
	ui.patternFrame->layout()->addWidget(this->viewer);
}

PatternEditor::~PatternEditor()
{
	delete &this->ui;
}
