#include "sphericalBlendShapeVisualizeNode.h"

#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <maya/MGlobal.h>

MTypeId sphericalBlendShapeVisualizer::id(0x00009);
MObject sphericalBlendShapeVisualizer::aSpaceMatrix;
MObject sphericalBlendShapeVisualizer::aPoleAxis;
MObject sphericalBlendShapeVisualizer::aSeamAxis;

MString	sphericalBlendShapeVisualizer::drawDbClassification("drawdb/geometry/sphericalSpaceVisualizer");
MString	sphericalBlendShapeVisualizer::drawRegistrantId("sphericalSpaceVisualizerNodePlugin");

sphericalBlendShapeVisualizer::sphericalBlendShapeVisualizer() {}
sphericalBlendShapeVisualizer::~sphericalBlendShapeVisualizer() {}

void* sphericalBlendShapeVisualizer::creator()
{
	return new sphericalBlendShapeVisualizer();
}

MStatus sphericalBlendShapeVisualizer::initialize()
		
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

	addAttribute(aSpaceMatrix);
	addAttribute(aPoleAxis);
	addAttribute(aSeamAxis);

	return MS::kSuccess;
}


// called by legacy default viewport
void sphericalBlendShapeVisualizer::draw( M3dView & view, const MDagPath & /*path*/,
							 M3dView::DisplayStyle style,
							 M3dView::DisplayStatus status )
{

}


MBoundingBox sphericalBlendShapeVisualizer::boundingBox() const
{
	MBoundingBox bbox;

	return bbox;
}

bool sphericalBlendShapeVisualizer::isTransparent() const
{ 
	return true;
}


bool sphericalBlendShapeVisualizer::isBounded() const
{
	return true;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Viewport 2.0 override implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
sphericalBlendShapeVisualizerDrawOverride::sphericalBlendShapeVisualizerDrawOverride(const MObject& obj)
: MHWRender::MPxDrawOverride(obj, sphericalBlendShapeVisualizerDrawOverride::draw)
{
}

sphericalBlendShapeVisualizerDrawOverride::~sphericalBlendShapeVisualizerDrawOverride()
{
}

MHWRender::DrawAPI sphericalBlendShapeVisualizerDrawOverride::supportedDrawAPIs() const
{
	// this plugin supports both GL and DX
	return (MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}

bool sphericalBlendShapeVisualizerDrawOverride::isBounded(const MDagPath& /*objPath*/,
                                      const MDagPath& /*cameraPath*/) const
{
	return true;
}

MBoundingBox sphericalBlendShapeVisualizerDrawOverride::boundingBox(
	const MDagPath& objPath,
	const MDagPath& cameraPath) const
{
	MBoundingBox bbox;
	double radius = 1.0;

	MMatrix spaceMatrix = getSpaceMatrix(objPath);

	/*for(unsigned int i=0; i<bboxPoints.length(); i++) {
		bbox.expand(bboxPoints[i]);
	}*/


	bbox.expand(MPoint(-radius, 0.0, 0.0) * spaceMatrix);
	bbox.expand(MPoint(radius, 0.0, 0.0) * spaceMatrix);

	bbox.expand(MPoint(0.0, -radius, 0.0) * spaceMatrix);
	bbox.expand(MPoint(0.0, radius, 0.0) * spaceMatrix);

	bbox.expand(MPoint(0.0, 0.0, -radius) * spaceMatrix);
	bbox.expand(MPoint(0.0, 0.0, radius) * spaceMatrix);


	return bbox;
}

// Called by Maya each time the object needs to be drawn.
MUserData* sphericalBlendShapeVisualizerDrawOverride::prepareForDraw(
	const MDagPath& objPath,
	const MDagPath& cameraPath,
	const MHWRender::MFrameContext& frameContext,
	MUserData* oldData)
{
	sphericalBlendShapeVisualizerData* data = dynamic_cast<sphericalBlendShapeVisualizerData*>(oldData);
	if (!data)
	{
		data = new sphericalBlendShapeVisualizerData();
	}

	data->fSpaceMatrix = getSpaceMatrix(objPath);
	data->fPoleAxis    = getPoleAxis(objPath);
	data->fSeamAxis    = getSeamAxis(objPath);

	return data;
}

void sphericalBlendShapeVisualizerDrawOverride::addUIDrawables(
								const MDagPath& objPath,
								MHWRender::MUIDrawManager& drawManager,
								const MHWRender::MFrameContext& frameContext,
								const MUserData* data)
{
	sphericalBlendShapeVisualizerData* pLocatorData = (sphericalBlendShapeVisualizerData*)data;
	if (!pLocatorData)
	{
		return;
	}

	MMatrix spaceMatrix    = pLocatorData->fSpaceMatrix;
	MMatrix spaceInvMatrix = spaceMatrix.inverse();

	short poleAxis      = pLocatorData->fPoleAxis;
	short seamAxis      = pLocatorData->fSeamAxis;

	drawManager.beginDrawable();

	MColor lineColor, vertexColor, poleAxisColor, seamAxisColor;
	lineColor		= MColor(0.7294f, .239216f, 0.2980f, 1.0f);
	vertexColor		= MColor(0.5843f, 0.78824f, .17255f, 1.0f);
	poleAxisColor   = MColor(1.0f, 0.0f, 0.f, 1.0f);
	seamAxisColor   = MColor(0.0f, 1.0f, 0.0f, 1.0f);

	MMatrix identity;
	identity.setToIdentity();

	double radius   = 1.0;
	int numRings    = 20;
	int numSections = 20;

	MPoint sphericalPoint, xyzPoint;
	MPoint startPoint, endPoint, firstPoint;
	MPointArray points;

	drawManager.setDepthPriority(5);

	drawManager.setColor(lineColor);

	bboxPoints.clear();
	bboxPoints.setLength(numRings*numSections);

	for(int ring=0; ring<=numRings; ring++) {
		double azimuth = (double)ring / numRings * M_PI * 2;

		for(int section=0; section<=numSections; section++) {
			double zenith = (double)section / (numSections) * M_PI;

			sphericalPoint.x = radius;
			sphericalPoint.y = zenith;
			sphericalPoint.z = azimuth;
			
			if (section==0) {
				sphericalToCartesian(sphericalPoint, poleAxis, seamAxis, startPoint);
				startPoint = startPoint * spaceMatrix;
				firstPoint = startPoint;
				bboxPoints.append(firstPoint);
				continue;
			} else {
				sphericalToCartesian(sphericalPoint, poleAxis, seamAxis, endPoint);
				endPoint = endPoint * spaceMatrix;
				drawManager.line(startPoint, endPoint);
				bboxPoints.append(endPoint);
				startPoint = endPoint;
			}
		}
	}

	for(int ring=0; ring<=numRings; ring++) {
		double azimuth = (double)ring / numRings * M_PI * 2;

		for(int section=0; section<=numSections; section++) {
			double zenith = (double)section / (numSections) * M_PI;

			sphericalPoint.x = radius;
			sphericalPoint.y = azimuth;
			sphericalPoint.z = zenith;
			
			if (section==0) {
				sphericalToCartesian(sphericalPoint, poleAxis, seamAxis, startPoint);
				startPoint = startPoint * spaceMatrix;
				firstPoint = startPoint;
				continue;
			} else {
				sphericalToCartesian(sphericalPoint, poleAxis, seamAxis, endPoint);
				endPoint = endPoint * spaceMatrix;
				drawManager.line(startPoint, endPoint);
				startPoint = endPoint;
			}
		}
	}

	drawManager.setLineWidth(3.0);
	drawManager.setLineStyle(MHWRender::MUIDrawManager::kDashed);
	
	startPoint = MPoint(0, 0, 0) * spaceMatrix;


	MVector endVector(0, 0, 0);
	setAxis(endVector, poleAxis, radius);
	endPoint = MPoint(endVector) * spaceMatrix;

	drawManager.setColor(poleAxisColor);
	drawManager.line(startPoint, endPoint);

	endVector = MVector(0, 0, 0);
	setAxis(endVector, seamAxis, radius);
	endPoint = MPoint(endVector) * spaceMatrix;

	drawManager.setColor(seamAxisColor);
	drawManager.line(startPoint, endPoint);
	drawManager.endDrawable();
}

MMatrix sphericalBlendShapeVisualizerDrawOverride::getSpaceMatrix(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return MMatrix();
	}

	MPlug spaceMatrixPlug(locatorNode, sphericalBlendShapeVisualizer::aSpaceMatrix);
	MObject spaceMatrixObject;
	status = spaceMatrixPlug.getValue(spaceMatrixObject);
	MFnMatrixData fnData(spaceMatrixObject);
	MMatrix matrix = fnData.matrix();

	return matrix;
}

short sphericalBlendShapeVisualizerDrawOverride::getPoleAxis(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return 0;
	}

	MPlug poleAxisPlug(locatorNode, sphericalBlendShapeVisualizer::aPoleAxis);
	
	return poleAxisPlug.asShort();
}

short sphericalBlendShapeVisualizerDrawOverride::getSeamAxis(const MDagPath& objPath) const
{
	MStatus status;

	MObject locatorNode = objPath.node(&status);
	if (!status)
	{
		return 0;
	}

	MPlug seamAxisPlug(locatorNode, sphericalBlendShapeVisualizer::aSeamAxis);
	
	return seamAxisPlug.asShort();
}

// Convert from spherical to cartesian coordinates
void sphericalBlendShapeVisualizerDrawOverride::sphericalToCartesian(const MPoint& aPoint, short aPoleAxis, short aSeamAxis, MPoint& aOutPoint) {
	const double radius  = aPoint[0];
    const double zenith  = aPoint[1];
    const double azimuth = aPoint[2];

	MVector result;

	setAxis(result, aSeamAxis, radius * sin(zenith) * cos(azimuth));
    setAxis(result, axisCross(aPoleAxis, aSeamAxis), radius * sin(zenith) * sin(azimuth));
    setAxis(result, aPoleAxis, radius * cos(zenith));

	aOutPoint.x = result[0];
	aOutPoint.y = result[1];
	aOutPoint.z = result[2];
}

void sphericalBlendShapeVisualizerDrawOverride::setAxis(MVector& aPoint, short axis, double value)
{
	if (axis > CUR_AXIS_POS_Z) {
        aPoint[axis % 3] = -value;
    } else {
        aPoint[axis % 3] = value;
    }
}

const CurAxis _axisCrossLUT[6][6] = {
               /* +X */    /* +Y */    /* +Z */    /* -X */    /* -Y */    /* -Z */
    /* +X */ {CUR_AXIS_BOGUS, CUR_AXIS_POS_Z, CUR_AXIS_NEG_Y, CUR_AXIS_BOGUS, CUR_AXIS_NEG_Z, CUR_AXIS_POS_Y},
    /* +Y */ {CUR_AXIS_NEG_Z, CUR_AXIS_BOGUS, CUR_AXIS_POS_X, CUR_AXIS_POS_Z, CUR_AXIS_BOGUS, CUR_AXIS_NEG_X},
    /* +Z */ {CUR_AXIS_POS_Y, CUR_AXIS_NEG_X, CUR_AXIS_BOGUS, CUR_AXIS_NEG_Y, CUR_AXIS_POS_X, CUR_AXIS_BOGUS},
    /* -X */ {CUR_AXIS_BOGUS, CUR_AXIS_NEG_Z, CUR_AXIS_POS_Y, CUR_AXIS_BOGUS, CUR_AXIS_POS_Z, CUR_AXIS_NEG_Y},
    /* -Y */ {CUR_AXIS_POS_Z, CUR_AXIS_BOGUS, CUR_AXIS_NEG_X, CUR_AXIS_NEG_Z, CUR_AXIS_BOGUS, CUR_AXIS_POS_X},
    /* -Z */ {CUR_AXIS_NEG_Y, CUR_AXIS_POS_X, CUR_AXIS_BOGUS, CUR_AXIS_POS_Y, CUR_AXIS_NEG_X, CUR_AXIS_BOGUS}
};

short sphericalBlendShapeVisualizerDrawOverride::axisCross(short aAxisA, short aAxisB) {
    return _axisCrossLUT[aAxisA][aAxisB];
}



