# Universal Mapper to connect Phoneme Node to Rig
# Activision 2015
# Etienne Danvoye etienne.danvoye@activision.com

import sys
import json
import re
import maya.cmds as cmds
import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx

kPluginCmdName = "edUniversalMapper"

# How-to (Maya Python)
# import maya.cmds as cmds
# mapper = cmds.createNode('edUniMapperNode')
# locatorSrc = cmds.spaceLocator(name='phonemeNode')[0]
# locatorManip = cmds.spaceLocator(name='manipulator')[0]
# locatorRig = cmds.spaceLocator(name='rig')[0]
# cmds.connectAttr('%s.tx' % locatorManip, '%s.tx' % locatorRig)
# cmds.connectAttr('%s.ty' % locatorManip, '%s.ty' % locatorRig)
# cmds.connectAttr('%s.tz' % locatorManip, '%s.tz' % locatorRig)
# cmds.edUniMapperNode_addPhonemeChannel(mapper, '%s.tx' % locatorSrc)
# cmds.edUniMapperNode_addPhonemeChannel(mapper, '%s.ty' % locatorSrc)
# cmds.edUniMapperNode_addPhonemeChannel(mapper, '%s.tz' % locatorSrc)
# cmds.edUniMapperNode_addOutput(mapper, '%s.tx' % locatorManip)
# cmds.edUniMapperNode_addOutput(mapper, '%s.ty' % locatorManip)
# cmds.edUniMapperNode_addOutput(mapper, '%s.tz' % locatorManip)
# cmds.setAttr('%s.tx' % locatorManip, 2.0)
# cmds.setAttr('%s.ty' % locatorManip, 3.0)
# cmds.setAttr('%s.tz' % locatorManip, 0.0)
# cmds.edUniMapperNode_capture(mapper, '%s.tx' % locatorSrc)
# cmds.setAttr('%s.tx' % locatorManip, -2.0)
# cmds.setAttr('%s.ty' % locatorManip, 8.0)
# cmds.setAttr('%s.tz' % locatorManip, 1.0)
# cmds.edUniMapperNode_capture(mapper, '%s.ty' % locatorSrc)
# cmds.edUniMapperNode_saveMapping(mapper, r'c:\temp\test.json')

def MObjectFromName(name):
    list = OpenMaya.MSelectionList()
    list.add(name)
    if not list.isEmpty():
        iter = OpenMaya.MItSelectionList(list)
        obj = OpenMaya.MObject()
        iter.getDependNode(obj)
        return obj
    raise RuntimeError, 'Cannot find MOBject named %s' % name
    
def cleanPlugName(name):
    return name.replace('.','_')

class edUniMapperData():
    def __init__(self, json_data):
        self.mapping = {}
        self.forwardMapping = {}
        self.inputPlugs = []
        self.outputPlugs = []
        self.inputToOutputs = {}
        self.cleanNameMap = {}
        if json_data:
            self.setMappingDefinition( json.loads(json_data) )

    def setMappingDefinition(self, data):
        self.mapping = {}
        self.inputPlugs = []
        self.inputToOutputs = {}
        self.forwardMapping = data['inputs']
        self.outputPlugs = data['outputs']
        self.updateInternal()
        
    def registerNewPlugName(self, name):
       self.cleanNameMap[cleanPlugName(name)] = name
       
    def updateInternal(self):
        # convert forwardMapping to output mapping definition
        for input in self.forwardMapping:

            if input not in self.inputPlugs:
                self.inputPlugs.append(input)

            outputs = self.forwardMapping[input]
            for output,value in outputs:
                if output not in self.mapping:
                    self.mapping[output] = []
                self.mapping[output].append( (input,value) )
    
        for key in self.mapping:
            if key not in self.outputPlugs:
                self.outputPlugs.append(key)
            for name,value in self.mapping[key]:
                if name not in self.inputToOutputs:
                    self.inputToOutputs[name] = [key]
                else:
                    self.inputToOutputs[name].append(key)
                if name not in self.inputPlugs:
                    self.inputPlugs.append(name)
                    
        [self.registerNewPlugName(x) for x in self.inputPlugs]
        [self.registerNewPlugName(x) for x in self.outputPlugs]
                            
    def addInput(self, name):
        if not name in self.forwardMapping:
            self.registerNewPlugName(name)
            self.forwardMapping[name] = []
            self.updateInternal()
            
    def addOutput(self, name):
        if not name in self.outputPlugs:
            self.registerNewPlugName(name)
            self.outputPlugs.append(name)
            
    def setMapping(self, inputName, outputs):
        print 'setMapping() %s' % inputName
        for a,b in outputs:
            print "%s v=%f" % (a,b)
        self.forwardMapping[inputName] = outputs
        
    def plugNameToChannel(self, cleanName):
        if cleanName in self.cleanNameMap:
            return self.cleanNameMap[cleanName]
        return cleanName
                    
    def toJson(self):
        d = {}
        d['inputs'] = self.forwardMapping
        d['outputs'] = self.outputPlugs
        return json.dumps(d)
        
def getMapperDataFromMObject(mobj):
    plug = OpenMaya.MPlug(mobj, edUniMapperNode.mapData)
    if plug:
        jsonData = plug.asString()
        return edUniMapperData(jsonData)
    else:
        raise RuntimeError, 'Cannot find mapData plug'

def getMapperDataFromBlock(block):
    jsonData = block.inputValue(edUniMapperNode.mapData).asString()
    return edUniMapperData(jsonData)
        
def setMapperDataOnMObject(mobj, data):
    plug = OpenMaya.MPlug(mobj, edUniMapperNode.mapData)
    if plug:
        json = data.toJson()
        plug.setString(json)
    else:
        raise RuntimeError, 'Cannot find mapData plug'
    
class edUniMapperNode(OpenMayaMPx.MPxNode):
    kPluginNodeId = OpenMaya.MTypeId(0x001154C0, 56)
    
    mapData = OpenMaya.MObject()
    manipulatorWeight = OpenMaya.MObject()
    mapperWeight = OpenMaya.MObject()
    outputClamp = OpenMaya.MObject()
    
    def __init__(self):
        OpenMayaMPx.MPxNode.__init__(self)
        
    def postConstructor(self):
        self.updateMappingFromData()

    def compute(self, plug, dataBlock):

        #print 'Compute %s' % plug.partialName()
    
        data = self.getDataFromPlug(dataBlock)
        name = data.plugNameToChannel(plug.partialName())
        if name in data.outputPlugs:
            val = self.computeOutputValue(dataBlock, name)
            print val
            outValue = dataBlock.outputValue(plug)
            outValue.setFloat(val)
            dataBlock.setClean(plug)

    def getDataFromPlug(self, block=None):
        if block:
            return getMapperDataFromBlock(block)
        else:
            return getMapperDataFromMObject(self.thisMObject())

    def updateMappingFromData(self):
        data = self.getDataFromPlug()
        self.updateAll(data)
        
    def updateMappingFromString(self, jsonData):
        data = edUniMapperData(jsonData)
        self.updateAll(data)
        
    def updateAll(self, data):
        self.updateInputPlugs(data)
        self.updateOutputPlugs(data)
    
    def updateInputPlugs(self, data):
        # make sure all required input plugs exist
        nodeFn = OpenMaya.MFnDependencyNode(self.thisMObject())
        for name in [cleanPlugName(x) for x in data.inputPlugs]:
            if not nodeFn.hasAttribute(name):
                #print 'DEBUG Create Input Attribute %s' % name
                nAttr = OpenMaya.MFnNumericAttribute()
                ta = nAttr.create(name, name, OpenMaya.MFnNumericData.kFloat, 0.0)
                nAttr.setKeyable(True)
                nodeFn.addAttribute(ta)
    
    def updateOutputPlugs(self, data):
        # make sure all required output plugs exist
        nodeFn = OpenMaya.MFnDependencyNode(self.thisMObject())
        for name in [cleanPlugName(x) for x in data.outputPlugs]:
        
            if not nodeFn.hasAttribute(name):
                #print 'DEBUG Create Output Attribute %s' % name
                nAttr = OpenMaya.MFnNumericAttribute()
                ta = nAttr.create(name, name, OpenMaya.MFnNumericData.kFloat, 0.0)
                nAttr.setStorable(False)
                nAttr.setWritable(False)
                nAttr.setKeyable(False)
                nodeFn.addAttribute(ta)
                
            input_name = 'manip_'+name
            if not nodeFn.hasAttribute(input_name):
                #print 'DEBUG Create Manip Input Attribute %s' % input_name
                nAttr = OpenMaya.MFnNumericAttribute()
                ta = nAttr.create(input_name, input_name, OpenMaya.MFnNumericData.kFloat, 0.0)
                nodeFn.addAttribute(ta)
    
    def getInputValue(self, name, block=None):

        nodeFn = OpenMaya.MFnDependencyNode(self.thisMObject())
        plugName = cleanPlugName(name)
        
        if nodeFn.hasAttribute(plugName):
            attr = nodeFn.attribute(plugName)
            if attr:
                if block:
                    return block.inputValue(attr).asFloat()
                else:
                    plug = nodeFn.findPlug(attr, False)
                    if plug:
                        return plug.asFloat()

        return 0.0

    def setInternalValueInContext(self, plug, dataHandle, ctx):
        #print 'setInternalValueInContext %s' % plug.partialName() # DEBUG
        
        if plug.attribute()==edUniMapperNode.mapData:
            self.updateMappingFromString(dataHandle.asString())
        return OpenMayaMPx.MPxNode.setInternalValueInContext( self, plug, dataHandle, ctx )
        
    def computeOutputValue(self, block, name):
        data = self.getDataFromPlug(block)

        clampValues = block.inputValue(edUniMapperNode.outputClamp).asFloat2()
        
        if name in data.mapping:        
            # get value or relevant input plugs
            weightMapper = block.inputValue(edUniMapperNode.mapperWeight).asFloat()
            weightManip = block.inputValue(edUniMapperNode.manipulatorWeight).asFloat()
        
            ops_list = data.mapping[name]
            val = 0.0
            for input,value in ops_list:
                val += self.getInputValue(input, block) * value * weightMapper
            val += self.getInputValue('manip_'+cleanPlugName(name), block) * weightManip
            
            val = max(min(val,clampValues[1]),clampValues[0])
                
            return val
        elif name in data.outputPlugs:
            # no mapping yet, pass directly the manipulator
            val = self.getInputValue('manip_'+cleanPlugName(name), block)
            val = max(min(val,clampValues[1]),clampValues[0])
            return val
        else:
            return 0.0
        
    def setDependentsDirty(self, plugBeingDirtied, affectedPlugs):
        #print 'DEBUG: setDependentsDirty %s' % plugBeingDirtied.partialName() # DEBUG
        
        nodeFn = OpenMaya.MFnDependencyNode(self.thisMObject())
        data = self.getDataFromPlug()
        plug_name = plugBeingDirtied.partialName()
        
        # In dirtied plug is one of the inputs, dirty related outputs
        name = data.plugNameToChannel(plug_name)
        if name in data.inputToOutputs:            
            output_list = data.inputToOutputs[name]
            for out in [cleanPlugName(x) for x in output_list]:
                if nodeFn.hasAttribute(out):
                    attr = nodeFn.attribute(out)
                    if attr:
                        plug = OpenMaya.MPlug(self.thisMObject(), attr)
                        if plug:             
                            #print 'affects %s' % out
                            affectedPlugs.append(plug)

        # Manipulator inputs affect the corresponding output
        m = re.match('manip_(.*)',plug_name)
        if m:
            out = m.group(1)
            if nodeFn.hasAttribute(out):
                attr = nodeFn.attribute(out)
                if attr:
                    plug = OpenMaya.MPlug(self.thisMObject(), attr)
                    if plug:             
                        #print 'affects %s' % out
                        affectedPlugs.append(plug)
                            
        # if plug being dirtied is the mapping file path, dirty all outputs
        willDirtyOutput = [edUniMapperNode.mapData, edUniMapperNode.mapperWeight, edUniMapperNode.manipulatorWeight, edUniMapperNode.outputClamp]
        if plugBeingDirtied.attribute() in willDirtyOutput:
            for out in [cleanPlugName(x) for x in data.outputPlugs]:
                if nodeFn.hasAttribute(out):
                    attr = nodeFn.attribute(out)
                    if attr:
                        plug = OpenMaya.MPlug(self.thisMObject(), attr)
                        if plug:            
                            #print 'affects %s' % out
                            affectedPlugs.append(plug)

    @staticmethod                          
    def creator():
        return OpenMayaMPx.asMPxPtr(edUniMapperNode())     

    @staticmethod
    def initialize():
        # String attribute for internal data, as json
        tAttr = OpenMaya.MFnTypedAttribute()
        edUniMapperNode.mapData = tAttr.create("mapData", "md", OpenMaya.MFnData.kString)
        tAttr.setHidden(True)
        tAttr.setInternal(True)
        edUniMapperNode.addAttribute(edUniMapperNode.mapData)    

        # manipulatorWeight
        nAttr = OpenMaya.MFnNumericAttribute()
        edUniMapperNode.manipulatorWeight = nAttr.create("manipulatorWeight", "mw", OpenMaya.MFnNumericData.kFloat, 1.0)
        nAttr.setKeyable(True)
        edUniMapperNode.addAttribute(edUniMapperNode.manipulatorWeight)    
        
        # mapperWeight
        nAttr = OpenMaya.MFnNumericAttribute()
        edUniMapperNode.mapperWeight = nAttr.create("mapperWeight", "pw", OpenMaya.MFnNumericData.kFloat, 1.0)
        nAttr.setKeyable(True)
        edUniMapperNode.addAttribute(edUniMapperNode.mapperWeight)    
        
        # outputClamp
        nAttr = OpenMaya.MFnNumericAttribute()
        edUniMapperNode.outputClamp = nAttr.create("outputClamp", "oc", OpenMaya.MFnNumericData.k2Float)
        nAttr.setDefault(-1.0, 1.0)
        nAttr.setKeyable(True)
        edUniMapperNode.addAttribute(edUniMapperNode.outputClamp)    

    manipulatorWeight = OpenMaya.MObject()
    mapperWeight = OpenMaya.MObject()
    outputClamp = OpenMaya.MObject()
        
        
class edUniMapperNode_addPhonemeChannel(OpenMayaMPx.MPxCommand):
    def __init__(self):
        OpenMayaMPx.MPxCommand.__init__(self)
    def doIt(self, args):
        nodeName = args.asString(0)
        newInputName = args.asString(1)
        obj = MObjectFromName(nodeName)
        data = getMapperDataFromMObject(obj)
        data.addInput(newInputName)
        setMapperDataOnMObject(obj, data)
        cmds.connectAttr( newInputName, '%s.%s' % (nodeName, cleanPlugName(newInputName)) )
    @staticmethod                          
    def creator():
        return edUniMapperNode_addPhonemeChannel()    
        
class edUniMapperNode_addManipulatorChannel(OpenMayaMPx.MPxCommand):
    def __init__(self):
        OpenMayaMPx.MPxCommand.__init__(self)
    def doIt(self, args):
        nodeName = args.asString(0)
        manipOutputName = args.asString(1)
        obj = MObjectFromName(nodeName)
        data = getMapperDataFromMObject(obj)
        data.addOutput(manipOutputName)
        setMapperDataOnMObject(obj, data)
        
        # list current outgoing connections 
        outgoingConnections = cmds.listConnections( manipOutputName, d=True, s=False, p=True )
        if outgoingConnections:
            for conn in outgoingConnections:
                cmds.disconnectAttr(manipOutputName, conn)
                cmds.connectAttr('%s.%s' % (nodeName,cleanPlugName(manipOutputName)), conn)
        cmds.connectAttr(manipOutputName, '%s.manip_%s' % (nodeName,cleanPlugName(manipOutputName)))
        
    @staticmethod                          
    def creator():
        return edUniMapperNode_addManipulatorChannel()    

class edUniMapperNode_saveMapping(OpenMayaMPx.MPxCommand):
    def __init__(self):
        OpenMayaMPx.MPxCommand.__init__(self)
    def doIt(self, args):
        nodeName = args.asString(0)
        filename = args.asString(1)
        obj = MObjectFromName(nodeName)
        data = getMapperDataFromMObject(obj)
        with open(filename, 'w') as f:
            f.write(data.toJson())
    @staticmethod                          
    def creator():
        return edUniMapperNode_saveMapping()    

class edUniMapperNode_loadMapping(OpenMayaMPx.MPxCommand):
    def __init__(self):
        OpenMayaMPx.MPxCommand.__init__(self)
    def doIt(self, args):
        nodeName = args.asString(0)
        filename = args.asString(1)
        obj = MObjectFromName(nodeName)
        with open(filename, 'r') as f:
            j = f.read()
            data = edUniMapperData(j)
            setMapperDataOnMObject(obj, data)
    @staticmethod                          
    def creator():
        return edUniMapperNode_saveMapping()    

class edUniMapperNode_capture(OpenMayaMPx.MPxCommand):
    def __init__(self):
        OpenMayaMPx.MPxCommand.__init__(self)
    def doIt(self, args):
        nodeName = args.asString(0)
        inputName = args.asString(1)
        obj = MObjectFromName(nodeName)

        nodeFn = OpenMaya.MFnDependencyNode(obj)
        plugname = inputName.replace(".","_")  

        if not nodeFn.hasAttribute(plugname):
            print "NOT VALID - %s " % plugname
            return
        else:
            print "SUCCESS: %s is captured!!" % plugname


        data = getMapperDataFromMObject(obj)
        outputs = []
        for output in data.outputPlugs:
            value = cmds.getAttr(output) # read directly the manipulator value
            outputs.append( (output,value) )
        data.setMapping(inputName, outputs)
        setMapperDataOnMObject(obj, data)
    @staticmethod                          
    def creator():
        return edUniMapperNode_capture()    
        
def initializePlugin(mobject):
    plugin = OpenMayaMPx.MFnPlugin(mobject)
    plugin.registerNode('edUniMapperNode', edUniMapperNode.kPluginNodeId, edUniMapperNode.creator, edUniMapperNode.initialize)
    plugin.registerCommand("edUniMapperNode_addPhonemeChannel", edUniMapperNode_addPhonemeChannel.creator)
    plugin.registerCommand("edUniMapperNode_addManipulatorChannel", edUniMapperNode_addManipulatorChannel.creator)
    plugin.registerCommand("edUniMapperNode_saveMapping", edUniMapperNode_saveMapping.creator)
    plugin.registerCommand("edUniMapperNode_loadMapping", edUniMapperNode_loadMapping.creator)
    plugin.registerCommand("edUniMapperNode_capture", edUniMapperNode_capture.creator)
        
def uninitializePlugin(mobject):
    plugin = OpenMayaMPx.MFnPlugin(mobject)
    plugin.deregisterNode(edUniMapperNode.kPluginNodeId)
    plugin.deregisterCommand("edUniMapperNode_addPhonemeChannel")
    plugin.deregisterCommand("edUniMapperNode_addManipulatorChannel")
    plugin.deregisterCommand("edUniMapperNode_saveMapping")
    plugin.deregisterCommand("edUniMapperNode_loadMapping")
    plugin.deregisterCommand("edUniMapperNode_capture")
    
        