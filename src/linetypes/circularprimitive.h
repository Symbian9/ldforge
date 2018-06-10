#pragma once
#include "../primitives.h"
#include "modelobject.h"

class LDCircularPrimitive : public LDMatrixObject
{
public:
	static const LDObjectType SubclassType = LDObjectType::CircularPrimitive;

	LDCircularPrimitive() = default;
	LDCircularPrimitive(
		PrimitiveModel::Type type,
		int segments,
		int divisions,
		const Matrix& transformationMatrix,
		const Vertex& position
	);

	LDObjectType type() const override;
	QString asText() const override;
	void getVertices(DocumentManager *context, QSet<Vertex>& verts) const override;
	bool isRasterizable() const override;
	void rasterize(
		DocumentManager* context,
		Winding parentWinding,
		Model& model,
		bool deep,
		bool render
	) override;
	QVector<LDPolygon> rasterizePolygons(DocumentManager* context, Winding parentWinding) override;
	QString objectListText() const override;
	int triangleCount(DocumentManager*) const override;
	QString iconName() const override;
	void serialize(class Serializer& serializer) override;

private:
	QString buildFilename() const;
	void buildPrimitiveBody(Model& model) const;
	QString stem() const;

	PrimitiveModel::Type m_type = PrimitiveModel::Circle;
	int m_segments = MediumResolution;
	int m_divisions = MediumResolution;
};
