#ifndef _curveLocatorNode
#define _curveLocatorNode

#include <maya/MPxNode.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MTypeId.h> 
#include <maya/MPointArray.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnMesh.h>
#include <maya/MMeshIntersector.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MFnIntArrayData.h>

#include <string>
#include <sstream>

// Viewport 2.0 includes
#include <maya/MDrawRegistry.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MUserData.h>
#include <maya/MDrawContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPointArray.h>
 
class curveLocator : public MPxLocatorNode
{
public:
						curveLocator();
	virtual				~curveLocator(); 

	virtual void        postConstructor();

	virtual MStatus		compute( const MPlug& plug, MDataBlock& data );

	static  void*		creator();
	static  MStatus		initialize();

	virtual void        draw(M3dView & view, const MDagPath & path, 
							 M3dView::DisplayStyle style,
							 M3dView::DisplayStatus status);

	void drawCube(MPoint center, float size, bool filled);
	void drawSquare(MPoint center, float size);
	void drawLine(MPoint start, MPoint end);

	virtual bool            isBounded() const;
	virtual bool            isTransparent() const;
	virtual MBoundingBox    boundingBox() const; 

	static MObject aInputCurve;
	static MObject aInputMesh;
	static MObject aPointSize;
	static MObject aCurvePoints;
	static MObject aMeshPoints;
	static MObject aMeshPointIndecies;
	static MObject aMeshClosestPoints;

	static MObject aDrawCurvePoints;
	static MObject aDrawMeshPoints;
	static MObject aDrawLines;
	static MObject aDrawNumbers;

public:
	static	MTypeId	id;
	static	MString	drawDbClassification;
	static	MString	drawRegistrantId;
};


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Viewport 2.0 override implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class CurveLocatorData : public MUserData
{
public:
	CurveLocatorData() : MUserData(false) {} // don't delete after draw
	virtual ~CurveLocatorData() {}

	MPointArray fCurvePoints;
	MPointArray fMeshPoints;
	MIntArray   fIndecies;
};

class CurveLocatorDrawOverride : public MHWRender::MPxDrawOverride
{
public:
	static MHWRender::MPxDrawOverride* Creator(const MObject& obj)
	{
		return new CurveLocatorDrawOverride(obj);
	}

	virtual ~CurveLocatorDrawOverride();

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

private:
	CurveLocatorDrawOverride(const MObject& obj);

	MPointArray getCurvePoints(const MDagPath& objPath) const;
	MPointArray getMeshPoints(const MDagPath& objPath) const;
	MIntArray getMeshPointIndecies(const MDagPath& objPath) const;
};

#endif
