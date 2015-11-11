#ifndef _sphericalBlendShapeVisualizerNode
#define _sphericalBlendShapeVisualizerNode

#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MTypeId.h> 
#include <maya/MPxLocatorNode.h>

#include <stdio.h>
#include <maya/MString.h>
#include <maya/MArgList.h>
#include <maya/MPxCommand.h>
#include <maya/MIOStream.h>

// Viewport 2.0 includes
#include <maya/MDrawRegistry.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MUserData.h>
#include <maya/MDrawContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPointArray.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnMesh.h>
#include <maya/MMeshIntersector.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MSelectionList.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MDrawData.h>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Node implementation with standard viewport draw
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class sphericalBlendShapeVisualizer : public MPxLocatorNode
{
public:
							sphericalBlendShapeVisualizer();
	virtual					~sphericalBlendShapeVisualizer(); 

	static  void*			creator();
	static  MStatus			initialize();

	virtual void            draw( M3dView & view, const MDagPath & path,
								  M3dView::DisplayStyle style,
								  M3dView::DisplayStatus status );

	virtual bool            isBounded() const;
	virtual MBoundingBox    boundingBox() const;
	virtual bool            isTransparent() const;

public:

	static MTypeId id;

	static MObject aSpaceMatrix;
	static MObject aPoleAxis;
	static MObject aSeamAxis;

	static MString drawDbClassification;
	static MString drawRegistrantId;
};

enum CurAxis {
    CUR_AXIS_BOGUS = -1, // i picked BOGUS so that the constants are all the same length
    CUR_AXIS_POS_X = 0,
    CUR_AXIS_POS_Y,
    CUR_AXIS_POS_Z,
    CUR_AXIS_NEG_X,
    CUR_AXIS_NEG_Y,
    CUR_AXIS_NEG_Z
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Viewport 2.0 override implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class sphericalBlendShapeVisualizerData : public MUserData
{
public:
	sphericalBlendShapeVisualizerData() : MUserData(false) {} // don't delete after draw
	virtual ~sphericalBlendShapeVisualizerData() {}

	MMatrix fSpaceMatrix;
	short   fPoleAxis;
	short   fSeamAxis;
};

class sphericalBlendShapeVisualizerDrawOverride : public MHWRender::MPxDrawOverride
{
public:
	static MHWRender::MPxDrawOverride* Creator(const MObject& obj)
	{
		return new sphericalBlendShapeVisualizerDrawOverride(obj);
	}

	virtual ~sphericalBlendShapeVisualizerDrawOverride();

	virtual MHWRender::DrawAPI supportedDrawAPIs() const;

	virtual bool isBounded(
		const MDagPath& objPath,
		const MDagPath& cameraPath) const;

	virtual MBoundingBox boundingBox(
		const MDagPath& objPath,
		const MDagPath& cameraPath) const;

	virtual MUserData* prepareForDraw(
		const MDagPath& objPath,
		const MDagPath& cameraPath,
		const MHWRender::MFrameContext& frameContext,
		MUserData* oldData);

	virtual bool hasUIDrawables() const { return true; }

	virtual void addUIDrawables(
		const MDagPath& objPath,
		MHWRender::MUIDrawManager& drawManager,
		const MHWRender::MFrameContext& frameContext,
		const MUserData* data);

	static void draw(const MHWRender::MDrawContext& context, const MUserData* data) {};

	MMatrix getSpaceMatrix(const MDagPath& objPath) const;
	short   getPoleAxis(const MDagPath& objPath) const;
	short   getSeamAxis(const MDagPath& objPath) const;

	void sphericalToCartesian(const MPoint& aPoint, short aPoleAxis, short aSeamAxis,  MPoint& aOutPoint);
	void setAxis(MVector& aPoint, short axis, double value); 
	short axisCross(short aAxisA, short aAxisB);

private:
	sphericalBlendShapeVisualizerDrawOverride(const MObject& obj);

	MPointArray bboxPoints;
};

#endif