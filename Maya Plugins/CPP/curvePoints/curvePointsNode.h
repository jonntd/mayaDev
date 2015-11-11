#ifndef _curvePointsNode
#define _curvePointsNode


#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MTypeId.h> 
#include <maya/MPxLocatorNode.h>
#include <maya/MPointArray.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/M3dView.h>
 
class curvePoints : public MPxNode
{
public:
						curvePoints();
	virtual				~curvePoints(); 

	virtual MStatus		compute( const MPlug& plug, MDataBlock& data );

	static  void*		creator();
	static  MStatus		initialize();

	virtual void        draw(M3dView & view, const MDagPath & path, 
							 M3dView::DisplayStyle style,
							 M3dView::DisplayStatus status);

	void drawCube(MPoint center, float size, bool filled);

	virtual bool            isBounded() const;
	virtual bool            isTransparent() const;
	virtual MBoundingBox    boundingBox() const; 

	static MObject aInputCurve;
	static MObject aCurvePoints;

	static MObject aFillColor; 
	static MObject aOutlineColor;
	static MObject aTransparency; 

public:
	static	MTypeId		id;
};

#endif
