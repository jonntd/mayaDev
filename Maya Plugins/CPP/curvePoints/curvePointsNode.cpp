#include "curvePointsNode.h"

#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <maya/MGlobal.h>

MTypeId curvePoints::id( 0x08102C );
MObject curvePoints::aInputCurve;
MObject curvePoints::aCurvePoints;
MObject curvePoints::aFillColor; 
MObject curvePoints::aOutlineColor;
MObject curvePoints::aTransparency; 

curvePoints::curvePoints() {}
curvePoints::~curvePoints() {}

MStatus curvePoints::compute( const MPlug& plug, MDataBlock& data )

{
	MStatus status;

	/*MDataHandle hInputCurve = data.inputValue(aInputCurve, &status);
	if (!status) {
		status.perror("Error Getting Curve Data!");
		return MS::kFailure;
	}

	MObject curve = hInputCurve.asNurbsCurveTransformed();
	MFnNurbsCurve curveFn(curve, &status);
	if( !status ) {
		status.perror("ERROR creating curve function set");
		return MS::kFailure;
	}*/

	//MArrayDataHandle hCurvePoints = data.outputValue(aCurvePoints, &status);
	//MArrayDataBuilder arrayBuilder = hCurvePoints.builder(&status);

	//int numCVs = curveFn.numCVs();

	//MPointArray outPositions;
	//outPositions.setLength(numCVs);

	//MPoint curvePoint;
	//for (int c=0; c<numCVs; c++) {
	//	MDataHandle arrayHandle = arrayBuilder.addElement(c, &status);
	//	status = curveFn.getCV(c, curvePoint);
	//	arrayHandle.set(MVector(curvePoint));
	//}
	//
	//hCurvePoints.set(arrayBuilder);
	return MS::kSuccess;
}

void curvePoints::draw(M3dView& view,
					   const MDagPath& DGpath,
					   M3dView::DisplayStyle style,
					   M3dView::DisplayStatus status)
{
	//MObject thisNode = thisMObject();

	////MPlug pointsPlug = MPlug(thisNode, aCurvePoints);

	//MPlug tPlug = MPlug(thisNode, aTransparency); 
	//MPlug fillColorPlug = MPlug(thisNode, aFillColor); 
	//MPlug outlineColorPlug = MPlug(thisNode, aOutlineColor);

	//float fr, fg, fb, fa; 
	//float or, og, ob;
	//MObject fillColor;
	//MObject outlineColor; 

	//fillColorPlug.getValue(fillColor); 
	//MFnNumericData fillColordata(fillColor); 
	//fillColordata.getData(fr, fg, fb); 
	//tPlug.getValue(fa); 

	//outlineColorPlug.getValue(outlineColor); 
	//MFnNumericData outlineColordata(outlineColor); 
	//outlineColordata.getData(or, og, ob); 

	//view.beginGL(); 

	//MColor solidColor, wireColor;
	//if ( status == M3dView::kActive )
 //   {
 //       solidColor = MColor( 1.0f, 1.0f, 1.0f, fa );
 //       wireColor = MColor( 1.0f, 0.86f, .141f, 1.0f );
 //   }
 //   else if ( status == M3dView::kLead )
 //   {
 //       solidColor = MColor(fr, fg, fb, fa);
 //       wireColor = MColor( 1.0f, 0.86f, .141f, 1.0f );
 //   }
 //   else
 //   {
 //       solidColor = MColor(fr, fg, fb, fa);
 //       wireColor = MColor(or, og, ob, 1.0);
 //   }

	//MPoint center(0.0f, 0.0f, 0.0f, 1.0f);

	//float size = 1.0f;

	//glColor4f( solidColor.r, solidColor.g, solidColor.b, solidColor.a );
	//drawCube(center, size, true);

	//glColor4f( wireColor.r, wireColor.g, wireColor.b, wireColor.a );
	//drawCube(center, size, false);

	//view.endGL();
}

void curvePoints::drawCube(MPoint center, float size, bool filled)
{
	int renderState = filled ? GL_POLYGON : GL_LINE_LOOP;

	float ver[8][3] = {
		{-size/2.0f, -size/2.0f, size/2.0f},
		{-size/2.0f, size/2.0f,  size/2.0f},
		{size/2.0f,  size/2.0f,  size/2.0f},
		{size/2.0f,  -size/2.0f, size/2.0f},
		{-size/2.0f, -size/2.0f, -size/2.0f},
		{-size/2.0f, size/2.0f,  -size/2.0f},
		{size/2.0f,  size/2.0f,  -size/2.0f},
		{size/2.0f,  -size/2.0f, -size/2.0f},
	};

	// transform the ver array by the center
	for (int i=0; i<8; i++) {
		ver[i][0] += float(center.x);
		ver[i][1] += float(center.y);
		ver[i][2] += float(center.z);
	}

	int quads[6][4] = {
		{0,3,2,1},
		{2,3,7,6},
		{0,4,7,3},
		{1,2,6,5},
		{4,5,6,7},
		{0,1,5,4},
	};


	for (int i=0; i<6; i++) {
		glBegin(renderState);

		// glLineWidth(lineWidth);

		glVertex3fv(ver[quads[i][0]]);
		glVertex3fv(ver[quads[i][1]]);
		glVertex3fv(ver[quads[i][2]]);
		glVertex3fv(ver[quads[i][3]]);

		glEnd();
	}
}

bool curvePoints::isBounded() const
{ 
	return true;
}


bool curvePoints::isTransparent() const
{ 
	return true;
}


MBoundingBox curvePoints::boundingBox() const
{
	MBoundingBox bbox;

	return bbox;
}


void* curvePoints::creator()
{
	return new curvePoints();
}

MStatus curvePoints::initialize()
{
	MStatus status;
	MFnTypedAttribute tAttr;
	MFnNumericAttribute nAttr;

	/*aInputCurve = tAttr.create("inputCurve", "inputCurve", MFnData::kNurbsCurve, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	aCurvePoints = tAttr.create("curvePoints", "curvePoints", MFnData::kPointArray, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	tAttr.setStorable(false);
	tAttr.setWritable(false);
	tAttr.setArray(true);
	tAttr.setHidden(true);

	aFillColor = nAttr.createColor("fillColor", "fillColor");
	nAttr.setDefault(0.1, 0.1, 0.8);
	nAttr.setKeyable(true);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setStorable(true);

	aOutlineColor = nAttr.createColor("outlineColor", "outlineColor");
	nAttr.setDefault(0.0, 0.0, 0.0);
	nAttr.setKeyable(true);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setStorable(true);

	aTransparency = nAttr.create("transparency", "transparency", MFnNumericData::kFloat);
	nAttr.setDefault(0.15);
	nAttr.setKeyable(true);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setStorable(true);

	addAttribute(aInputCurve);
	addAttribute(aCurvePoints);
	addAttribute(aFillColor);
	addAttribute(aOutlineColor);
	addAttribute(aTransparency);
*/
	return MS::kSuccess;
}

