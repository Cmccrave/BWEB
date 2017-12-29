#pragma once
#include "BWEB.h"

using namespace BWAPI;
using namespace BWEM;
using namespace std;

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