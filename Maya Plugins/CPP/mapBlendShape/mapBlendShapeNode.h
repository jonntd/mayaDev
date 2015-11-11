#ifndef _mapBlendShapeNode
#define _mapBlendShapeNode

#include <maya/MItGeometry.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MFnData.h>
#include <maya/MPlugArray.h>
#include <maya/MImage.h>
#include <maya/MDynamicsUtil.h>
#include <vector>
 
class mapBlendShape : public MPxDeformerNode
{
public:
						mapBlendShape();
	virtual				~mapBlendShape(); 

	virtual MStatus     deform( MDataBlock& data,
                                MItGeometry& itGeo,
                                const MMatrix& localToWorldMatrix,
                                unsigned int geomIndex );

	static  void*		creator();
	static  MStatus		initialize();

public:

	static  MObject		aBlendMesh;		
	static  MObject		aBlendMap;		
	static  MObject     aUseBlendMap;
	static  MObject		aBlendMapMultiplier;

	static	MTypeId		id;

};

#endif
