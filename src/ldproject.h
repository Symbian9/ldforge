#pragma once
#include "main.h"

using LDProjectPtr = QSharedPointer<class LDProject>;

class LDProject
{
public:
	LDProject (const LDProject&) = delete;
	~LDProject();

	bool save (const QString& filename);

	void operator= (const LDProject&) = delete;
	static LDProjectPtr LoadFromFile (const QString& filename);
	static LDProjectPtr NewProject();

private:
	QString m_filePath;
	QList<LDDocumentPtr> m_documents;
	LDProject();
};

