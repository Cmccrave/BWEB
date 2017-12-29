#pragma once
#include "BWEB.h"

using namespace BWAPI;
using namespace BWEM;
using namespace std;

class Wall
{
	TilePosition wallSmall, wallMedium, wallLarge;
public:
	Wall() { };
	TilePosition getSmallWall() { return wallSmall; }
	TilePosition getMediumWall() { return wallMedium; }
	TilePosition getLargeWall() { return wallLarge; }

	void setSmallWall(TilePosition here) { wallSmall = here; }
	void setMediumWall(TilePosition here) { wallMedium = here; }
	void setLargeWall(TilePosition here) { wallLarge = here; }
};

//Wall::Wall(TilePosition small, TilePosition medium, TilePosition large)
//{
//
//}
