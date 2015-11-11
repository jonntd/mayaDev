#ifndef _GaussianFKNode
#define _GaussianFKNode
//
// Copyright (C) 
// 
// File: GaussianFKNode.h
//
// Dependency Graph Node: GaussianFK
//
// Author: Maya Plug-in Wizard 2.0
//

#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MTypeId.h> 
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MIOStream.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MFnNurbsCurveData.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>

#include <vector>
#include <array>
#include <math.h>


 
class GaussianFK : public MPxNode
{
public:
						GaussianFK();
	virtual				~GaussianFK(); 

	virtual MStatus		compute( const MPlug& plug, MDataBlock& data );

	static  void*		creator();
	static  MStatus		initialize();

public:
	static MObject aInputMatrices;
	static MObject aParentInverseMatrices;
	static MObject aControllers;

	static MObject aRotationX;
	static MObject aRotationY;
	static MObject aRotationZ;
	static MObject aRotation;

	static MObject aScale;
	static MObject aLocation;
	static MObject aSharpness;
	static MObject aAmplitude;

	static MObject aOutputMatrices;
	static MObject aOutputControllerMatrices;

	static	MTypeId		id;
};

#endif
