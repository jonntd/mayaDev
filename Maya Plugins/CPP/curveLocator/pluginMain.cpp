#include "curveLocatorNode.h"
#include <maya/MFnPlugin.h>

MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, "Sven Pohle", "2016", "Any");

	status = plugin.registerNode( "curveLocator", curveLocator::id, curveLocator::creator,
								  curveLocator::initialize, MPxNode::kLocatorNode,
								  &curveLocator::drawDbClassification);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
		curveLocator::drawDbClassification,
		curveLocator::drawRegistrantId,
		CurveLocatorDrawOverride::Creator);
	if (!status) {
		status.perror("registerDrawOverrideCreator");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
		curveLocator::drawDbClassification,
		curveLocator::drawRegistrantId);
	if (!status) {
		status.perror("deregisterDrawOverrideCreator");
		return status;
	}

	status = plugin.deregisterNode( curveLocator::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
