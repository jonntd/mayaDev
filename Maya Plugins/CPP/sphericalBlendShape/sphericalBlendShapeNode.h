#ifndef _sphericalBlendShapeNode
#define _sphericalBlendShapeNode

#include <maya/MPxNode.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MTypeId.h> 
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MItGeometry.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MVector.h>

 
#define PI 3.14159265
#define TWOPI 6.28318531
#define HALFPI 1.57079633
#define EPSILON 0.0001
#define RADTODEG 57.29577951
#define DEGTORAD 0.01745329

enum Axis {
    AXIS_BOGUS = -1, // i picked BOGUS so that the constants are all the same length
    AXIS_POS_X = 0,
    AXIS_POS_Y,
    AXIS_POS_Z,
    AXIS_NEG_X,
    AXIS_NEG_Y,
    AXIS_NEG_Z
};


class sphericalBlendShape : public MPxDeformerNode
{
public:
						sphericalBlendShape();
	virtual				~sphericalBlendShape(); 

	virtual MStatus deform(MDataBlock& data, MItGeometry& itGeo, const MMatrix& mat, unsigned int geomIndex);

	static  void*		creator();
	static  MStatus		initialize();

public:
	
	static MObject aSpaceMatrix;
	static MObject aPoleAxis;
	static MObject aSeamAxis;
	static MObject aWarpMatrix;
	static MObject aMethod;

	static MTypeId id;

private:
	void lerp(MPoint& p1, MPoint& p2, double blend, MPoint& outPoint);
	void cartesianToSpherical(const MPoint& p, short poleAxis, short seamAxis, MPoint& warpPoint, MPoint& o);
	void sphericalToCartesian(const MPoint& p, short poleAxis, short seamAxis, MPoint& warpPoint, MPoint& o);

	MStatus checkPoleAndSeam(short poleAxis, short seamAxis);
	double getAxis(MVector& aPoint, short aAxis); 
	void setAxis(MVector& aPoint, short axis, double value); 
	double getAzimuth(MPoint point, short poleAxis, short seamAxis);
	short axisCross(short aAxisA, short aAxisB);
	bool isNearZero(double aValue);
};

#endif
