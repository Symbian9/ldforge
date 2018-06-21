#pragma once
#include <QWidget>
#include "../basics.h"

class CircularSectionEditor : public QWidget
{
	Q_OBJECT

public:
	explicit CircularSectionEditor(QWidget *parent = 0);
	~CircularSectionEditor();

	CircularSection section();
	void setSection(const CircularSection& newSection);

signals:
	void sectionChanged(CircularSection);

private:
	class Ui_CircularSectionEditor& ui;
	int previousDivisions;

	Q_SLOT void updateFractionLabel();
	Q_SLOT void segmentsChanged();
	Q_SLOT void divisionsChanged();
};
