#ifndef _toubinSmoothDeformerNode
#define _toubinSmoothDeformerNode

#include <maya/MPxNode.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MItGeometry.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshVertex.h>
#include <maya/MMeshIntersector.h>
#include <maya/MIntArray.h>
#include <maya/MTypeId.h> 
#include <vector>

 
class toubinSmoothDeformer : public MPxDeformerNode
{
public:
						toubinSmoothDeformer();
	virtual				~toubinSmoothDeformer(); 

	virtual MStatus     deform( MDataBlock& data,
                                MItGeometry& itGeo,
                                const MMatrix& localToWorldMatrix,
                                unsigned int geomIndex );

	static  void*		creator();
	static  MStatus		initialize();

	static MObject aIterations;
	static MObject aSmoothBoundaries;
	static MObject aLambda;
	static MObject aMu;
	static MObject aUpdateUV;

public:
	static	MTypeId		id;

private:
	void    lerp(MPoint& p1, MPoint& p2, double blend, MPoint& outPoint);

	void laplaceSmoothingUmbrellaOperator(MFnMesh& mesh, MItGeometry& itGeo, MItMeshVertex& vertexIter, MDataBlock& data, unsigned int geomIndex, double lambda, bool boundaries, float envelope);
};

#endif
