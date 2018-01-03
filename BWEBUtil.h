#pragma once
#include "BWEB.h"

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