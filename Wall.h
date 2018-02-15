#pragma once
#include "BWEB.h"

namespace BWEB
{	
	using namespace BWAPI;
	using namespace std;

	class Wall
	{
		TilePosition wallDoor;
		set<TilePosition> defenses, small, medium, large;
		
	public:
		Wall() { wallDoor = TilePositions::Invalid; };

		void insertDefense(TilePosition here) { defenses.insert(here); }
		void setWallDoor(TilePosition here) { wallDoor = here; }
		void insertSegment(TilePosition, UnitType);
		
		// Returns the defense locations associated with this Wall
		const set<TilePosition> getDefenses() { return defenses; }

		// Returns the TilePosition belonging to the position where a melee unit should stand to fill the gap of the wall
		const TilePosition getDoor() { return wallDoor; }

		// Returns the TilePosition belonging to large UnitType buildings
		const set<TilePosition>& largeTiles() { return large; }

		// Returns the TilePosition belonging to medium UnitType buildings
		const set<TilePosition>& mediumTiles() { return medium; }

		// Returns the TilePosition belonging to small UnitType buildings
		const set<TilePosition>& smallTiles() { return small; }
	};	
}
