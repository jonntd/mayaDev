import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx
import maya.OpenMayaRender as OpenMayaRender
import maya.OpenMayaUI as OpenMayaUI

import math
import sys

kPluginLocatorTypeName = "spRotateLocator"
kPluginLocatorManipTypeName = "spRotateLocatorManip"

spRotateLocatorId = OpenMaya.MTypeId(0x8700E)
spRotateLocatorManipId = OpenMaya.MTypeId(0x8700F)

glRenderer = OpenMayaRender.MHardwareRenderer.theRenderer()
glFT = glRenderer.glFunctionTable()

class spRotateLocator(OpenMayaMPx.MPxLocatorNode):
    aRotation    = OpenMaya.MObject()
    aSensitivity = OpenMaya.MObject()
    aOutRotation = OpenMaya.MObject()
    
    def __init__(self):
        OpenMayaMPx.MPxLocatorNode.__init__(self)
        
    def compute(self, plug, dataBlock):
        return OpenMaya.kUnknownParameter

    def draw(self, view, path, style, status):
        thisNode = self.thisMObject()
        plug = OpenMaya.MPlug(thisNode, self.size)
       
    def isBounded(self):
        return True

    def boundingBox(self):
        thisNode = self.thisMObject()
        
        bbox = OpenMaya.MBoundingBox()
        return bbox

def locatorCreator():
    return OpenMayaMPx.asMPxPtr(spRotateLocator())

def locatorInitializer():
    nAttr  = OpenMaya.MFnNumericAttribute()

    spRotateLocator.aRotation = nAttr.create("rotation", "rotation", OpenMaya.MFnNumericData.k3Double)
    nAttr.setStorable(True)
    nAttr.setWritable(True)

    spRotateLocator.aSensitivity = nAttr.create("sensitivity", "sensitivity", OpenMaya.MFnNumericData.kDouble)
    nAttr.setMin(0.0)
    nAttr.setStorable(True)
    nAttr.setWritable(True)

    spRotateLocator.aOutRotation = nAttr.create("outRotation", "outRotation", OpenMaya.MFnNumericData.k3Double)
    nAttr.setStorable(False)
    nAttr.setWritable(False)

    spRotateLocator.addAttribute(spRotateLocator.aRotation)
    spRotateLocator.addAttribute(spRotateLocator.aSensitivity)
    spRotateLocator.addAttribute(spRotateLocator.aOutRotation)

    spRotateLocator.attributeAffects(spRotateLocator.aRotation, spRotateLocator.aOutRotation)
    spRotateLocator.attributeAffects(spRotateLocator.aSensitivity, spRotateLocator.aOutRotation)

    OpenMayaMPx.MPxManipContainer.addToManipConnectTable(spRotateLocatorId)

# initialize the script plug-in
def initializePlugin(mobject):
    mplugin = OpenMayaMPx.MFnPlugin(mobject)

    try:
        mplugin.registerNode( kPluginLocatorTypeName, spRotateLocatorId, locatorCreator, locatorInitializer, OpenMayaMPx.MPxNode.kLocatorNode )
    except:
        sys.stderr.write("Failed to register locator node: %s" % kPluginLocatorTypeName)
        raise


# uninitialize the script plug-in
def uninitializePlugin(mobject):
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    try:
        mplugin.deregisterNode(spRotateLocatorId)
    except:
        sys.stderr.write( "Failed to deregister locator node: %s" % kPluginLocatorTypeName )
        raise
