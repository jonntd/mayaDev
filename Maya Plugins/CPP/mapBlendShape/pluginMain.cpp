#include "mapBlendShapeNode.h"
#include <maya/MFnPlugin.h>

MStatus initializePlugin(MObject obj)
{ 
	MStatus   status;
	MFnPlugin plugin(obj, "sven pohle", "2016", "Any");

	status = plugin.registerNode("mapBlendShape", 
								 mapBlendShape::id, 
								 mapBlendShape::creator,
								 mapBlendShape::initialize,
								 MPxNode::kDeformerNode);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( mapBlendShape::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
