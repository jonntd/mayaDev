#include "sphericalBlendShapeNode.h"
#include "sphericalBlendShapeVisualizeNode.h"

#include <maya/MFnPlugin.h>

MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, "Sven Pohle", "2016", "Any");

	status = plugin.registerNode( "sphericalBlendShape", sphericalBlendShape::id, sphericalBlendShape::creator,
								  sphericalBlendShape::initialize, MPxNode::kDeformerNode );
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	status = plugin.registerNode("sphericalBlendShapeVisualizer", 
								 sphericalBlendShapeVisualizer::id, 
								 sphericalBlendShapeVisualizer::creator,
								 sphericalBlendShapeVisualizer::initialize,
								 MPxNode::kLocatorNode,
								 &sphericalBlendShapeVisualizer::drawDbClassification);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
		sphericalBlendShapeVisualizer::drawDbClassification,
		sphericalBlendShapeVisualizer::drawRegistrantId,
		sphericalBlendShapeVisualizerDrawOverride::Creator);
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
		sphericalBlendShapeVisualizer::drawDbClassification,
		sphericalBlendShapeVisualizer::drawRegistrantId);
	if (!status) {
		status.perror("deregisterDrawOverrideCreator");
		return status;
	}

	status = plugin.deregisterNode( sphericalBlendShape::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
