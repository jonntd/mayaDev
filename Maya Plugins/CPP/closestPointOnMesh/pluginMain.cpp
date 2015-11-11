#include "closestPointOnMeshNode.h"
#include <maya/MFnPlugin.h>


MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, "Sven Pohle", "2016", "Any");

	status = plugin.registerNode("closestPointOnMesh", closestPointOnMesh::id, closestPointOnMesh::creator,
								  closestPointOnMesh::initialize,
								  MPxNode::kLocatorNode,
								  &closestPointOnMesh::drawDbClassification);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
		closestPointOnMesh::drawDbClassification,
		closestPointOnMesh::drawRegistrantId,
		closestPointOnMeshDrawOverride::Creator);
	if (!status) {
		status.perror("registerDrawOverrideCreator");
		return status;
	}

   /* status = plugin.registerCommand("closestPointOnMeshCreate", closestPointOnMeshCommand::creator );
	if (!status) {
		status.perror("Register Command [closestPointOnMeshCreate] failed!");
		return status;
	}*/

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
		closestPointOnMesh::drawDbClassification,
		closestPointOnMesh::drawRegistrantId);
	if (!status) {
		status.perror("deregisterDrawOverrideCreator");
		return status;
	}

	status = plugin.deregisterNode( closestPointOnMesh::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	/*status = plugin.deregisterCommand("closestPointOnMeshCreate");
	if (!status) {
		status.perror("DeRegister Command [closestPointOnMeshCreate] failed!");
		return status;
	}*/

	return status;
}
