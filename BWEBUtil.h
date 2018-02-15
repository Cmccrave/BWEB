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
		bool overlapsAnything(TilePosition here, int width = 1, int height = 1, bool ignoreBlocks = false);
		bool isWalkable(TilePosition);
		bool insideNatArea(TilePosition here, int width = 1, int height = 1);
	};
}