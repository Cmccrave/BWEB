#pragma once
#include "BWEB.h"

using namespace BWAPI;
using namespace BWEM;
using namespace std;

namespace BWEB
{	
	class Wall
	{
		TilePosition wallSmall, wallMedium, wallLarge;
		set<TilePosition> defenses;
	public:
		Wall() { };
		TilePosition getSmallWall() { return wallSmall; }
		TilePosition getMediumWall() { return wallMedium; }
		TilePosition getLargeWall() { return wallLarge; }
		set<TilePosition> getDefenses() { return defenses; }

		void setSmallWall(TilePosition here) { wallSmall = here; }
		void setMediumWall(TilePosition here) { wallMedium = here; }
		void setLargeWall(TilePosition here) { wallLarge = here; }
	};

	void findWalls();
	void findLargeWall();
	void findMediumWall();
	void findSmallWall();
}
