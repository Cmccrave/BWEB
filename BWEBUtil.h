#pragma once
#include "BWEB.h"

using namespace BWAPI;
using namespace BWEM;
using namespace std;

namespace BWEB
{
	class BWEBUtil
	{
	public:
		bool overlapsBlocks(TilePosition);
		bool overlapsBases(TilePosition);
		bool overlapsNeutrals(TilePosition);
		bool overlapsMining(TilePosition);
		bool overlapsWalls(TilePosition);
		bool isWalkable(TilePosition);
	};
}