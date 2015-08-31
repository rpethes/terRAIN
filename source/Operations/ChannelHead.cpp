#include "ChannelHead.h"
#include "OperationInterface.h"

namespace TR
{


ChannelHead::ChannelHead(size_t id, RasterPosition pos, double generationTime) :
	_id(id), _pos(pos), _generationTime(generationTime), _ereaseTime(-1.0), _lastMoveTime(generationTime)
{
	ChannelHeadHistoryRecord record;
	record.pos = pos;
	record.time = generationTime;
	_history.push_back(record);
}

size_t ChannelHead::getID() const
{
	return _id;
}

RasterPosition ChannelHead::getPosition() const
{
	return _pos;
}
	
double ChannelHead::getGenerationTime() const
{	
	return _generationTime;
}
	
double ChannelHead::getEreaseTime() const
{
	return _ereaseTime;
}
	
double ChannelHead::getLastMoveTime() const
{
	return _lastMoveTime;
}
	
const ChannelHeadHistory & ChannelHead::history() const
{
	return _history;
}


bool ChannelHead::isExist() const
{
	return ( _ereaseTime > _generationTime);
}

void ChannelHead::moveTo(RasterPosition pos, double time)
{
	ChannelHeadHistoryRecord record;
	record.pos = pos;
	record.time = time;
	_history.push_back(record);

	_pos = pos;
	_lastMoveTime = time;
}

void ChannelHead::erease(double ereaseTime)
{
	_ereaseTime = ereaseTime;
}


ChannelHeadTracker::ChannelHeadTracker(size_t sizeX, size_t sizeY) : 
	_sizeX(sizeX), _sizeY(sizeY), _lastID(0)
{
	int initVal = 0;
	_channelHeads.init(_sizeY, _sizeX, 1.0, origoBottomLeft, initVal);
	_prevChannels.init(_sizeY, _sizeX, 1.0, origoBottomLeft, initVal);
}
	
int ChannelHeadTracker::lastID() const
{
	return _lastID;
}
	
const ChannelHeadMap & ChannelHeadTracker::channelHeadMap() const
{
	return _channelHeadMap;
}

const IntRasterMx & ChannelHeadTracker::channelHeads() const
{
	return _channelHeads;
}

bool ChannelHeadTracker::track(const DblRasterMx & currentChannelHeads, DblRasterMx & currentChannels, double time)
{
	DblRasterMx currentChannelHeadsCopy(currentChannelHeads);

	//first try to track the current channel heads:
	
	IntRasterMx::iterator iChannelHeads = _channelHeads.begin(),  endChannelHeads = _channelHeads.end();

	for (; iChannelHeads != endChannelHeads; ++iChannelHeads) {
		int id = *iChannelHeads;

		if (id > 0) {
			size_t row = iChannelHeads.getRow();
			size_t col = iChannelHeads.getCol();

			DblRasterMx::iterator iCurrentChannelHead = currentChannelHeadsCopy.getIteratorAt(row, col);
			
			// the channel head did not move
			if (*iCurrentChannelHead > 1e-6) {
				*iCurrentChannelHead = 0.0;
				continue;
			}

			// check if the channel head moved
			DblRasterMx::iterator::dCoords delta;
			delta._nDCol = 0;
			delta._nDRow = 0;
			RasterPositionVector neighbourChannelHeadPositions(8);
			for (unsigned char cc = 1; cc < 10; ++cc) {
				if (cc==5 || !iCurrentChannelHead.isValidItemByChainCode(cc))
					continue;
				double val = iCurrentChannelHead.chain_code(cc);
				if (val > 1e-6) {
					delta = iCurrentChannelHead.getChainCodeDelta(cc);
					RasterPosition pos(row + delta._nDRow, col + delta._nDCol);
					neighbourChannelHeadPositions.push_back(pos);
				}
			}
			
			size_t n = neighbourChannelHeadPositions.size();
			RasterPosition new_pos;
			if (n == 0) { // the channel head is ereased 
				*iChannelHeads = 0;
				_channelHeadMap[id].erease(time);
				continue;
			} else if (n == 1) { // the channel head moved, and there is only one direction	
				new_pos = neighbourChannelHeadPositions.front();
			} else if (n > 1) { // the channel head moved, but there are multiple directions
				
				DblRasterMx channelPixels;
				copyChannelTo(_prevChannels, row, col, channelPixels); 
				DblRasterMx distances; 
				RasterPositionMatrix positions;
				distanceTransform(channelPixels, distances, positions);
				
				double minDistance = DoubleUtil::getMAXValue();
				int minDistanceIndex = 0;

				for (int i = 0; i < n; ++i ) {
					RasterPosition & pos = neighbourChannelHeadPositions[i];
					double distance = channelDistance( currentChannels, pos.getRow(), pos.getCol(), distances);
					if (distance < minDistance ) {
						minDistance = distance;
						minDistanceIndex = i;
					}
				}
				new_pos = neighbourChannelHeadPositions[minDistanceIndex];
			}

			*iChannelHeads = 0;
			_channelHeads(new_pos.getRow(), new_pos.getCol()) = id;
		    _channelHeadMap[id].moveTo(new_pos, time);
		    currentChannelHeadsCopy(new_pos.getRow(), new_pos.getCol()) = 0.0;
		}
	}
	
	// find the new channel heads
	DblRasterMx::iterator iCurrentChannelHead = currentChannelHeadsCopy.begin(), endCurrentChannelHead = currentChannelHeadsCopy.end();

	for (; iCurrentChannelHead != endCurrentChannelHead; ++iCurrentChannelHead) {
		if (*iCurrentChannelHead > 1e-6) {
			RasterPosition pos(iCurrentChannelHead.getRow(), iCurrentChannelHead.getCol());
			++_lastID;
			_channelHeadMap[_lastID] = ChannelHead(_lastID, pos, time);
		}
	}

	_prevChannels = currentChannels;

	return true;
}

void ChannelHeadTracker::copyChannelTo(DblRasterMx & channels, size_t channelHeadRow, size_t channelHeadCol, DblRasterMx & target)
{
	double  initVal = 0;
	target.initlike(channels);
	target.fill(0.0);

	size_t currentRow = channelHeadRow, currentCol = channelHeadCol;
	size_t prevRow = 0, prevCol = 0;

	double currentVal = channels(currentRow, currentCol);

	while ( currentVal > 1e-6) {
		target(	currentRow, currentCol ) = 1.0;
		currentVal = 0.0;
		DblRasterMx::iterator it = channels.getIteratorAt( currentRow, currentCol );
		prevRow = currentRow;
		prevCol = currentCol;
		for (char cc = 1; cc < 10; ++cc) {
			if (cc == 5 || !it.isValidItemByChainCode(cc))
				continue;
			double val = it.chain_code(cc);
			if (val > 1e-6) {
				DblRasterMx::iterator itNeighbour;
				it.neighbourIterator(cc, itNeighbour);
				if ( itNeighbour.getRow()!=prevRow ||  itNeighbour.getCol() != prevCol) {
					currentRow = itNeighbour.getRow();
					currentCol = itNeighbour.getCol();
					currentVal = channels(currentRow, currentCol);
					break;
				}
			}
		}

	}

}

double channelDistance( DblRasterMx & channels, size_t channelHeadRow, size_t channelHeadCol, DblRasterMx & distances)
{
	double sum = 0.0;

	size_t currentRow = channelHeadRow, currentCol = channelHeadCol;
	size_t prevRow = 0, prevCol = 0;

	double currentVal = channels(currentRow, currentCol);

	while ( currentVal > 1e-6) {
		sum += distances(currentRow, currentCol);  
		currentVal = 0.0;
		DblRasterMx::iterator it = channels.getIteratorAt( currentRow, currentCol );
		prevRow = currentRow;
		prevCol = currentCol;
		for (char cc = 1; cc < 10; ++cc) {
			if (cc == 5 || !it.isValidItemByChainCode(cc))
				continue;
			double val = it.chain_code(cc);
			if (val > 1e-6) {
				DblRasterMx::iterator itNeighbour;
				it.neighbourIterator(cc, itNeighbour);
				if ( itNeighbour.getRow()!=prevRow ||  itNeighbour.getCol() != prevCol) {
					currentRow = itNeighbour.getRow();
					currentCol = itNeighbour.getCol();
					currentVal = channels(currentRow, currentCol);
					break;
				}
			}
		}

	}
}

}