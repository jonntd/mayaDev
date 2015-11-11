#include "sphericalBlendShapeNode.h"

#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <maya/MGlobal.h>

MTypeId sphericalBlendShape::id(0x00008);

MObject sphericalBlendShape::aSpaceMatrix;
MObject sphericalBlendShape::aPoleAxis;
MObject sphericalBlendShape::aSeamAxis;
MObject sphericalBlendShape::aWarpMatrix;
MObject sphericalBlendShape::aMethod;

sphericalBlendShape::sphericalBlendShape() {}
sphericalBlendShape::~sphericalBlendShape() {}

MStatus sphericalBlendShape::deform(MDataBlock& data, MItGeometry& itGeo, const MMatrix& mat, unsigned int geomIndex) 
{
	MStatus status = MS::kSuccess;

	float env = data.inputValue(envelope).asFloat();
	if (env <= 0.0f) {
		return MS::kSuccess;
	}

	MMatrix spaceMatrix = data.inputValue(aSpaceMatrix).asMatrix();
	short poleAxis		= data.inputValue(aPoleAxis).asShort();
	short seamAxis		= data.inputValue(aSeamAxis).asShort();
	short method		= data.inputValue(aMethod).asShort();
	MMatrix warpMatrix	= data.inputValue(aWarpMatrix).asMatrix();

	MTransformationMatrix warpTransMatrix(warpMatrix);
	MPoint warpPoint = warpTransMatrix.getTranslation(MSpace::kWorld);
	
	status = checkPoleAndSeam(poleAxis, seamAxis);
	CHECK_MSTATUS_AND_RETURN_IT(status);


	MMatrix invGeoMatrix   = mat.inverse();
	MMatrix invSpaceMatrix = spaceMatrix.inverse();

	MPointArray defPoints;
	MPoint* defPoint;
	MPoint inPoint, returnPoint;

	itGeo.allPositions(defPoints);
	unsigned int count = defPoints.length();

	unsigned int i;
	switch(method) {
		// XYZ to Spherical
	case 0:
		for (i=0; i<count; i++) {
			defPoint = &defPoints[i];
			inPoint = *defPoint;

			// bring the point into world space
			inPoint *= invGeoMatrix;
			// bring into local space of the input matrix
			inPoint *= invSpaceMatrix;

			cartesianToSpherical(inPoint, poleAxis, seamAxis, warpPoint, returnPoint);

			// bring the point back into world space
			returnPoint *= spaceMatrix;
			// bring the point back into local space
			returnPoint *= mat;
				
			lerp(*defPoint, returnPoint, env, *defPoint);
		}
		break;

	case 1:
		for (i=0; i<count; i++) {
			defPoint = &defPoints[i];
			inPoint = *defPoint;

			// bring the point into world space
			inPoint *= invGeoMatrix;
			// bring into local space of the input matrix
			inPoint *= invSpaceMatrix;

			sphericalToCartesian(inPoint, poleAxis, seamAxis, warpPoint, returnPoint);

			// bring the point back into world space
			returnPoint *= spaceMatrix;
			// bring the point back into local space
			returnPoint *= mat;
				
			lerp(*defPoint, returnPoint, env, *defPoint);
		}
		break;
	}

	itGeo.setAllPositions(defPoints);

	return MS::kSuccess;
}

// linear interpolation
void sphericalBlendShape::lerp(MPoint& p1, MPoint& p2, double blend, MPoint& outPoint) {
	outPoint.x = p1.x + (p2.x - p1.x)*blend;
	outPoint.y = p1.y + (p2.y - p1.y)*blend;
	outPoint.z = p1.z + (p2.z - p1.z)*blend;
}

// Convert from cartesian to spherical coordinates
void sphericalBlendShape::cartesianToSpherical(const MPoint& point, short poleAxis, short seamAxis, MPoint& warpPoint, MPoint& outPoint) {
	double radius, zenith, azimuth;

	radius = MVector(point).length();
	
	azimuth = getAzimuth(point, poleAxis, seamAxis);

	zenith = 0.0;
	if (!isNearZero(radius)) {
		zenith = acos(getAxis(MVector(point), poleAxis) / radius);
	}

	outPoint.x = radius;
	outPoint.y = zenith;
	outPoint.z = azimuth;
}

// Convert from spherical to cartesian coordinates
void sphericalBlendShape::sphericalToCartesian(const MPoint& aPoint, short aPoleAxis, short aSeamAxis, MPoint& warpPoint, MPoint& outPoint) {
	const double radius  = aPoint[0];
    const double zenith  = aPoint[1];
    const double azimuth = aPoint[2];

	MVector result;

	setAxis(result, aSeamAxis, radius * sin(zenith) * cos(azimuth));
    setAxis(result, axisCross(aPoleAxis, aSeamAxis), radius * sin(zenith) * sin(azimuth));
    setAxis(result, aPoleAxis, radius * cos(zenith));

	outPoint.x = result[0];
	outPoint.y = result[1];
	outPoint.z = result[2];
}

double sphericalBlendShape::getAzimuth(MPoint point, short aPoleAxis, short aSeamAxis)
{
	double azimuth = atan2(getAxis(MVector(point), axisCross(aPoleAxis, aSeamAxis)), getAxis(MVector(point), aSeamAxis));

	// atan2 returns the range [-pi, pi].  remap to [0, 2pi).
    if (azimuth < 0 && isNearZero(azimuth)) {
        azimuth += 2 * M_PI;
    }

	return azimuth;
}

bool sphericalBlendShape::isNearZero(double aValue)
{
	const double epsilon = 1e-12;

	return std::abs(aValue) <= epsilon;
}

const Axis _axisCrossLUT[6][6] = {
               /* +X */    /* +Y */    /* +Z */    /* -X */    /* -Y */    /* -Z */
    /* +X */ {AXIS_BOGUS, AXIS_POS_Z, AXIS_NEG_Y, AXIS_BOGUS, AXIS_NEG_Z, AXIS_POS_Y},
    /* +Y */ {AXIS_NEG_Z, AXIS_BOGUS, AXIS_POS_X, AXIS_POS_Z, AXIS_BOGUS, AXIS_NEG_X},
    /* +Z */ {AXIS_POS_Y, AXIS_NEG_X, AXIS_BOGUS, AXIS_NEG_Y, AXIS_POS_X, AXIS_BOGUS},
    /* -X */ {AXIS_BOGUS, AXIS_NEG_Z, AXIS_POS_Y, AXIS_BOGUS, AXIS_POS_Z, AXIS_NEG_Y},
    /* -Y */ {AXIS_POS_Z, AXIS_BOGUS, AXIS_NEG_X, AXIS_NEG_Z, AXIS_BOGUS, AXIS_POS_X},
    /* -Z */ {AXIS_NEG_Y, AXIS_POS_X, AXIS_BOGUS, AXIS_POS_Y, AXIS_NEG_X, AXIS_BOGUS}
};

short sphericalBlendShape::axisCross(short aAxisA, short aAxisB) {
    return _axisCrossLUT[aAxisA][aAxisB];
}

MStatus sphericalBlendShape::checkPoleAndSeam(short poleAxis, short seamAxis) 
{
	if (poleAxis % 3 == seamAxis % 3) {
		return MS::kFailure;
	}

	return MS::kSuccess;
}

double sphericalBlendShape::getAxis(MVector& aPoint, short aAxis)
{
	double result = aPoint[aAxis % 3];
	if (aAxis > AXIS_POS_Z) {
        result *= -1;
    }

    return result;
}
void sphericalBlendShape::setAxis(MVector& aPoint, short axis, double value)
{
	if (axis > AXIS_POS_Z) {
        aPoint[axis % 3] = -value;
    } else {
        aPoint[axis % 3] = value;
    }
}

void* sphericalBlendShape::creator()
{
	return new sphericalBlendShape();
}

MStatus sphericalBlendShape::initialize()
{
	MStatus status;

	MFnMatrixAttribute mAttr;
	MFnEnumAttribute   eAttr;

	aSpaceMatrix = mAttr.create("spaceMatrix", "spaceMatrix", MFnMatrixAttribute::kDouble, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	aPoleAxis = eAttr.create("poleAxis", "poleAxis", 1, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	eAttr.addField("+X", 0);
	eAttr.addField("+Y", 1);
	eAttr.addField("+Z", 2);
	eAttr.addField("-X", 3);
	eAttr.addField("-Y", 4);
	eAttr.addField("-Z", 5);
	eAttr.setDefault(1);
	eAttr.setKeyable(true);
	eAttr.setStorable(true);
	eAttr.setWritable(true);

	aSeamAxis = eAttr.create("seamAxis", "seamAxis", 0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	eAttr.addField("+X", 0);
	eAttr.addField("+Y", 1);
	eAttr.addField("+Z", 2);
	eAttr.addField("-X", 3);
	eAttr.addField("-Y", 4);
	eAttr.addField("-Z", 5);
	eAttr.setDefault(0);
	eAttr.setKeyable(true);
	eAttr.setStorable(true);
	eAttr.setWritable(true);

	aWarpMatrix = mAttr.create("warpMatrix", "warpMatrix", MFnMatrixAttribute::kDouble, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	aMethod = eAttr.create("conversionMethod", "conversionMethod", 0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	eAttr.addField("xyzToSpherical", 0);
	eAttr.addField("sphericalToXyz", 1);
	eAttr.setDefault(0);
	eAttr.setKeyable(true);
	eAttr.setStorable(true);
	eAttr.setWritable(true);

	addAttribute(aSpaceMatrix);
	addAttribute(aPoleAxis);
	addAttribute(aSeamAxis);
	addAttribute(aWarpMatrix);
	addAttribute(aMethod);

	attributeAffects(aSpaceMatrix, outputGeom);
	attributeAffects(aPoleAxis, outputGeom);
	attributeAffects(aSeamAxis, outputGeom);
	attributeAffects(aWarpMatrix, outputGeom);
	attributeAffects(aMethod, outputGeom);

	return MS::kSuccess;
}

