#include "timedBlendShapeNode.h"

#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <maya/MGlobal.h>

MTypeId     timedBlendShape::id( 0x00002 );
MObject timedBlendShape::aBlendMesh;
MObject timedBlendShape::aBlendWeight;
MObject timedBlendShape::aTimingWeights;
MObject timedBlendShape::aTimingWeightsList;

timedBlendShape::timedBlendShape() {}
timedBlendShape::~timedBlendShape() {}

/*
 * getWeights() - Uses data block to nicely get weights
 *  for given attrs passed in.  This gets all weights for the given listIdx.
 *
 *  Parameters:
 *          data        - MDataBlock passed in
 *          aWtList     - Attribute that is the per point larger compound weight list
 *          aWts        - Attribute that is the double array of weights inside of the WtList array.
 *          dArrWts     - Array of doubles to values we are getting.
 *          nTotal      - Total number of elements we expect in the weights array.
 *          listIdx     - What pt/list index for the first list array do we want
 */
MStatus timedBlendShape::getWeights(MDataBlock& data, MObject& aWtList, MObject& aWts, MFloatArray& dArrWts, int nTotal, int listIdx)
{
    MStatus stat ;
    dArrWts.setLength(nTotal) ; // first make sure array can hold it all
 
    // Get handle to array wt list
    MArrayDataHandle hArrWtList = data.inputArrayValue(aWtList, &stat) ;
 
    // Go to right pt/list index...
    stat = hArrWtList.jumpToElement( listIdx ) ;
    if (stat != MS::kSuccess)   // If that index doesn't exist, our wts are 0..so just return.
    {
        return stat;
    }
 
    // Now we get the listIdx-th compound element handle...
    MDataHandle hCmpdWtListEle = hArrWtList.inputValue(&stat) ; 
    if (stat != MS::kSuccess)
    {
        cout << "Odd, can't get weight list compound element handle!" << endl ;
        return stat ;
    }
 
 
    // Now from this compound, get the aWts weights array child...
    MDataHandle hWtListChild = hCmpdWtListEle.child(aWts);
 
    // Now get this child array as an MArrayDataHandle....
    MArrayDataHandle hArrWts(hWtListChild, &stat) ;
    if (stat != MS::kSuccess)
    {
        cout << "Odd, can't get weight list handle!" <<endl;
        return stat;
    }
 
    // Whew!  Now we can finally just go through the weights array and get the values
    // Since indexing is logical/sparse, not all elements may exist...
    int idx=0;
    for (idx=0; idx < nTotal; ++idx)
    {
        stat = hArrWts.jumpToElement(idx);
        if (stat != MS::kSuccess) 
        {      // If element doesnt exist..wt is 0.0
            dArrWts[idx] = -1.0;
        }
        else            // get value from child double
        {
            MDataHandle hWtEle = hArrWts.inputValue(&stat) ;
            if (stat != MS::kSuccess)       // Strange, cant get handle.
            {
                cout << "Odd, can't get weights element handle!" << endl ;
                dArrWts[idx] = 0.0 ;
            }
            else
            {
                dArrWts[idx] = hWtEle.asFloat() ; 
            }
        }
    }
 
    return MS::kSuccess ;

}


MStatus timedBlendShape::deform( MDataBlock& data, MItGeometry& itGeo, 
        const MMatrix& localToWorldMatrix, unsigned int geomIndex )
{
    MStatus status;

	MDataHandle hBlendMesh = data.inputValue( aBlendMesh, &status );
    CHECK_MSTATUS_AND_RETURN_IT( status );
    MObject oBlendMesh = hBlendMesh.asMesh();
    if ( oBlendMesh.isNull() )
    {
        return MS::kSuccess;
    }

    MFnMesh fnMesh( oBlendMesh, &status );
    CHECK_MSTATUS_AND_RETURN_IT( status );
    MPointArray blendPoints;
    fnMesh.getPoints( blendPoints );

    float blendWeight = data.inputValue( aBlendWeight ).asFloat();
    float env = data.inputValue( envelope ).asFloat();
    MPoint point;
	MPoint destPoint;
    float w;
	float slope, b, y;

	MFloatArray timingWeights;
	status = getWeights(data, aTimingWeightsList, aTimingWeights, timingWeights, itGeo.count(), geomIndex);

    for ( ; !itGeo.isDone(); itGeo.next() )
    {
        point = itGeo.position();
        w = weightValue(data, geomIndex, itGeo.index());

		// y = mx + b 
		// y = slope * x + b
		// slope = 1 / (1 - timing)
		// 
		// 1 = slope * x + b
		// 1 - slope = b
		slope = 0.0;
		b = 1;
		if (blendWeight < 1.0) {
			if (timingWeights[itGeo.index()] >= 1.0) { 
				slope = 1.0;
			} else {
				slope = 1 / (1-timingWeights[itGeo.index()]);
			}
			
			b = 1 - slope;
		}

		// linear equation :: y = m*x + b
		// we got m and b here
		y = slope * blendWeight + b;
		if (y < 0.0) { y = 0.0; }

		lerp(point, blendPoints[itGeo.index()], y, destPoint);

        itGeo.setPosition( destPoint );
    }


	return MS::kSuccess;
}

// linear interpolation
void timedBlendShape::lerp(MPoint& p1, MPoint& p2, double blend, MPoint& outPoint) {
	outPoint.x = p1.x + (p2.x - p1.x)*blend;
	outPoint.y = p1.y + (p2.y - p1.y)*blend;
	outPoint.z = p1.z + (p2.z - p1.z)*blend;
}

void* timedBlendShape::creator()
{
	return new timedBlendShape();
}

MStatus timedBlendShape::initialize()
{
	MFnNumericAttribute nAttr;
	MFnTypedAttribute   tAttr;
	MFnCompoundAttribute cAttr;

	MStatus status;

	aBlendMesh = tAttr.create("blendMesh", "blendMesh", MFnData::kMesh, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	aBlendWeight = nAttr.create("blendWeight", "blendWeight", MFnNumericData::kFloat, 0.0);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	nAttr.setKeyable(true);
	nAttr.setStorable(true);

	aTimingWeights = nAttr.create("timingWeights", "timingWeights", MFnNumericData::kFloat, 0.0);
	nAttr.setHidden(true);
	nAttr.setArray(true);
	nAttr.setUsesArrayDataBuilder(true);

	aTimingWeightsList = cAttr.create("timingWeightsList", "timingWeightsList");
	cAttr.addChild(aTimingWeights);
	cAttr.setHidden(true);
	cAttr.setArray(true);
	cAttr.setUsesArrayDataBuilder(true);

	addAttribute(aBlendMesh);
	addAttribute(aBlendWeight);
	addAttribute(aTimingWeights);
	addAttribute(aTimingWeightsList);

	attributeAffects(aBlendWeight, outputGeom);
	attributeAffects(aTimingWeights, outputGeom);
	attributeAffects(aTimingWeightsList, outputGeom);

	MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer timedBlendShape weights;");
	MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer timedBlendShape timingWeights;");

	return MS::kSuccess;
}

