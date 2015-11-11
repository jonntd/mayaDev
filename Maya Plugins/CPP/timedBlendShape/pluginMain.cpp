#include "timedBlendShapeNode.h"

#include <maya/MFnPlugin.h>

MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, "Sven Pohle", "2016", "Any");

	status = plugin.registerNode( "timedBlendShape", timedBlendShape::id, timedBlendShape::creator,
								  timedBlendShape::initialize, MPxNode::kDeformerNode );
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

	status = plugin.deregisterNode( timedBlendShape::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
