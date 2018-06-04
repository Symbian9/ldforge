#pragma once
#include "modelobject.h"

class LDCylinder : public LDMatrixObject
{
public:
	static const LDObjectType SubclassType = LDObjectType::Cylinder;

	LDCylinder() = default;
	LDCylinder(int segments, int divisions, const Matrix& transformationMatrix, const Vertex& position);

	virtual LDObjectType type() const override
	{
		return SubclassType;
	}

	virtual QString asText() const override;
	virtual void getVertices(DocumentManager *context, QSet<Vertex>& verts) const override;
	bool isRasterizable() const override { return true; }
	void rasterize(
		DocumentManager* context,
		Winding parentWinding,
		Model& model,
		bool deep,
		bool render
	) override;
	QVector<LDPolygon> rasterizePolygons(DocumentManager* context, Winding parentWinding) override;
	QString objectListText() const override;
	int triangleCount(DocumentManager*) const override { return 2 * m_segments; }
	QString typeName() const override { return "cylinder"; }
	void serialize(class Serializer& serializer) override;

private:
	QString buildFilename() const;
	void buildPrimitiveBody(Model& model, Winding winding = CounterClockwise) const;

	int m_segments;
	int m_divisions;
};
