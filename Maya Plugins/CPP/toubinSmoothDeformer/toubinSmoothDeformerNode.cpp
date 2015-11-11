#include "toubinSmoothDeformerNode.h"

#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <maya/MGlobal.h>

MTypeId toubinSmoothDeformer::id(0x00001);
MObject toubinSmoothDeformer::aIterations;
MObject toubinSmoothDeformer::aSmoothBoundaries;
MObject toubinSmoothDeformer::aLambda;
MObject toubinSmoothDeformer::aMu;
MObject toubinSmoothDeformer::aUpdateUV;

toubinSmoothDeformer::toubinSmoothDeformer() {}
toubinSmoothDeformer::~toubinSmoothDeformer() {}

MStatus toubinSmoothDeformer::deform( MDataBlock& data, MItGeometry& itGeo, 
        const MMatrix& localToWorldMatrix, unsigned int geomIndex )
{
    MStatus status;
	MPoint point;

	// get the input geom
	MArrayDataHandle hInput = data.outputArrayValue(input, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = hInput.jumpToElement(geomIndex);
	MDataHandle hInputElement = hInput.outputValue(&status);
	MObject oInputGeom = hInputElement.child(inputGeom).asMesh();
	MFnMesh fnInputMesh(oInputGeom);

	// get the output
	MArrayDataHandle hOutput = data.outputArrayValue(outputGeom, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = hOutput.jumpToElement(geomIndex);
	MDataHandle hOutputElement = hOutput.outputValue(&status);
	MObject oOutputGeom = hOutputElement.child(outputGeom).asMesh();
	MFnMesh fnOutputMesh(oOutputGeom);

	int numIterations     = data.inputValue(aIterations).asInt();
	bool smoothBoundaries = data.inputValue(aSmoothBoundaries).asBool();
	double lambda		  = data.inputValue(aLambda).asFloat();
	double mu			  = data.inputValue(aMu).asFloat();
	bool updateUVs        = data.inputValue(aUpdateUV).asBool();

	float env = data.inputValue(envelope).asFloat();
	
	MItMeshVertex vertexIter(oInputGeom, &status);

	for (int iteration=0; iteration<numIterations; iteration++) {
		laplaceSmoothingUmbrellaOperator(fnInputMesh, itGeo, vertexIter, data, geomIndex, lambda, false, env);
		if (smoothBoundaries) {
			laplaceSmoothingUmbrellaOperator(fnInputMesh, itGeo, vertexIter, data, geomIndex, lambda, true, env);
		}
		laplaceSmoothingUmbrellaOperator(fnInputMesh, itGeo, vertexIter, data, geomIndex, mu, false, env);
		if (smoothBoundaries) {
			laplaceSmoothingUmbrellaOperator(fnInputMesh, itGeo, vertexIter, data, geomIndex, mu, true, env);
		}
	}
	
	if (updateUVs) {
		MPointOnMesh closestPoint;
		float2 uvPoint;

		MItMeshVertex oVertexIter(oOutputGeom, &status);
		int prevIndex = 0;

		// Get UVSets for this mesh
		MStringArray  UVSets;
		status = fnInputMesh.getUVSetNames(UVSets);

		// Get all UVs for the first UV set.
		MFloatArray  uArray, vArray;
		fnInputMesh.getUVs(uArray, vArray, &UVSets[0]);

		itGeo.reset();
		for ( ; !itGeo.isDone(); itGeo.next() ) {
			point = itGeo.position();
			status = fnInputMesh.getUVAtPoint(point, uvPoint, MSpace::kObject);

			uArray[itGeo.index()] = uvPoint[0];
			vArray[itGeo.index()] = uvPoint[1];
		}

		fnOutputMesh.setUVs(uArray, vArray, &UVSets[0]);
		fnOutputMesh.updateSurface();
	}

	

	return MS::kSuccess;
}

void toubinSmoothDeformer::laplaceSmoothingUmbrellaOperator(MFnMesh& fnMesh, MItGeometry& itGeo, MItMeshVertex& vertexIter, MDataBlock& data, unsigned int geomIndex, double lambda, bool boundaries, float envelope) {
	MStatus status;
	MPoint point, destPoint;
	float w;

	MIntArray vertNeighbors;
	int prevIndex = 0;

	MPointArray meshVerts;
	fnMesh.getPoints(meshVerts, MSpace::kObject);

	MPoint centerOfGravity;
	MPointArray listOfNewPositions;
	listOfNewPositions.clear();
	listOfNewPositions.setLength(meshVerts.length());


	for ( ; !itGeo.isDone(); itGeo.next() )
	{
		unsigned int valence=0;
		centerOfGravity.x = 0.0;
		centerOfGravity.y = 0.0;
		centerOfGravity.z = 0.0;

		// get the neighbor vertices
		vertexIter.setIndex(itGeo.index(), prevIndex);
		vertexIter.getConnectedVertices(vertNeighbors);
		for (unsigned int c=0; c<vertNeighbors.length(); c++) {
			centerOfGravity += meshVerts[vertNeighbors[c]];
			++valence;
		}
		listOfNewPositions[itGeo.index()] = centerOfGravity/valence;
	}

	itGeo.reset();

	for ( ; !itGeo.isDone(); itGeo.next() )
	{
		point = itGeo.position();
		w = weightValue(data, geomIndex, itGeo.index());
		vertexIter.setIndex(itGeo.index(), prevIndex);

		if (!boundaries && !vertexIter.onBoundary(&status)) {
			destPoint = (listOfNewPositions[itGeo.index()] - point) * lambda + point;
			lerp(point, destPoint, w*envelope, destPoint);
			itGeo.setPosition(destPoint);
		}
		if (boundaries && vertexIter.onBoundary(&status)) {
			destPoint = (listOfNewPositions[itGeo.index()] - point) * lambda + point;
			lerp(point, destPoint, w*envelope, destPoint);
			itGeo.setPosition(destPoint);
		}
	}
}

// linear interpolation
void toubinSmoothDeformer::lerp(MPoint& p1, MPoint& p2, double blend, MPoint& outPoint) {
	outPoint.x = p1.x + (p2.x - p1.x)*blend;
	outPoint.y = p1.y + (p2.y - p1.y)*blend;
	outPoint.z = p1.z + (p2.z - p1.z)*blend;
}

void* toubinSmoothDeformer::creator()
{
	return new toubinSmoothDeformer();
}

MStatus toubinSmoothDeformer::initialize()
{
	MStatus stat;
	MFnNumericAttribute nAttr;

	aIterations = nAttr.create("iterations", "iterations", MFnNumericData::kInt, 0);

	aSmoothBoundaries = nAttr.create("smoothBoundaries", "smoothBoundaries", MFnNumericData::kBoolean, true);

	aLambda = nAttr.create("lambda", "lambda", MFnNumericData::kFloat, 0.330);
	aMu     = nAttr.create("mu", "mu", MFnNumericData::kFloat, -0.331);

	aUpdateUV = nAttr.create("updateUVs", "updateUVs", MFnNumericData::kBoolean, false);

	addAttribute(aIterations);
	addAttribute(aSmoothBoundaries);
	addAttribute(aLambda);
	addAttribute(aMu);
	addAttribute(aUpdateUV);

	attributeAffects(aIterations, outputGeom);
	attributeAffects(aSmoothBoundaries, outputGeom);
	attributeAffects(aLambda, outputGeom);
	attributeAffects(aMu, outputGeom);
	attributeAffects(aUpdateUV, outputGeom);


	return MS::kSuccess;
}

