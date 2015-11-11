import math, sys

import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx

kPluginNodeTypeName = 'spSineNode'
sineNodeId          = OpenMaya.MTypeId(0x8700)


# Node Definition
class sineNode(OpenMayaMPx.MPxNode):
    # class variables
    aInput  = OpenMaya.MObject()
    aOutput = OpenMaya.MObject()

    def __init__(self):
        OpenMayaMPx.MPxNode.__init__(self)

    def compute(self, plug, data):
        if plug == sineNode.aOutput:
            inputValue = data.inputValue(sineNode.aInput).asFloat()
            result = math.sin(inputValue) * 10.0
            hOutput = data.outputValue(sineNode.aOutput)
            hOutput.setFloat(result)
            data.setClean(plug)

# Node Creator
def nodeCreator():
    return OpenMayaMPx.asMPxPtr(sineNode())

# Node Initializer
def nodeInitializer():
    # attr types
    nAttr = OpenMaya.MFnNumericAttribute()

    sineNode.aInput = nAttr.create("input", "input", OpenMaya.MFnNumericData.kFloat, 0.0)
    nAttr.setStorable(True)

    sineNode.aOutput = nAttr.create("output", "output", OpenMaya.MFnNumericData.kFloat, 0.0)
    nAttr.setStorable(False)
    nAttr.setWritable(False)

    sineNode.addAttribute(sineNode.aInput)
    sineNode.addAttribute(sineNode.aOutput)
    sineNode.attributeAffects(sineNode.aInput, sineNode.aOutput)

def initializePlugin(mObject):
    mPlugin = OpenMayaMPx.MFnPlugin(mObject)
    try:
        mPlugin.registerNode(kPluginNodeTypeName, sineNodeId, nodeCreator, nodeInitializer)
    except:
        sys.stderr.write("Failed to register node: %s" % kPluginNodeTypeName)
        raise

def uninitializePlugin(mObject):
    mPlugin = OpenMayaMPx.MFnPlugin(mObject)
    try:
        mPlugin.deregisterNode(sineNodeId)
    except:
        sys.stderr.write("Failed to deregister node: %s" % kPluginNodeTypeName)
        raise







