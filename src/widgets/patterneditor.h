#pragma once
#include <QMainWindow>
#include "../main.h"
#include "../types/pattern.h"

class PatternEditor : public QMainWindow
{
	Q_OBJECT

public:
	PatternEditor(Pattern& pattern, QWidget *parent = nullptr);
	~PatternEditor();

signals:

public slots:

private:
	friend class PatternViewer;

	class Ui_PatternEditor& ui;
	class PatternViewer* viewer;
	Pattern& pattern;
	LDColor currentColor = MainColor;
};
