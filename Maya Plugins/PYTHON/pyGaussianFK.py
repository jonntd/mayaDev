import sys
import math
import maya.api.OpenMaya as om
import maya.api.OpenMayaUI as omui
import maya.api.OpenMayaRender as omr

def maya_useNewAPI():
    """
    The presence of this function tells Maya that the plugin produces, and
    expects to be passed, objects created using the Maya Python API 2.0.
    """
    pass

kPluginNodeName = 'spGaussianFK'
kPluginNodeId   = om.MTypeId(0x8700C)

class gaussianFk(om.MPxNode):
    aInputMatrices  = om.MObject()
    aRotation       = om.MObject()
    aLocation       = om.MObject()
    aSharpness      = om.MObject()
    aAmplitude      = om.MObject()
    aOutputMatrices = om.MObject()

    def __init__(self):
        om.MPxNode.__init__(self)

    def compute(self, plug, data):
        if plug != gaussianFk.aOutputMatrices: 
            return

        inputMatrices = list()

        rotation = data.inputValue(gaussianFk.aRotation).asFloat3()
        location = data.inputValue(gaussianFk.aLocation).asFloat()
        sharpness = data.inputValue(gaussianFk.aSharpness).asFloat()
        amplitude = data.inputValue(gaussianFk.aAmplitude).asFloat()

        hInputMatrices = data.inputArrayValue(gaussianFk.aInputMatrices)
        inputCount = 0

        while not hInputMatrices.isDone():
            hInputMatrices.jumpToPhysicalElement(inputCount)
            inputMatrices.append(hInputMatrices.inputValue().asMatrix())
            inputCount += 1
            hInputMatrices.next()


        gaussianValues = list()
        rotationGaussianValues = list()

        sqrt2Pi = math.sqrt(2.0 * math.pi)
        paramStep = 1.0 / (inputCount - 1.0)
        gaussianSum = 0.0
        mu = (inputCount - 1.0) * location

        standardDeviation = (inputCount / 4.2) * (1.0 - sharpness) + 0.01
        maxGaussian = 1.0 / (sqrt2Pi * standardDeviation)
        parametricFactor = 1.0 / maxGaussian
        stnrdDeviatSquareReciprocal = 1.0/(2.0*standardDeviation*standardDeviation)
        sqrtStnrdDeviatReciprocal = 1.0/(sqrt2Pi*standardDeviation)

        # calculate and fill the gaussian values
        for i in range(inputCount):
            gaussianLocal = math.exp(-(((i-mu)*(i-mu)) * stnrdDeviatSquareReciprocal)) * sqrtStnrdDeviatReciprocal
            parametricGaussian = gaussianLocal * parametricFactor
            gaussianValues.append(parametricGaussian)

        gaussianSum = 1.0;
        gaussianSumRecip = 1.0/gaussianSum

        rotationResult = [0, 0, 0]
        for j in range(inputCount):
            gaussianValues[j] = gaussianValues[j] * gaussianSumRecip
            for i in range(3):
                rotationResult[i] = rotation[i] * gaussian[j] * amplitude
            
            rotationGaussianValues.append(rotationResult)


        # set the output matrices
        hOutputMatrices = data.outputArrayValue(aOutputMatrices)
        outputBuilder = hOutputMatrices.builder()

        radRotation = [0,0,0]
        for index in range(inputCount):
            radRotation[0] = math.radians(rotationGaussianValues[index][0])
            radRotation[1] = math.radians(rotationGaussianValues[index][1])
            radRotation[2] = math.radians(rotationGaussianValues[index][2])

            transformMatrix = om.MTransformationMatrix(inputMatrices[index])
            transformMatrix.addRotation(radRotation)

            outHandle = outputBuilder.addElement(index)
            outHandle.setMMatrix(inputMatrices[index])

        hOutputMatrices.set(outputBuilder)
        hOutputMatrices.setAllClean()

        return om.kSuccess


def nodeCreator():
    return gaussianFk()

def nodeInitializer():
    mAttr = om.MFnMatrixAttribute()
    nAttr = om.MFnNumericAttribute()

    gaussianFk.aInputMatrices = mAttr.create("inputMatrices", "inputMatrices")
    om.MPxNode.addAttribute(gaussianFk.aInputMatrices)
    mAttr.keyable = False
    mAttr.hidden  = True
    mAttr.connectable = True
    mAttr.array = True
    mAttr.usesArrayDataBuilder = True

    gaussianFk.aRotation = nAttr.create("rotation", "rotation", om.MFnNumericData.k3Double, 0.0)
    om.MPxNode.addAttribute(gaussianFk.aRotation)

    gaussianFk.aLocation = nAttr.create("location", "location", om.MFnNumericData.kFloat, 0.5)
    om.MPxNode.addAttribute(gaussianFk.aLocation)
    nAttr.setMin(0.0)
    nAttr.setMax(1.0)

    gaussianFk.aSharpness = nAttr.create("sharpness", "sharpness", om.MFnNumericData.kFloat, 0.5)
    om.MPxNode.addAttribute(gaussianFk.aSharpness)
    nAttr.setMin(0.0)
    nAttr.setMax(1.0)

    gaussianFk.aAmplitude = nAttr.create("amplitude", "amplitude", om.MFnNumericData.kFloat, 1.0)
    om.MPxNode.addAttribute(gaussianFk.aAmplitude)
    nAttr.setMin(0.0)
    nAttr.setMax(1.0)

    gaussianFk.aOutputMatrices = mAttr.create("outputMatrices", "outputMatrices")
    om.MPxNode.addAttribute(gaussianFk.aOutputMatrices)
    mAttr.keyable = False
    mAttr.storable = False
    mAttr.hidden  = True
    mAttr.array = True
    mAttr.usesArrayDataBuilder = True

    ## ---------- Specify what inputs affect the outputs ----------
    ##
    om.MPxNode.attributeAffects(gaussianFk.aInputMatrices, gaussianFk.aOutputMatrices)
    om.MPxNode.attributeAffects(gaussianFk.aRotation, gaussianFk.aOutputMatrices)
    om.MPxNode.attributeAffects(gaussianFk.aLocation, gaussianFk.aOutputMatrices)
    om.MPxNode.attributeAffects(gaussianFk.aSharpness, gaussianFk.aOutputMatrices)
    om.MPxNode.attributeAffects(gaussianFk.aAmplitude, gaussianFk.aOutputMatrices)

def initializePlugin(mobject):
    ''' Initialize the plug-in '''
    mplugin = om.MFnPlugin(mobject)
    try:
        mplugin.registerNode( kPluginNodeName, kPluginNodeId, nodeCreator, nodeInitializer)
    except:
        sys.stderr.write( "Failed to register node: " + kPluginNodeName )
        raise

def uninitializePlugin(mobject):
    ''' Uninitializes the plug-in '''
    mplugin = om.MFnPlugin(mobject)
    try:
        mplugin.deregisterNode( kPluginNodeId )
    except:
        sys.stderr.write( "Failed to deregister node: " + kPluginNodeName )
        raise