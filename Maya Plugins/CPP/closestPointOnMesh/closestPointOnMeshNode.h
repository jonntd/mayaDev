#ifndef _closestPointOnMeshNode
#define _closestPointOnMeshNode

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
 
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Node implementation with standard viewport draw
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class closestPointOnMesh : public MPxLocatorNode
{
public:
						closestPointOnMesh();
	virtual				~closestPointOnMesh(); 

	virtual MStatus		compute( const MPlug& plug, MDataBlock& data );

	static  void*		creator();
	static  MStatus		initialize();

	virtual void            draw( M3dView & view, const MDagPath & path,
								  M3dView::DisplayStyle style,
								  M3dView::DisplayStatus status );
	
	void drawCube(MPoint center, float size, bool filled);
	void drawSquare(MPoint center, float size);
	void drawLine(MPoint start, MPoint end);

	virtual bool            isBounded() const;
	virtual MBoundingBox    boundingBox() const;
	virtual bool            isTransparent() const;

	// Attributes
	static MObject aInputCurve;
	static MObject aInputMesh;
	static MObject aPointSize;
	static MObject aCurvePoints;
	static MObject aMeshPoints;
	static MObject aMeshPointIndecies;
	static MObject aMeshClosestPoints;

public:

	static	MTypeId		id;
	static	MString		drawDbClassification;
	static	MString		drawRegistrantId;
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Viewport 2.0 override implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class closestPointOnMeshData : public MUserData
{
public:
	closestPointOnMeshData() : MUserData(false) {} // don't delete after draw
	virtual ~closestPointOnMeshData() {}

	MPointArray fCurvePoints;
	MPointArray fMeshPoints;
	MPointArray fMeshClosestPoints;
	MIntArray   fMeshIndecies;

	float       fPointSize;
};

class closestPointOnMeshDrawOverride : public MHWRender::MPxDrawOverride
{
public:
	static MHWRender::MPxDrawOverride* Creator(const MObject& obj)
	{
		return new closestPointOnMeshDrawOverride(obj);
	}

	virtual ~closestPointOnMeshDrawOverride();

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

	MPointArray getCurvePoints(const MDagPath& objPath) const;
	MPointArray getMeshPoints(const MDagPath& objPath) const;
	MPointArray getMeshClosestPoints(const MDagPath& objPath) const;
	MIntArray   getMeshPointIndecies(const MDagPath& objPath) const;
	float		getPointSize(const MDagPath& objPath) const;

private:
	closestPointOnMeshDrawOverride(const MObject& obj);
};

//
//class closestPointOnMeshCommand : public MPxCommand
//{
//public:
//	MStatus doIt(const MArgList& arg);
//	static void* creator();
//};

#endif
