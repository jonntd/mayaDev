#include <string.h>
#include <maya/MIOStream.h>
#include <cmath>
#include <float.h>
#include <stdio.h>
#include <sstream>

#include <maya/MPxNode.h> 
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MTypeId.h> 
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MMatrix.h>
#include <maya/MVector.h>
#include <maya/MDoubleArray.h>
#include <maya/MGlobal.h>

#define SYMMETRY_PLANE_XY   0
#define SYMMETRY_PLANE_YZ   1
#define SYMMETRY_PLANE_XZ   2

#define POSITION_MIRROR_X 0
#define POSITION_MIRROR_Y 1
#define POSITION_MIRROR_Z 2
#define POSITION_AND_AXES 3


#define LOG_TO_CONSOLE(debugText, object)                   \
{                                                           \
    std::stringstream stream;                               \
    stream << object;                                       \
    MGlobal::displayInfo(MString(debugText) + " : " + MString(stream.str().c_str()));    \
}

enum FlipAxis {
    FLIP_AXIS_X = 0,
    FLIP_AXIS_Y = 1,
    FLIP_AXIS_Z = 2
};

class mirrorMatrix : public MPxNode
{
public:
                        mirrorMatrix();
	virtual             ~mirrorMatrix();

	static  void*		creator();
	static  MStatus		initialize();


    virtual MStatus     compute(const MPlug&, MDataBlock&);

public:
    static MObject aInputMatrices;
    static MObject aPivotMatrix;
    static MObject aSymmetryPlane;
    static MObject aFlippingMode;
    static MObject aOutputMatrices;
    
	static MTypeId id;

private:
    MStatus flipMirrorOrientationXform(MMatrix& inputMatrix, 
                                       MMatrix& outputXform,
                                       unsigned int flipAxis,
                                       unsigned int freeAxis);

    MStatus flipMatrix(MMatrix& matrix, unsigned int symmetryPlane, unsigned int freeAxis);
};