#include "TestSimulation.h"
#include "FillMLDDPits.h"
#include "OperationInterface.h"
#include "StatFile.h"
#include "FillMLDDPits.h"
#include "MarkTouchedRasters.h"
#include "MLDDFunctions.h"
#include <iostream>

#ifdef VISUALIZATION
#include "TerrainRenderWindow.h"
#endif

#include "defs.h"
#include "FillPITs.h"
#include "InterpolateRasterToSquares.h"

using namespace std;
using namespace TR;

namespace SIMULATION
{

TestSimulation::TestSimulation(ostream & os, ParamHandler & params):
	Simulation(os,params)
{}

bool TestSimulation::run()
{

	setOutputDirectory("c:\\terrain_output");
	size_t size = 10;
	DblRasterMx mx(size, size, 10.0, origoBottomLeft, 0.0);
	DblRasterMx channel(size, size, 10.0, origoBottomLeft, 0.0);
	for (size_t i = 0; i < size; ++i) {
		mx(i,i) = (i+1) * 10.0;
		channel(i,i) = 1.0;
	}

	DblRasterMx distances;
	RasterPositionMatrix positions;
	distanceTransform(mx, distances, positions);

	printmx(distances);
	saveToAsc(positions,0,"positions");

	DblRasterMx valley;
	create_sample_terrain(mx, true, 1.0, valley);
	saveToAsc(valley,0,"valley");

	MultiflowDMatrix mLDD;
	TR::semiMultiflowLDD(valley, mLDD, false);

	DblRasterMx channelHeads;
	findChannelHeads(mLDD, channel, channelHeads); 

	::saveToAsc(channelHeads,0,"channelHeads");

	/*
	setOutflowType(ofAllSides);

	DblRasterMx mx;
	DblRasterMx mxRet;
	DblRasterMx mxRetMax;
	DblRasterMx mxPoints;
	MultiflowDMatrix  mxMLDD;

	double initVal = 0.0;

	mapattr(10,10,10,0,mx);
	mapattr(10,10,10,0,mxPoints);

	mxPoints(5,5)=1;
	mxPoints(2,2)=1;

	for (int i = 0; i < 10; i++){
		for (int j = 0; j < 10; j++){
			mx(i,j)=i+j;	
		}
	}
	mx(2,2) = 50;
	printmx(mx);
	multiflowLDD( 1, mx, mxMLDD,true);
	printmx(mxMLDD);
	spreadLDD(mxMLDD,mxPoints,mxRet,0.0);
	spreadLDDMax(mxMLDD,mxPoints,mxRetMax,0.0);
	
	DblRasterMx mxLongestflowpathlength;
	longestflowpathlength(mxMLDD, mxLongestflowpathlength);
    printmx(mxRet);
	printmx(mxRetMax);
	*/
	/*
	::saveToAsc(mx,0,"a");
	::saveToAsc(mx,0,"b",true);
	
	DblRasterMx mxFluid;
	mapattr(10,10,10, 1.0, mxFluid);
	DblRasterMx mxAccflux;
	mapattr(10,10,10,0, mxAccflux);
	accflux(mxMLDD,mxFluid,mxAccflux,0.1);

	DblRasterMx mxUpstreamTotal;
	upstreamtotal( mxMLDD, mx, mxUpstreamTotal,false,true);

	DblRasterMx mxDownstreammin;

    RasterPositionMatrix mxPos;
	downstreammin(mxMLDD, mx, mxDownstreammin,mxPos,true);
	*/
	return true;
}
}