#pragma once
#include "BWEB.h"

using namespace BWAPI;
using namespace BWEM;
using namespace std;

namespace BWEB
{
	class Block
	{
		int w, h;
		TilePosition t;
	public:
		Block() { };
		Block(int width, int height, TilePosition tile) { w = width, h = height, t = tile; }
		int width() { return w; }
		int height() { return h; }
		TilePosition tile() { return t; }
	};

	namespace Blocks
	{
		map<TilePosition, Block> blocks;
		set<TilePosition> resourceCenter;
		set<TilePosition> smallPosition, mediumPosition, largePosition, expoPosition, sDefPosition, mDefPosition;

		void findBlocks();
		bool canAddBlock(TilePosition, int, int, bool);
		void insertSmallBlock(TilePosition, bool, bool);
		void insertMediumBlock(TilePosition, bool, bool);
		void insertLargeBlock(TilePosition, bool, bool);
		void insertStartBlock(TilePosition, bool, bool);

		// Returns all the blocks and the TilePosition of their top left corner
		map<TilePosition, Block>& getBlocks() { return blocks; }

		set<TilePosition> getSmallPosition() { return smallPosition; }
		set<TilePosition> getMediumPosition() { return mediumPosition; }
		set<TilePosition> getLargePosition() { return largePosition; }
		set<TilePosition> getExpoPosition() { return expoPosition; }
		set<TilePosition> getSDefPosition() { return sDefPosition; }
		set<TilePosition> getMDefPosition() { return mDefPosition; }
		set<TilePosition> getResourceCenter() { return resourceCenter; }
	}
}
