#pragma once
#include "BWEB.h"

namespace BWEB
{
	using namespace BWAPI;
	using namespace std;

	class BWEBUtil
	{
	public:
		bool overlapsBlocks(TilePosition);
		bool overlapsStations(TilePosition);
		bool overlapsNeutrals(TilePosition);
		bool overlapsMining(TilePosition);
		bool overlapsWalls(TilePosition);
		bool overlapsAnything(TilePosition, int, int);
		bool isWalkable(TilePosition);
	};
}