#include "GaussianFKNode.h"

#define LOG_TO_CONSOLE(debugText, object)                   \
{                                                           \
    std::stringstream stream;                               \
    stream << object;                                       \
    MGlobal::displayInfo(MString(debugText) + " : " + MString(stream.str().c_str()));    \
}

MTypeId     GaussianFK::id( 0x81036 );

MObject GaussianFK::aInputMatrices;  
MObject GaussianFK::aParentInverseMatrices;
MObject	GaussianFK::aControllers;

MObject	GaussianFK::aRotationX;
MObject	GaussianFK::aRotationY;
MObject	GaussianFK::aRotationZ;
MObject	GaussianFK::aRotation;

MObject	GaussianFK::aScale;
MObject	GaussianFK::aLocation;
MObject	GaussianFK::aSharpness;
MObject	GaussianFK::aAmplitude;

MObject GaussianFK::aOutputMatrices;       
MObject GaussianFK::aOutputControllerMatrices;

GaussianFK::GaussianFK() {}
GaussianFK::~GaussianFK() {}

MStatus GaussianFK::compute( const MPlug& plug, MDataBlock& data )
{
	MStatus status;
	char buffer[256];
	
	MArrayDataHandle hInputMatrices = data.inputArrayValue(aInputMatrices, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	unsigned int inputCount = hInputMatrices.elementCount();

	MArrayDataHandle hParentInverseMatrices = data.inputArrayValue(aParentInverseMatrices, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	unsigned int parentCount = hParentInverseMatrices.elementCount();

	MArrayDataHandle hOutputMatrices = data.outputArrayValue(aOutputMatrices, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MArrayDataBuilder builder(aOutputMatrices, inputCount, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MArrayDataHandle hControllers = data.inputArrayValue(aControllers);
	unsigned int numControllers = hControllers.elementCount();

	std::vector<std::vector<double>> rotationGaussianValues;
	rotationGaussianValues.resize(inputCount);
	for (unsigned int i=0; i<inputCount; i++) {
		rotationGaussianValues[i].resize(3);
	}

	for (unsigned int c=0; c<numControllers; ++c) {
		std::vector<std::vector<double>> rotationGaussianValuesLocal;
		rotationGaussianValuesLocal.resize(inputCount);
		for (unsigned int i=0; i<inputCount; i++) {
			rotationGaussianValuesLocal[i].resize(3);
		}

		hControllers.jumpToArrayElement(c);
		MDataHandle curControllerHandle = hControllers.inputValue();
		double3 &rotation = curControllerHandle.child(aRotation).asDouble3();
		MVector scale = curControllerHandle.child(aScale).asVector();

		double location  = curControllerHandle.child(aLocation).asDouble();
		double sharpness = curControllerHandle.child(aSharpness).asDouble();
		double amplitude = curControllerHandle.child(aAmplitude).asDouble();

		std::vector<double> gaussian;
		const double sqrt2Pi = sqrt(2.0 * M_PI);
		const double paramStep = 1.0 / (inputCount - 1.0);
		double gaussianSum = 0.0;
		const double mu = (inputCount - 1.0) * location;
		const double maxDev = 8.0; // @ 0 sharpness
		const double minDev = 0.2; // @ 1 sharpness
		const double standardDeviation = maxDev - ((maxDev - minDev)/100.0)*sharpness*100;
		const double maxGaussian = 1.0 / (sqrt2Pi * standardDeviation);
		const double parametricFactor = 1.0 / maxGaussian;
		const double stnrdDeviatSquareReciprocal = 1.0/(2.0*standardDeviation*standardDeviation);
		const double sqrtStnrdDeviatReciprocal = 1.0/(sqrt2Pi*standardDeviation);

			
		std::vector<double> fullRotation;
		fullRotation.resize(3);
		double parametricGaussian;
		// calculate and fill the gaussian values
		for (unsigned int j=0; j<inputCount; j++) {
			// e^(-(x-μ)^2/(2 σ^2))/(sqrt(2 π) σ)
			const double gaussianLocal = exp(-(((j-mu)*(j-mu)) * stnrdDeviatSquareReciprocal)) * sqrtStnrdDeviatReciprocal;
			parametricGaussian = gaussianLocal * parametricFactor;
			gaussian.push_back(parametricGaussian);

			std::vector<double> rotationResult;
			for (unsigned int i=0; i<3; i++) {
				double rotCalc = rotation[i] * parametricGaussian * amplitude;
				rotationGaussianValuesLocal[j][i] = rotCalc;
				fullRotation[i] += rotCalc;
			}
		}

		// normalize the rotation values
		for (unsigned int i=0; i<3; i++) {
			if (rotation[i]==0.0) {
				fullRotation[i] = 0.0;
			} else {
				fullRotation[i] = rotation[i]/fullRotation[i];	
			}
		}

		for (unsigned int j=0; j<inputCount; j++) {
			for (unsigned int i=0; i<3; i++) {
				rotationGaussianValuesLocal[j][i] *= fullRotation[i];
				rotationGaussianValues[j][i] += rotationGaussianValuesLocal[j][i];
			}
		}
	}


	MMatrix inputMatrix;
	double finalRotation[3];
	for (unsigned int listIndex=0; listIndex<inputCount; listIndex++) {
		MDataHandle outHandle = builder.addElement(listIndex);

		hInputMatrices.jumpToElement(listIndex);
		inputMatrix = hInputMatrices.inputValue(&status).asMatrix();

		// convert to radians
		finalRotation[0] = rotationGaussianValues[listIndex][0] * M_PI/180;
		finalRotation[1] = rotationGaussianValues[listIndex][1] * M_PI/180;
		finalRotation[2] = rotationGaussianValues[listIndex][2] * M_PI/180;

		MTransformationMatrix transform(inputMatrix);
		transform.addRotation(finalRotation, MTransformationMatrix::kXYZ, MSpace::kTransform);

		outHandle.setMMatrix(transform.asMatrix());
	}

	status = hOutputMatrices.set(builder);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = hOutputMatrices.setAllClean();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// set the output controller matrices
	MArrayDataBuilder controllersBuilder(aOutputControllerMatrices, numControllers, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	std::vector<float> matrixLocations;
	matrixLocations.resize(inputCount);
	for (unsigned int listIndex=0; listIndex<inputCount; listIndex++) {
		matrixLocations[listIndex] = float(listIndex/inputCount);
	}

	// build a curve through the points
	MObject curveDummy;
	MFnNurbsCurve fnCurve;
	MPointArray curvePoints;
	for (unsigned int listIndex=0; listIndex<inputCount; listIndex++) {
		hOutputMatrices.jumpToElement(listIndex);
		MMatrix outMatrix = hOutputMatrices.outputValue().asMatrix();
		curvePoints.append(MPoint(outMatrix[3][0], outMatrix[3][1], outMatrix[3][2]));
	}

	MDoubleArray curveKnots;
    curveKnots.insert(0.0,0);
    for (int j = 0; j < (int)(curvePoints.length()); j++) {
		curveKnots.append((double)j);
    }
    curveKnots.append(curvePoints.length()-1);

	fnCurve.create(curvePoints,curveKnots,3,MFnNurbsCurve::kOpen,false,false,curveDummy,&status);
	double curveLength = fnCurve.length();
	
	for (unsigned int c=0; c<numControllers; c++) {
		hControllers.jumpToArrayElement(c);
		MDataHandle curControllerHandle = hControllers.inputValue();
		double location  = curControllerHandle.child(aLocation).asDouble();

		double param = curveLength*location;
		MPoint curvePoint;
		MVector tangent, normal;
		fnCurve.getPointAtParam(param, curvePoint);
		normal = fnCurve.normal(param);
		tangent = fnCurve.tangent(param);

		// build the matrix
		MMatrix outMatrix;
		outMatrix.setToIdentity();
		outMatrix[3][0] = curvePoint[0];
		outMatrix[3][1] = curvePoint[1];
		outMatrix[3][2] = curvePoint[2];

		MDataHandle outHandle = controllersBuilder.addElement(c);
		outHandle.setMMatrix(outMatrix);
	}

	return MS::kSuccess;
}

void* GaussianFK::creator()
{
	return new GaussianFK();
}

MStatus GaussianFK::initialize()	
{
	MStatus status;
	MFnMatrixAttribute mAttr;
	MFnNumericAttribute nAttr;
	MFnCompoundAttribute cAttr;

	aInputMatrices = mAttr.create("inputMatrices", "inputMatrices");
	mAttr.setConnectable(true);
    mAttr.setKeyable(true);  
    mAttr.setStorable(true);
    mAttr.setReadable(true); 
    mAttr.setWritable(true);
    mAttr.setHidden(false);
    mAttr.setArray(true);
    mAttr.setUsesArrayDataBuilder(true);
    addAttribute(aInputMatrices);

	aParentInverseMatrices = mAttr.create("parentInverseMatrices", "parentInverseMatrices");
	mAttr.setConnectable(true);
    mAttr.setKeyable(true);  
    mAttr.setStorable(true);
    mAttr.setReadable(true); 
    mAttr.setWritable(true);
    mAttr.setHidden(false);
    mAttr.setArray(true);
    mAttr.setUsesArrayDataBuilder(true);
    addAttribute(aParentInverseMatrices);

	aRotation = nAttr.create("rotation", "rotation", MFnNumericData::k3Double);
	nAttr.setDefault(0.0, 0.0, 0.0);
    nAttr.setStorable(true);
	nAttr.setKeyable(true);
	//addAttribute(aRotation);

	aScale = nAttr.create("scale", "scale", MFnNumericData::k3Double);
	nAttr.setDefault(1.0, 1.0, 1.0);
    nAttr.setStorable(true);
	nAttr.setKeyable(true);
	//addAttribute(aScale);

	aLocation = nAttr.create("location", "location", MFnNumericData::kDouble);
	nAttr.setDefault(0.5);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	nAttr.setKeyable(true);
	//addAttribute(aLocation);
	
	aSharpness = nAttr.create("sharpness", "sharpness", MFnNumericData::kDouble);
	nAttr.setDefault(0.5);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	nAttr.setKeyable(true);
	//addAttribute(aSharpness);

	aAmplitude = nAttr.create("amplitude", "amplitude", MFnNumericData::kDouble);
	nAttr.setDefault(1.0);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	nAttr.setKeyable(true);
	//addAttribute(aAmplitude);

	aControllers = cAttr.create("controllers", "controllers");
	cAttr.addChild(aRotation);
	cAttr.addChild(aScale);
	cAttr.addChild(aLocation);
	cAttr.addChild(aSharpness);
	cAttr.addChild(aAmplitude);
	cAttr.setArray(true);
	addAttribute(aControllers);

	aOutputMatrices = mAttr.create("outputMatrices", "outputMatrices");
    mAttr.setConnectable(true);
    mAttr.setKeyable(false);  
    mAttr.setStorable(false);
    mAttr.setWritable(false);
    mAttr.setHidden(true);
    mAttr.setArray(true);
    mAttr.setUsesArrayDataBuilder(true);
    addAttribute(aOutputMatrices);

	aOutputControllerMatrices = mAttr.create("outputControllerMatrices", "outputControllerMatrices");
    mAttr.setConnectable(true);
    mAttr.setKeyable(false);  
    mAttr.setStorable(false);
    mAttr.setWritable(false);
    mAttr.setHidden(true);
    mAttr.setArray(true);
    mAttr.setUsesArrayDataBuilder(true);
    addAttribute(aOutputControllerMatrices);

	attributeAffects(aInputMatrices, aOutputMatrices);
	attributeAffects(aControllers, aOutputMatrices);
	attributeAffects(aRotation, aOutputMatrices);
	attributeAffects(aLocation, aOutputMatrices);
	attributeAffects(aSharpness, aOutputMatrices);
	attributeAffects(aAmplitude, aOutputMatrices);

	attributeAffects(aInputMatrices, aOutputControllerMatrices);
	attributeAffects(aControllers, aOutputControllerMatrices);
	attributeAffects(aRotation, aOutputControllerMatrices);
	attributeAffects(aLocation, aOutputControllerMatrices);
	attributeAffects(aSharpness, aOutputControllerMatrices);
	attributeAffects(aAmplitude, aOutputControllerMatrices);

	return MS::kSuccess;
}

