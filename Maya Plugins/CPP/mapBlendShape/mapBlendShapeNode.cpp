#include "mapBlendShapeNode.h"

// create attribute objects
MTypeId     mapBlendShape::id(0x000001);
MObject     mapBlendShape::aBlendMesh;        
MObject     mapBlendShape::aBlendMap; 
MObject		mapBlendShape::aUseBlendMap;
MObject     mapBlendShape::aBlendMapMultiplier;

// global array to hold the luminance values
std::vector<float> lumValues;

mapBlendShape::mapBlendShape() {}
mapBlendShape::~mapBlendShape() {}

MStatus mapBlendShape::deform(MDataBlock& data, 
							  MItGeometry& itGeo, 
							  const MMatrix& localToWorldMatrix, 
							  unsigned int geomIndex)
{
    MStatus status;

	// get the blendMesh 
	MDataHandle hBlendMesh = data.inputValue( aBlendMesh, &status );
    CHECK_MSTATUS_AND_RETURN_IT( status );
    MObject oBlendMesh = hBlendMesh.asMesh();
    if (oBlendMesh.isNull())
    {
        return MS::kSuccess;
    }

	MFnMesh fnMesh( oBlendMesh, &status );
    CHECK_MSTATUS_AND_RETURN_IT( status );
    MPointArray blendPoints;
    fnMesh.getPoints( blendPoints );

	// get the dirty flags for the input and blendMap
	bool inputGeomClean = data.isClean(inputGeom, &status);
	bool blendMapClean  = data.isClean(aBlendMap, &status);

	if (!blendMapClean) {
		lumValues.reserve(itGeo.count());
	}
	
	MDoubleArray uCoords, vCoords;
	MVectorArray resultColors;
	MDoubleArray resultAlphas;

	uCoords.setLength(1);
	vCoords.setLength(1);

	bool hasTextureNode;
	bool useBlendMap = data.inputValue(aUseBlendMap).asBool();
	float blendMapMultiplier = data.inputValue(aBlendMapMultiplier).asFloat();

	if (blendMapMultiplier<=0.0) {
		useBlendMap = false;
	}

	if (useBlendMap) {
		hasTextureNode = MDynamicsUtil::hasValidDynamics2dTexture(thisMObject(), aBlendMap);
	}

	float env = data.inputValue(envelope).asFloat();
    MPoint point;
	float2 uvPoint;
    float w, lum;

    for ( ; !itGeo.isDone(); itGeo.next() )
    {
		lum = 1.0;

		if (useBlendMap) {
			if (!blendMapClean) {
				fnMesh.getUVAtPoint(blendPoints[itGeo.index()], uvPoint);

				if (hasTextureNode) {
					uCoords[0] = uvPoint[0];
					vCoords[0] = uvPoint[1];
					MDynamicsUtil::evalDynamics2dTexture(thisMObject(), aBlendMap, uCoords, vCoords, &resultColors, &resultAlphas);
					lum = float(resultColors[0][0]);
				}
				lumValues[itGeo.index()] = lum;
			} else {
				lum = lumValues[itGeo.index()];
			}
		}

        point = itGeo.position();
        w = weightValue( data, geomIndex, itGeo.index() );
        point += (blendPoints[itGeo.index()] - point) * env * w * lum * blendMapMultiplier;
        itGeo.setPosition( point );
    }

	return MS::kSuccess;
}


void* mapBlendShape::creator()
{
	return new mapBlendShape();
}

MStatus mapBlendShape::initialize()
{
	MStatus status;

	MFnNumericAttribute nAttr;
	MFnTypedAttribute tAttr;

	aBlendMesh = tAttr.create("blendMesh", "blendMesh", MFnData::kMesh);
	addAttribute(aBlendMesh);
	
	aBlendMap = nAttr.createColor("blendMap", "blendMap");
	addAttribute(aBlendMap);

	aUseBlendMap = nAttr.create("useBlendMap", "useBlendMap", MFnNumericData::kBoolean);
	nAttr.setDefault(true);
	addAttribute(aUseBlendMap);

	aBlendMapMultiplier = nAttr.create("blendMapMultiplier", "blendMapMultiplier", MFnNumericData::kFloat);
	nAttr.setDefault(1.0);
	nAttr.setMin(0.0);
	addAttribute(aBlendMapMultiplier);
	
	attributeAffects(aBlendMesh, outputGeom);
	attributeAffects(aBlendMap, outputGeom);
	attributeAffects(aUseBlendMap, outputGeom);
	attributeAffects(aBlendMapMultiplier, outputGeom);

	MGlobal::executeCommand( "makePaintable -attrType multiFloat -sm deformer blendNode weights;" );

	return MS::kSuccess;
}

