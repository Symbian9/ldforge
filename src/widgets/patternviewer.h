#pragma once
#include <QWidget>
#include "patterneditor.h"

class PatternViewer : public QWidget
{
	Q_OBJECT

public:
	PatternViewer(PatternEditor* parent);

signals:
public slots:
protected:
	void paintEvent(QPaintEvent* event) override;

private:
	class PatternEditor* const editor;
	QTransform transformation = {};
};
