//
// Copyright (C) 
// 
// File: acMirrorMatrixNode.cpp
//
// Dependency Graph Node: acMirrorMatrix
//
// Author: Sven Pohle (Activision Central)
//

#include "acMirrorMatrixNode.h"

MTypeId mirrorMatrix::id(0x81035);

// local attributes
MObject mirrorMatrix::aInputMatrices;
MObject mirrorMatrix::aPivotMatrix;
MObject mirrorMatrix::aSymmetryPlane;
MObject mirrorMatrix::aFlippingMode;
MObject mirrorMatrix::aOutputMatrices;

mirrorMatrix::mirrorMatrix() {}
mirrorMatrix::~mirrorMatrix() {}

void* mirrorMatrix::creator()
{
	return new mirrorMatrix();
}

MStatus mirrorMatrix::initialize()
{
    MStatus status;
    MFnMatrixAttribute mAttr;
    MFnEnumAttribute eAttr;

    aInputMatrices = mAttr.create("inputMatrices", "inputMatrices");
    mAttr.setConnectable(true);
    mAttr.setKeyable(true);  
    mAttr.setStorable(true);
    mAttr.setReadable(true); 
    mAttr.setWritable(true);
    mAttr.setHidden(true);
    mAttr.setArray(true);
    mAttr.setUsesArrayDataBuilder(true);
    addAttribute(aInputMatrices);

    aPivotMatrix = mAttr.create("pivotMatrix", "pivotMatrix");
    mAttr.setConnectable(true);
    mAttr.setKeyable(true);  
    mAttr.setStorable(true);
    mAttr.setReadable(true); 
    mAttr.setWritable(true);
    mAttr.setHidden(true);
    addAttribute(aPivotMatrix);

    aSymmetryPlane = eAttr.create("symmetryPlace", "symmetryPlane", SYMMETRY_PLANE_XY, &status);
    eAttr.addField("xy", SYMMETRY_PLANE_XY);
    eAttr.addField("yz", SYMMETRY_PLANE_YZ);
    eAttr.addField("xz", SYMMETRY_PLANE_XZ);
    addAttribute(aSymmetryPlane);

    aFlippingMode = eAttr.create("flippingMode", "flippingMode", POSITION_MIRROR_X, &status);
    eAttr.addField("Flip Position, Mirror X Axis", POSITION_MIRROR_X);
    eAttr.addField("Flip Position, Mirror Y Axis", POSITION_MIRROR_Y);
    eAttr.addField("Flip Position, Mirror Z Axis", POSITION_MIRROR_Z);
    eAttr.addField("Position and Axes", POSITION_AND_AXES);
    addAttribute(aFlippingMode);

    aOutputMatrices = mAttr.create("outputMatrices", "outputMatrices");
    mAttr.setConnectable(true);
    mAttr.setKeyable(false);  
    mAttr.setStorable(false);
    mAttr.setWritable(false);
    mAttr.setHidden(true);
    mAttr.setArray(true);
    mAttr.setUsesArrayDataBuilder(true);
    addAttribute(aOutputMatrices);

    attributeAffects(aFlippingMode, aOutputMatrices);
    attributeAffects(aSymmetryPlane, aOutputMatrices);
    attributeAffects(aInputMatrices, aOutputMatrices);
    attributeAffects(aPivotMatrix, aOutputMatrices);
    
	return MStatus::kSuccess;
}

MStatus mirrorMatrix::compute(const MPlug& plug, MDataBlock& data)
{   
    MStatus status;

    MArrayDataHandle hInputMatrices = data.inputArrayValue(aInputMatrices, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    unsigned int inputCount = hInputMatrices.elementCount();

    MArrayDataHandle hOutputMatrices = data.outputArrayValue(aOutputMatrices, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MArrayDataBuilder builder(aOutputMatrices, inputCount, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDataHandle hPivotMatrix = data.inputValue(aPivotMatrix, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MMatrix pivotMatrix = hPivotMatrix.asMatrix();
    MMatrix pivotMatrixInverse = pivotMatrix.inverse();

    MDataHandle hSymmetryPlane = data.inputValue(aSymmetryPlane, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    short sSymmetryPlane = hSymmetryPlane.asShort();

    MDataHandle hFlippingMode = data.inputValue(aFlippingMode, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    short sFlippMode = hFlippingMode.asShort();

    unsigned int flipAxis = FLIP_AXIS_Z;
    switch(sSymmetryPlane) {
        case SYMMETRY_PLANE_XY:
            flipAxis = FLIP_AXIS_Z;
            break;
        case SYMMETRY_PLANE_YZ:
            flipAxis = FLIP_AXIS_X;
            break;
        case SYMMETRY_PLANE_XZ:
            flipAxis = FLIP_AXIS_Y;
            break;
    }

    unsigned int freeAxis = FLIP_AXIS_X;
    switch(sFlippMode) {
        case POSITION_MIRROR_X:
            freeAxis = FLIP_AXIS_X;
            break;
        case POSITION_MIRROR_Y:
            freeAxis = FLIP_AXIS_Y;
            break;
        case POSITION_MIRROR_Z:
            freeAxis = FLIP_AXIS_Z;
            break;
    }

    MMatrix inputMatrix;
    for (unsigned int listIndex=0; listIndex<inputCount; listIndex++) {
        MDataHandle outHandle = builder.addElement(listIndex);

        hInputMatrices.jumpToElement(listIndex);
        inputMatrix = hInputMatrices.inputValue(&status).asMatrix();

        MMatrix outputMatrix(inputMatrix);
        MMatrix inputXformInPivotSpace;
        inputXformInPivotSpace = inputMatrix * pivotMatrixInverse;

        if (sFlippMode == POSITION_AND_AXES) {
            MMatrix flipMatrix;
            flipMatrix[flipAxis][flipAxis] = -1.0;

            MMatrix flippedJointMatrix = inputXformInPivotSpace * flipMatrix;

            MMatrix axisFlipMatrix;
            axisFlipMatrix[0][0] = -1.0;
            axisFlipMatrix[1][1] = -1.0;
            axisFlipMatrix[2][2] = -1.0;

            flippedJointMatrix = axisFlipMatrix * flippedJointMatrix;
            outputMatrix = flippedJointMatrix * pivotMatrix;
        } else {
            outputMatrix = inputXformInPivotSpace;
            status = flipMatrix(outputMatrix, flipAxis, freeAxis);
            CHECK_MSTATUS_AND_RETURN_IT(status);
            outputMatrix = outputMatrix * pivotMatrix;
        }

        outHandle.setMMatrix(outputMatrix);
    }

    status = hOutputMatrices.set(builder);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = hOutputMatrices.setAllClean();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus mirrorMatrix::flipMirrorOrientationXform(MMatrix& inputMatrix, 
                                   MMatrix& outputMatrix,
                                   unsigned int flipAxis,
                                   unsigned int freeAxis)
{
    MStatus status;
    MMatrix result, rotationAndTranslation, scale;
    MVector scaleVec, shearVec;

    status = flipMatrix(outputMatrix, flipAxis, freeAxis);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus mirrorMatrix::flipMatrix(MMatrix& aMatrix, 
                                   unsigned int aSymmetryPlane,
                                   unsigned int aFreeAxis)
{
    switch (aSymmetryPlane) {
        LOG_TO_CONSOLE("aSymmetryPlane", aSymmetryPlane);
        case SYMMETRY_PLANE_XZ:
             switch(aFreeAxis) {
                 case FLIP_AXIS_X:
                     aMatrix[0][0] *= -1.0;
                     aMatrix[0][1] *= -1.0;
                     aMatrix[1][2] *= -1.0;
                     aMatrix[2][2] *= -1.0;
                     break;
                 case FLIP_AXIS_Y:
                     aMatrix[1][0] *= -1.0;
                     aMatrix[1][1] *= -1.0;
                     aMatrix[0][2] *= -1.0;
                     aMatrix[2][2] *= -1.0;
                     break;
                 case FLIP_AXIS_Z:
                     aMatrix[2][0] *= -1.0;
                     aMatrix[2][1] *= -1.0;
                     aMatrix[0][2] *= -1.0;
                     aMatrix[1][2] *= -1.0;
                     break;
             }

        aMatrix[3][2] *= -1.0;
        break;

        case SYMMETRY_PLANE_XY:
            switch(aFreeAxis) {
                case FLIP_AXIS_X:
                    aMatrix[0][1] *= -1.0;
                    aMatrix[0][2] *= -1.0;
                    aMatrix[1][0] *= -1.0;
                    aMatrix[2][0] *= -1.0;
                    break;
                case FLIP_AXIS_Y:
                    aMatrix[1][1] *= -1.0;
                    aMatrix[1][2] *= -1.0;
                    aMatrix[0][0] *= -1.0;
                    aMatrix[2][0] *= -1.0;
                    break;
                case FLIP_AXIS_Z:
                    aMatrix[2][1] *= -1.0;
                    aMatrix[2][2] *= -1.0;
                    aMatrix[0][0] *= -1.0;
                    aMatrix[1][0] *= -1.0;
                    break;
            }

        aMatrix[3][0] *= -1.0;
        break;

        case SYMMETRY_PLANE_YZ:
             switch(aFreeAxis) {
                 case FLIP_AXIS_X:
                     aMatrix[0][0] *= -1.0;
                     aMatrix[0][2] *= -1.0;
                     aMatrix[1][1] *= -1.0;
                     aMatrix[2][1] *= -1.0;
                     break;
                 case FLIP_AXIS_Y:
                     aMatrix[1][0] *= -1.0;
                     aMatrix[1][2] *= -1.0;
                     aMatrix[0][1] *= -1.0;
                     aMatrix[2][1] *= -1.0;
                     break;
                 case FLIP_AXIS_Z:
                     aMatrix[2][0] *= -1.0;
                     aMatrix[2][2] *= -1.0;
                     aMatrix[0][1] *= -1.0;
                     aMatrix[1][1] *= -1.0;
                     break;
             }

        aMatrix[3][1] *= -1.0;
        break;
    }


    return MS::kSuccess;
}


// standard initialization procedures
MStatus initializePlugin(MObject obj)
{
	MStatus result;
	MFnPlugin plugin(obj, "Sven Pohle", "1.0", "Any");
	result = plugin.registerNode("mirrorMatrix",
				      mirrorMatrix::id,
				      mirrorMatrix::creator,
				      mirrorMatrix::initialize);

	return result;
}

// standard uninitialization procedures
MStatus uninitializePlugin( MObject obj)
{
	MStatus result;
	MFnPlugin plugin( obj );
	result = plugin.deregisterNode(mirrorMatrix::id);
	return result;
}
