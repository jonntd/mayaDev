#include "curveLocatorNode.h"

#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <maya/MGlobal.h>

MTypeId curveLocator::id( 0x08102C );

MObject curveLocator::aInputCurve;
MObject curveLocator::aInputMesh;
MObject curveLocator::aPointSize;
MObject curveLocator::aCurvePoints;
MObject curveLocator::aMeshPoints;
MObject curveLocator::aMeshClosestPoints;
MObject curveLocator::aMeshPointIndecies;
MObject curveLocator::aDrawCurvePoints;
MObject curveLocator::aDrawMeshPoints;
MObject curveLocator::aDrawLines;
MObject curveLocator::aDrawNumbers;

MString	curveLocator::drawDbClassification("drawdb/geometry/footPrint");
MString	curveLocator::drawRegistrantId("CurveLocatorPlugin");

curveLocator::curveLocator() {}
curveLocator::~curveLocator() {}

MStatus curveLocator::compute( const MPlug& plug, MDataBlock& data )
{
	MStatus status;

	MDataHandle hInputCurve = data.inputValue(aInputCurve, &status);
	if (!status) {
		status.perror("Error getting handle to input curve");
		return MS::kFailure;
	}

	MObject curve = hInputCurve.asNurbsCurveTransformed();
	MFnNurbsCurve curveFn(curve, &status);
	if (!status) {
		status.perror("Error creating curve function set");
		return MS::kFailure;
	}

	int numCVs = curveFn.numCVs();

	MDataHandle hInputMesh = data.inputValue(aInputMesh, &status);
	if (!status) {
		status.perror("Error getting handle to input mesh");
		return MS::kFailure;
	}

	MObject mesh = hInputMesh.asMeshTransformed();
	MFnMesh meshFn(mesh, &status);
	if (!status) {
		status.perror("Error creating mesh function set");
		return MS::kFailure;
	}

	// create fast intersector structure
	MMeshIntersector intersector;
	intersector.create(mesh);

	MDataHandle hCurvePoints = data.outputValue ( aCurvePoints, &status );
	MObject curvePoints = hCurvePoints.data();
	MFnPointArrayData cvData(curvePoints, &status);
	MPointArray outPoints;

	MDataHandle hMeshPoints = data.outputValue(aMeshPoints, &status);
	MObject meshPoints = hMeshPoints.data();
	MFnPointArrayData meshPointData(meshPoints, &status);
	MPointArray outMeshPoints;

	MDataHandle hMeshClosestPoints = data.outputValue(aMeshClosestPoints, &status);
	MObject meshClosestPoints = hMeshClosestPoints.data();
	MFnPointArrayData meshClosestPointData(meshClosestPoints, &status);
	MPointArray outMeshClosestPoints;

	MDataHandle hMeshPointIndecies = data.outputValue(aMeshPointIndecies, &status);
	MObject meshPointIndecies = hMeshPointIndecies.data();
	MFnIntArrayData indeciesData(meshPointIndecies);
	MIntArray outMeshPointIndecies;

	MPoint curvePoint;
	MPointOnMesh meshPoint;
	MPoint closestPoint;
	bool failed = false;
	MItMeshPolygon polyIter(mesh);
	int dummyIndex;
	MPointArray facePoints;
	MVector directionVector;
	MIntArray vertIds;

	for (int i=0; i<numCVs; i++) {
		status = curveFn.getCV(i, curvePoint);
		outPoints.append(curvePoint);
		double length = 999999999999999.0;
		MPoint closestFacePoint(0.0, 0.0, 0.0, 1.0);
		int closestVertId = -1;

		MStatus localStatus = intersector.getClosestPoint(curvePoint, meshPoint);
		if(localStatus != MStatus::kSuccess) {
			// NOTE - we cannot break out of an OpenMP region, so set
			// bad status and skip remaining iterations
			failed = true;
			continue;
		}

		outMeshClosestPoints.append(meshPoint.getPoint());

		int faceIndex = meshPoint.faceIndex();
		polyIter.setIndex(faceIndex, dummyIndex);
		polyIter.getPoints(facePoints, MSpace::kWorld, &status);

		meshFn.getPolygonVertices(faceIndex, vertIds);

		for (size_t p=0; p<facePoints.length(); p++) {
			int curIndex = int(p);
			directionVector = MVector(curvePoint) - MVector(facePoints[curIndex]);
			double curLength = abs(directionVector.length());
			if (curLength < length) {
				length = curLength;
				closestFacePoint = facePoints[curIndex];
				closestVertId = vertIds[curIndex];
			}
		}

		outMeshPoints.append(closestFacePoint);
		outMeshPointIndecies.append(closestVertId);
	}

	status = cvData.set(outPoints);
	hCurvePoints.set(curvePoints);

	status = meshPointData.set(outMeshPoints);
	hMeshPoints.set(meshPoints);

	status = meshClosestPointData.set(outMeshClosestPoints);
	hMeshClosestPoints.set(meshClosestPoints);

	status = indeciesData.set(outMeshPointIndecies);
	hMeshPointIndecies.set(meshPointIndecies);

	status = data.setClean(plug);

	return MS::kSuccess;
}

MStatus curveLocator::initialize()
{
	MStatus status;

	MFnNumericAttribute nAttr;
	MFnTypedAttribute   tAttr;

	aInputCurve = tAttr.create("inputCurve", "inputCurve", MFnData::kNurbsCurve, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	aInputMesh = tAttr.create("inputMesh", "inputMesh", MFnData::kMesh, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	aCurvePoints = tAttr.create("outCurvePoints", "outCurvePoints", MFnPointArrayData::kPointArray, &status);
	tAttr.setStorable(false);
	tAttr.setWritable(false);
	
	MPointArray			defaultPoints;
	MFnPointArrayData	defaultArray;
	MObject				defaultAttr;

	defaultPoints.clear(); // Empty array
	defaultAttr = defaultArray.create (defaultPoints);
	status = tAttr.setDefault(defaultAttr);
	if (!status) {
		status.perror("could not create default output attribute");
		return status;
	}

	aMeshPoints = tAttr.create("outMeshPoints", "outMeshPoints", MFnPointArrayData::kPointArray, &status);
	tAttr.setStorable(false);
	tAttr.setWritable(false);
	
	defaultPoints.clear(); 
	defaultAttr = defaultArray.create (defaultPoints);
	status = tAttr.setDefault(defaultAttr);
	if (!status) {
		status.perror("could not create default output attribute");
		return status;
	}

	aMeshClosestPoints = tAttr.create("outMeshClosestPoints", "outMeshClosestPoints", MFnPointArrayData::kPointArray, &status);
	tAttr.setStorable(false);
	tAttr.setWritable(false);
	
	defaultPoints.clear(); 
	defaultAttr = defaultArray.create (defaultPoints);
	status = tAttr.setDefault(defaultAttr);
	if (!status) {
		status.perror("could not create default output attribute");
		return status;
	}

	aMeshPointIndecies = tAttr.create("outMeshPointIndecies", "outMeshPointIndecies", MFnNumericData::kIntArray, &status);
	tAttr.setStorable(false);
	tAttr.setWritable(false);

	MIntArray defaultIndecies;
	MFnIntArrayData defaultIndeciesArray;
	MObject defaultIndex;

	defaultIndecies.clear(); // Empty array
	defaultAttr = defaultIndeciesArray.create (defaultIndecies);
	status = tAttr.setDefault(defaultAttr);

	if (!status) {
		status.perror("could not create default output attribute");
		return status;
	}


	aPointSize = nAttr.create("debugPointSize", "debugPointSize", MFnNumericData::kFloat, 1.0f);
	nAttr.setKeyable(true);
	nAttr.setMin(0.0);

	aDrawCurvePoints = nAttr.create("debugDrawCurvePoints", "debugDrawCurvePoints", MFnNumericData::kBoolean, true);
	aDrawMeshPoints  = nAttr.create("debugDrawMeshPoints", "debugDrawMeshPoints", MFnNumericData::kBoolean, true);
	aDrawLines       = nAttr.create("debugDrawLines", "debugDrawLines", MFnNumericData::kBoolean, true);
	aDrawNumbers     = nAttr.create("debugDrawNumbers", "debugDrawNumbers", MFnNumericData::kBoolean, true);


	addAttribute(aInputCurve);
	addAttribute(aInputMesh);
	addAttribute(aPointSize);
	addAttribute(aCurvePoints);
	addAttribute(aMeshPoints);
	addAttribute(aMeshClosestPoints);
	addAttribute(aMeshPointIndecies);

	addAttribute(aDrawCurvePoints);
	addAttribute(aDrawMeshPoints);
	addAttribute(aDrawLines);
	addAttribute(aDrawNumbers);

	attributeAffects(aInputCurve, aCurvePoints);
	attributeAffects(aInputMesh, aCurvePoints);

	attributeAffects(aInputCurve, aMeshPoints);
	attributeAffects(aInputMesh, aMeshPoints);

	attributeAffects(aInputCurve, aMeshClosestPoints);
	attributeAffects(aInputMesh, aMeshClosestPoints);

	attributeAffects(aInputCurve, aMeshPointIndecies);
	attributeAffects(aInputMesh, aMeshPointIndecies);

	return MS::kSuccess;
}

void curveLocator::draw(M3dView& view, const MDagPath& DGpath, M3dView::DisplayStyle style, M3dView::DisplayStatus status)
{
	MStatus stat;

	MObject thisNode = thisMObject();

	MPlug drawCurvePointsPlug = MPlug(thisNode, aDrawCurvePoints);
	MPlug drawMeshPointsPlug = MPlug(thisNode, aDrawMeshPoints);
	MPlug drawLinesPlug = MPlug(thisNode, aDrawLines);
	MPlug drawNumbersPlug = MPlug(thisNode, aDrawNumbers);

	bool drawCurve, drawMesh, drawLines, drawNumbers;
	drawCurvePointsPlug.getValue(drawCurve);
	drawMeshPointsPlug.getValue(drawMesh);
	drawLinesPlug.getValue(drawLines);
	drawNumbersPlug.getValue(drawNumbers);

	MPlug sizePlug = MPlug(thisNode, aPointSize);
	float size;
	sizePlug.getValue(size);

	MPlug pointsPlug( thisNode, aCurvePoints );
	MObject pointsObject;
	stat = pointsPlug.getValue(pointsObject);
	MFnPointArrayData pointsData(pointsObject, &stat);
	MPointArray points = pointsData.array( &stat );
	int numPoints = points.length();

	MPlug meshPointsPlug( thisNode, aMeshPoints );
	MObject meshPointsObject;
	stat = meshPointsPlug.getValue(meshPointsObject);
	MFnPointArrayData meshPointsData(meshPointsObject, &stat);
	MPointArray meshPoints = meshPointsData.array( &stat );

	MPlug meshClosestPointsPlug( thisNode, aMeshClosestPoints );
	MObject meshClosestPointsObject;
	stat = meshClosestPointsPlug.getValue(meshClosestPointsObject);
	MFnPointArrayData meshClosestPointsData(meshClosestPointsObject, &stat);
	MPointArray meshClosestPoints = meshClosestPointsData.array( &stat );

	MPlug indeciesPlug(thisNode, aMeshPointIndecies);
	MObject meshPointIndeciesObject;
	stat = indeciesPlug.getValue(meshPointIndeciesObject);
	MFnIntArrayData indeciesData(meshPointIndeciesObject, &stat);
	MIntArray meshIndecies = indeciesData.array(&stat);

	MColor curvePointsColor, meshPointsColor, lineColor, textColor;
	curvePointsColor = MColor( 0.7294f, .239216f, 0.2980f, 1.0f );
	meshPointsColor  = MColor( 0.5843f, 0.78824f, .17255f, 1.0f );
	lineColor        = MColor( 0.898f, 0.3255f, 0.2667f, 1.0f );
	textColor        = MColor( 0.0f, 0.0f, 0.0f, 1.0f);

	view.beginGL(); 
	for (int i = 0; i < numPoints; i++) {
		MPoint center(points[i]);
		
		if (drawCurve) {
			// draw the curve points
			glColor4f( curvePointsColor.r, curvePointsColor.g, curvePointsColor.b, curvePointsColor.a );
			drawSquare(center, size);
		}

		if (drawMesh) {
			// draw the mesh points
			glColor4f( meshPointsColor.r, meshPointsColor.g, meshPointsColor.b, meshPointsColor.a );
			drawSquare(meshPoints[i], size);
		}

		if (drawLines) {
			// draw the lines
			glColor4f( lineColor.r, lineColor.g, lineColor.b, lineColor.a );
			drawLine( center, meshPoints[i] );

			glColor4f( meshPointsColor.r, meshPointsColor.g, meshPointsColor.b, 0.25f );
			drawLine( center, meshClosestPoints[i] );
		}

		if (drawNumbers) {
			// draw the id
			glColor4f( textColor.r, textColor.g, textColor.b, textColor.a );
			view.drawText(MString() + int(meshIndecies[i]), meshPoints[i]);
		}
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glPopAttrib();

	view.endGL();

}

void curveLocator::drawLine(MPoint start, MPoint end)
{
	glBegin( GL_LINES );

	glVertex3f( (float)start.x, (float)start.y, (float)start.z);
	glVertex3f( (float)end.x, (float)end.y, (float)end.z);

	glEnd();
}

void curveLocator::drawSquare(MPoint center, float size)
{
	glPushAttrib( GL_CURRENT_BIT | GL_POINT_BIT );
	glPointSize( size * 10.0f );
	glEnable ( GL_POINT_SMOOTH ); 

	glBegin( GL_POINTS );
	glVertex3f( (float)center.x, (float)center.y, (float)center.z);
	glEnd();

	glPopAttrib();
}

void curveLocator::drawCube(MPoint center, float size, bool filled)
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

		glVertex3fv(ver[quads[i][0]]);
		glVertex3fv(ver[quads[i][1]]);
		glVertex3fv(ver[quads[i][2]]);
		glVertex3fv(ver[quads[i][3]]);

		glEnd();
	}
}

bool curveLocator::isBounded() const
{ 
	return true;
}


bool curveLocator::isTransparent() const
{ 
	return true;
}


MBoundingBox curveLocator::boundingBox() const
{
	MBoundingBox bbox;
	MStatus stat;

	MObject thisNode = thisMObject();

	MPlug sizePlug = MPlug(thisNode, aPointSize);
	float size;
	sizePlug.getValue(size);

	MPlug pointsPlug( thisNode, aCurvePoints );
	MObject pointsObject;
	stat = pointsPlug.getValue(pointsObject);
	MFnPointArrayData pointsData(pointsObject, &stat);
	MPointArray points = pointsData.array( &stat );
	int numPoints = points.length();

	MPlug meshPointsPlug( thisNode, aMeshPoints );
	MObject meshPointsObject;
	stat = meshPointsPlug.getValue(meshPointsObject);
	MFnPointArrayData meshPointsData(meshPointsObject, &stat);
	MPointArray meshPoints = meshPointsData.array( &stat );

	MPlug meshClosestPointsPlug( thisNode, aMeshClosestPoints );
	MObject meshClosestPointsObject;
	stat = meshClosestPointsPlug.getValue(meshClosestPointsObject);
	MFnPointArrayData meshClosestPointsData(meshClosestPointsObject, &stat);
	MPointArray meshClosestPoints = meshClosestPointsData.array( &stat );

	for (int i = 0; i < numPoints; i++) {
		bbox.expand(points[i]);
		bbox.expand(meshPoints[i]);
		bbox.expand(meshClosestPoints[i]);
	}


	return bbox;
}

void* curveLocator::creator()
{
	return new curveLocator();
}

void curveLocator::postConstructor()
{
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Viewport 2.0 override implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

MPointArray CurveLocatorDrawOverride::getCurvePoints(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return MPointArray();
	}

	MPlug pointsPlug(locatorNode, curveLocator::aCurvePoints);
	MObject pointsObject;
	status = pointsPlug.getValue(pointsObject);
	MFnPointArrayData pointsData(pointsObject, &status);
	MPointArray points = pointsData.array(&status);

	return points;
}

MPointArray CurveLocatorDrawOverride::getMeshPoints(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return MPointArray();
	}

	MPlug pointsPlug(locatorNode, curveLocator::aMeshPoints);
	MObject pointsObject;
	status = pointsPlug.getValue(pointsObject);
	MFnPointArrayData pointsData(pointsObject, &status);
	MPointArray points = pointsData.array(&status);

	return points;
}

MIntArray CurveLocatorDrawOverride::getMeshPointIndecies(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return MIntArray();
	}

	MPlug meshPointIndeciesPlug(locatorNode, curveLocator::aMeshPointIndecies);
	MObject meshPointIndeciesObject;
	status = meshPointIndeciesPlug.getValue(meshPointIndeciesObject);
	MFnIntArrayData meshPointIndeciesData(meshPointIndeciesObject, &status);
	MIntArray meshPointIndecies = meshPointIndeciesData.array(&status);

	return meshPointIndecies;
}


CurveLocatorDrawOverride::CurveLocatorDrawOverride(const MObject& obj) : MHWRender::MPxDrawOverride(obj, CurveLocatorDrawOverride::draw)
{
}

CurveLocatorDrawOverride::~CurveLocatorDrawOverride()
{
}

MHWRender::DrawAPI CurveLocatorDrawOverride::supportedDrawAPIs() const
{
	// this plugin supports both GL and DX
	return (MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}

bool CurveLocatorDrawOverride::isBounded(const MDagPath&, const MDagPath&) const
{
	return true;
}

MBoundingBox CurveLocatorDrawOverride::boundingBox(const MDagPath& objPath, const MDagPath& cameraPath) const
{
	MBoundingBox bbox;
	MStatus status;

	MPointArray curvePoints     = getCurvePoints(objPath);
	MPointArray meshPoints      = getMeshPoints(objPath);
	
	for (size_t i=0; i<curvePoints.length(); i++) {
		int index = int(i);
		bbox.expand(curvePoints[index]);
		bbox.expand(meshPoints[index]);
	}

	return MBoundingBox();
}

MUserData* CurveLocatorDrawOverride::prepareForDraw(
	const MDagPath& objPath,
	const MDagPath& cameraPath,
	const MHWRender::MFrameContext& frameContext,
	MUserData* oldData)
{
	CurveLocatorData* data = dynamic_cast<CurveLocatorData*>(oldData);
	if (!data)
	{
		data = new CurveLocatorData();
	}

	MStatus status;

	data->fCurvePoints = getCurvePoints(objPath);
	data->fMeshPoints  = getMeshPoints(objPath);
	data->fIndecies    = getMeshPointIndecies(objPath);

	return data;
}

void CurveLocatorDrawOverride::addUIDrawables(
		const MDagPath& objPath,
		MHWRender::MUIDrawManager& drawManager,
		const MHWRender::MFrameContext& frameContext,
		const MUserData* data)
{
	MString info = "Drawing";
	MGlobal::displayInfo(info);

	CurveLocatorData* pLocatorData = (CurveLocatorData*)data;
	if (!pLocatorData)
	{
		return;
	}

	MColor curvePointsColor, meshPointsColor, lineColor, textColor;
	curvePointsColor = MColor(0.7294f, .239216f, 0.2980f, 1.0f);
	meshPointsColor  = MColor(0.5843f, 0.78824f, .17255f, 1.0f);
	lineColor        = MColor(0.898f, 0.3255f, 0.2667f, 1.0f);
	textColor        = MColor(0.0f, 0.0f, 0.0f, 1.0f);

	drawManager.beginDrawable();

	drawManager.setColor(curvePointsColor);
	drawManager.setDepthPriority(5);

	drawManager.points(pLocatorData->fCurvePoints, true);
	drawManager.points(pLocatorData->fMeshPoints, true);

	MPoint pos(0.0, 0.0, 0.0);
	drawManager.text(pos,  MString("Sven"), MHWRender::MUIDrawManager::kCenter);

	drawManager.endDrawable();
}