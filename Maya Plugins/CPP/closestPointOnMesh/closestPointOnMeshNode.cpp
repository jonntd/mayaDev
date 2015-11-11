#include "closestPointOnMeshNode.h"

#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <maya/MGlobal.h>


MTypeId closestPointOnMesh::id(0x80009);
MString	closestPointOnMesh::drawDbClassification("drawdb/geometry/closestPointOnMesh");
MString	closestPointOnMesh::drawRegistrantId("closestPointOnMeshNodePlugin");


MObject closestPointOnMesh::aInputCurve;
MObject closestPointOnMesh::aInputMesh;
MObject closestPointOnMesh::aPointSize;
MObject closestPointOnMesh::aCurvePoints;
MObject closestPointOnMesh::aMeshPoints;
MObject closestPointOnMesh::aMeshClosestPoints;
MObject closestPointOnMesh::aMeshPointIndecies;

closestPointOnMesh::closestPointOnMesh() {}
closestPointOnMesh::~closestPointOnMesh() {}

MStatus closestPointOnMesh::compute( const MPlug& plug, MDataBlock& data )
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

void* closestPointOnMesh::creator()
{
	return new closestPointOnMesh();
}

MStatus closestPointOnMesh::initialize()
		
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

	addAttribute(aInputCurve);
	addAttribute(aInputMesh);
	addAttribute(aPointSize);
	addAttribute(aCurvePoints);
	addAttribute(aMeshPoints);
	addAttribute(aMeshClosestPoints);
	addAttribute(aMeshPointIndecies);

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

// called by legacy default viewport
void closestPointOnMesh::draw( M3dView & view, const MDagPath & /*path*/,
							 M3dView::DisplayStyle style,
							 M3dView::DisplayStatus status )
{
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
		
		// draw the curve points
		glColor4f( curvePointsColor.r, curvePointsColor.g, curvePointsColor.b, curvePointsColor.a );
		drawSquare(center, size);

		// draw the mesh points
		glColor4f( meshPointsColor.r, meshPointsColor.g, meshPointsColor.b, meshPointsColor.a );
		drawSquare(meshPoints[i], size);

		// draw the lines
		glColor4f( lineColor.r, lineColor.g, lineColor.b, lineColor.a );
		drawLine( center, meshPoints[i] );

		glColor4f( meshPointsColor.r, meshPointsColor.g, meshPointsColor.b, 0.25f );
		drawLine( center, meshClosestPoints[i] );

		// draw the id
		glColor4f( textColor.r, textColor.g, textColor.b, textColor.a );
		view.drawText(MString() + int(meshIndecies[i]), meshPoints[i]);
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glPopAttrib();

	view.endGL();
}

bool closestPointOnMesh::isTransparent() const
{ 
	return true;
}


bool closestPointOnMesh::isBounded() const
{
	return true;
}

MBoundingBox closestPointOnMesh::boundingBox() const
{
	MBoundingBox bbox;
	float size = 1.0f;

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

	for (int i=0; i<8; i++) {
		bbox.expand(ver[i]);
	}

	return bbox;
}

void closestPointOnMesh::drawLine(MPoint start, MPoint end)
{
	glBegin( GL_LINES );

	glVertex3f( (float)start.x, (float)start.y, (float)start.z);
	glVertex3f( (float)end.x, (float)end.y, (float)end.z);

	glEnd();
}

void closestPointOnMesh::drawSquare(MPoint center, float size)
{
	glPushAttrib( GL_CURRENT_BIT | GL_POINT_BIT );
	glPointSize( size * 10.0f );
	glEnable ( GL_POINT_SMOOTH ); 

	glBegin( GL_POINTS );
	glVertex3f( (float)center.x, (float)center.y, (float)center.z);
	glEnd();

	glPopAttrib();
}

void closestPointOnMesh::drawCube(MPoint center, float size, bool filled)
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

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Viewport 2.0 override implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
closestPointOnMeshDrawOverride::closestPointOnMeshDrawOverride(const MObject& obj)
: MHWRender::MPxDrawOverride(obj, closestPointOnMeshDrawOverride::draw)
{
}

closestPointOnMeshDrawOverride::~closestPointOnMeshDrawOverride()
{
}

MHWRender::DrawAPI closestPointOnMeshDrawOverride::supportedDrawAPIs() const
{
	// this plugin supports both GL and DX
	return (MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}

bool closestPointOnMeshDrawOverride::isBounded(const MDagPath& /*objPath*/,
                                      const MDagPath& /*cameraPath*/) const
{
	return true;
}

MBoundingBox closestPointOnMeshDrawOverride::boundingBox(
	const MDagPath& objPath,
	const MDagPath& cameraPath) const
{
	MBoundingBox bbox;
	MStatus status;

	MPointArray curvePoints       = getCurvePoints(objPath);
	MPointArray meshPoints        = getMeshPoints(objPath);
	MPointArray meshClosestPoints = getMeshClosestPoints(objPath);
	
	for (size_t i=0; i<curvePoints.length(); i++) {
		int index = int(i);
		bbox.expand(curvePoints[index]);
		bbox.expand(meshPoints[index]);
		bbox.expand(meshClosestPoints[index]);
	}

	return bbox;
}

MPointArray closestPointOnMeshDrawOverride::getCurvePoints(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return MPointArray();
	}

	MPlug pointsPlug(locatorNode, closestPointOnMesh::aCurvePoints);
	MObject pointsObject;
	status = pointsPlug.getValue(pointsObject);
	MFnPointArrayData pointsData(pointsObject, &status);
	MPointArray points = pointsData.array(&status);

	return points;
}

MPointArray closestPointOnMeshDrawOverride::getMeshPoints(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return MPointArray();
	}

	MPlug pointsPlug(locatorNode, closestPointOnMesh::aMeshPoints);
	MObject pointsObject;
	status = pointsPlug.getValue(pointsObject);
	MFnPointArrayData pointsData(pointsObject, &status);
	MPointArray points = pointsData.array(&status);

	return points;
}

MPointArray closestPointOnMeshDrawOverride::getMeshClosestPoints(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return MPointArray();
	}

	MPlug pointsPlug(locatorNode, closestPointOnMesh::aMeshClosestPoints);
	MObject pointsObject;
	status = pointsPlug.getValue(pointsObject);
	MFnPointArrayData pointsData(pointsObject, &status);
	MPointArray points = pointsData.array(&status);

	return points;
}

MIntArray closestPointOnMeshDrawOverride::getMeshPointIndecies(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return MIntArray();
	}

	MPlug meshPointIndeciesPlug(locatorNode, closestPointOnMesh::aMeshPointIndecies);
	MObject meshPointIndeciesObject;
	status = meshPointIndeciesPlug.getValue(meshPointIndeciesObject);
	MFnIntArrayData meshPointIndeciesData(meshPointIndeciesObject, &status);
	MIntArray meshPointIndecies = meshPointIndeciesData.array(&status);

	return meshPointIndecies;
}

float closestPointOnMeshDrawOverride::getPointSize(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return 1.0f;
	}

	MPlug pointSizePlug(locatorNode, closestPointOnMesh::aPointSize);
	float size = pointSizePlug.asFloat();

	return size;
}

// Called by Maya each time the object needs to be drawn.
MUserData* closestPointOnMeshDrawOverride::prepareForDraw(
	const MDagPath& objPath,
	const MDagPath& cameraPath,
	const MHWRender::MFrameContext& frameContext,
	MUserData* oldData)
{
	closestPointOnMeshData* data = dynamic_cast<closestPointOnMeshData*>(oldData);
	if (!data)
	{
		data = new closestPointOnMeshData();
	}

	data->fCurvePoints		 = getCurvePoints(objPath);
	data->fMeshPoints		 = getMeshPoints(objPath);
	data->fMeshClosestPoints = getMeshClosestPoints(objPath);
	data->fMeshIndecies		 = getMeshPointIndecies(objPath);

	data->fPointSize		 = getPointSize(objPath);
	
	return data;
}

void closestPointOnMeshDrawOverride::addUIDrawables(
		const MDagPath& objPath,
		MHWRender::MUIDrawManager& drawManager,
		const MHWRender::MFrameContext& frameContext,
		const MUserData* data)
{
	// Get data cached by prepareForDraw() for each drawable instance, then MUIDrawManager 
	// can draw simple UI by these data.
	closestPointOnMeshData* pLocatorData = (closestPointOnMeshData*)data;
	if (!pLocatorData)
	{
		return;
	}

	drawManager.beginDrawable();

	MColor curvePointsColor, meshPointsColor, lineColor, textColor, lineColor2;
	curvePointsColor = MColor(0.7294f, .239216f, 0.2980f, 1.0f);
	meshPointsColor  = MColor(0.5843f, 0.78824f, .17255f, 1.0f);
	lineColor        = MColor(0.898f, 0.3255f, 0.2667f, 1.0f);
	lineColor2       = MColor(0.235f, 0.42353f, 0.76f, 1.0f);
	textColor        = MColor(0.0f, 0.0f, 0.0f, 1.0f);

	drawManager.setDepthPriority(5);

	MPointArray curvePoints       = pLocatorData->fCurvePoints;
	MPointArray meshPoints        = pLocatorData->fMeshPoints;
	MPointArray meshClosestPoints = pLocatorData->fMeshClosestPoints;
	MIntArray   meshIndecies      = pLocatorData->fMeshIndecies;
	float       pointSize         = pLocatorData->fPointSize;
	
	for (int i=0; i<curvePoints.length(); i++) {
		drawManager.setColor(curvePointsColor);
		drawManager.sphere(curvePoints[i], pointSize*0.1, true);

		drawManager.setColor(meshPointsColor);
		drawManager.sphere(meshClosestPoints[i], pointSize*0.05, true);

		drawManager.setColor(meshPointsColor);
		drawManager.sphere(meshPoints[i], pointSize*0.1, true);

		drawManager.setColor(lineColor);
		drawManager.line(curvePoints[i], meshPoints[i]);

		drawManager.setColor(lineColor2);
		drawManager.line(curvePoints[i], meshClosestPoints[i]);

		drawManager.setColor(textColor);
		drawManager.text(meshPoints[i], MString() + int(meshIndecies[i]));
	}

	drawManager.endDrawable();
}


//MStatus closestPointOnMeshCommand::doIt( const MArgList& args ) 
//{
//	MStatus status;
//
//	MSelectionList activeSelectionList;
//	MGlobal::getActiveSelectionList(activeSelectionList);
//
//	if (activeSelectionList.isEmpty() || activeSelectionList.length() < 2) {
//		status.perror("Please select a Polygon Mesh and than a Nurbs Curve");
//		return MS::kFailure;
//	}
//
//	MObject meshObj;
//	activeSelectionList.getDependNode(0, meshObj);
//
//	MFnMesh meshFn(meshObj, &status);
//	if (!status) {
//		status.perror("First Selected Object is not a mesh");
//		return MS::kFailure;
//	}
//
//	MObject curveObj;
//	activeSelectionList.getDependNode(1, curveObj);
//
//	MFnNurbsCurve curveFn(curveObj, &status);
//	if (!status) {
//		status.perror("Second Selected Object is not a curve");
//		return MS::kFailure;
//	}
//
//
//	// create the new node
//	// give it a nice name
//	// hook up the mesh
//	// hook up the curve
//	
//
//    return MS::kSuccess;
//}
//
//void* closestPointOnMeshCommand::creator() {
//    return new closestPointOnMeshCommand;
//}