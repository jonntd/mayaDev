#include "curvePointsNode.h"

#include <maya/MFnPlugin.h>

MStatus initializePlugin( MObject obj )

{ 
	MStatus   status;
	MFnPlugin plugin( obj, "Sven Pohle", "2016", "Any");

	status = plugin.registerNode( "curvePoints", curvePoints::id, curvePoints::creator,
								  curvePoints::initialize, MPxNode::kLocatorNode );
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

	status = plugin.deregisterNode( curvePoints::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
