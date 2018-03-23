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
		const BWEM::Area * area;
		const BWEM::ChokePoint * choke;
		
	public:
		Wall(const BWEM::Area *, const BWEM::ChokePoint *);
		void insertDefense(TilePosition here) { defenses.insert(here); }
		void setWallDoor(TilePosition here) { wallDoor = here; }
		void insertSegment(TilePosition, UnitType);

		const BWEM::ChokePoint * getChokePoint() const { return choke; }
		const BWEM::Area * getArea() const { return area; }
		
		// Returns the defense locations associated with this Wall
		set<TilePosition> getDefenses() const { return defenses; }

		// Returns the TilePosition belonging to the position where a melee unit should stand to fill the gap of the wall
		TilePosition getDoor() const { return wallDoor; }

		// Returns the TilePosition belonging to large UnitType buildings
		set<TilePosition> largeTiles() const { return large; }

		// Returns the TilePosition belonging to medium UnitType buildings
		set<TilePosition> mediumTiles() const { return medium; }

		// Returns the TilePosition belonging to small UnitType buildings
		set<TilePosition> smallTiles() const { return small; }
	};	
}
