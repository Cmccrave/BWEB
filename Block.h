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

	void findBlocks();
	bool canAddBlock(TilePosition, int, int, bool);
	void insertSmallBlock(TilePosition, bool, bool);
	void insertMediumBlock(TilePosition, bool, bool);
	void insertLargeBlock(TilePosition, bool, bool);
	void insertStartBlock(TilePosition, bool, bool);
}
