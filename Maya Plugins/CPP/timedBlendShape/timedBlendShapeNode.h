#ifndef _timedBlendShapeNode
#define _timedBlendShapeNode

#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MFnMesh.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MTypeId.h> 
#include <maya/MItGeometry.h>
#include <maya/MFloatArray.h>
#include <maya/MFnCompoundAttribute.h>
#include <math.h>
#include <algorithm>

 
class timedBlendShape : public MPxDeformerNode
{
public:
						timedBlendShape();
	virtual				~timedBlendShape(); 

	virtual MStatus     deform( MDataBlock& data,
                                MItGeometry& itGeo,
                                const MMatrix& localToWorldMatrix,
                                unsigned int geomIndex );
	

	static  void*		creator();
	static  MStatus		initialize();

	static MObject aBlendMesh;
	static MObject aBlendWeight;
	static MObject aTimingWeights;
	static MObject aTimingWeightsList;

public:
	static	MTypeId		id;

private:
	MStatus getWeights(MDataBlock& data, MObject& aWtList, MObject& aWts, MFloatArray& dArrWts, int nTotal, int listIdx);
	void    lerp(MPoint& p1, MPoint& p2, double blend, MPoint& outPoint);
};

#endif
