#pragma once
#include "BWEB.h"

namespace BWEB
{	
	using namespace BWAPI;
	using namespace std;

	class Wall
	{
		TilePosition wallSmall, wallMedium, wallLarge, wallDoor;
		set<TilePosition> defenses;
	public:
		Wall() { };
		Wall(TilePosition, TilePosition, TilePosition, set<TilePosition>);

		// Returns the small building associated with this Wall
		const TilePosition getSmallWall() { return wallSmall; }

		// Returns the medium building associated with this Wall
		const TilePosition getMediumWall() { return wallMedium; }

		// Returns the large building associated with this Wall
		const TilePosition getLargeWall() { return wallLarge; }

		// Returns the defense locations associated with this Wall
		const set<TilePosition> getDefenses() { return defenses; }

		// Returns the TilePosition belonging to the position where a melee unit should stand to fill the gap of the wall
		const TilePosition getDoor() { return wallDoor; }

		void setSmallWall(TilePosition here) { wallSmall = here; }
		void setMediumWall(TilePosition here) { wallMedium = here; }
		void setLargeWall(TilePosition here) { wallLarge = here; }
		void insertDefense(TilePosition here) { defenses.insert(here); }
		void setWallDoor(TilePosition here) { wallDoor = here; }
	};	
}
