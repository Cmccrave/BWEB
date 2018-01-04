#pragma once
#pragma warning(disable : 4351)
#include <BWAPI.h>
#include "..\BWEM\bwem.h"
#include "Station.h"
#include "Block.h"
#include "Wall.h"
#include "BWEBUtil.h"

namespace BWEB
{
	using namespace BWAPI;
	using namespace std;

	class Block;
	class Wall;
	class Station;
	class Map
	{
	private:
		TilePosition firstChoke, natural, secondChoke;
		map<BWEM::Area const *, Wall> areaWalls;
		BWEM::Area const * naturalArea;
		BWEM::Area const * mainArea;
		
		set<TilePosition> smallPosition, mediumPosition, largePosition, resourceCenter;
		Position pStart;
		TilePosition tStart;

		vector<Block> blocks;
		vector<Station> stations;
		set<TilePosition> returnValues;

		Station returnS;

		void findMain(), findFirstChoke(), findSecondChoke(), findNatural(), findStartBlocks(), findBlocks(), findWalls(), findLargeWall(), findMediumWall(), findSmallWall(), findWallDefenses(), findPath();

		void findStations();

		int reservePath[256][256] = {};
		bool canAddBlock(TilePosition, int, int, bool);
		void insertSmallBlock(TilePosition, bool, bool);
		void insertMediumBlock(TilePosition, bool, bool);
		void insertLargeBlock(TilePosition, bool, bool);
		void insertStartBlock(TilePosition, bool, bool);
		set<TilePosition>& baseDefenses(TilePosition, bool, bool);
		static Map* BWEBInstance;

	public:
		void draw();
		void onStart();
		static Map &Instance();

		// Returns the closest build position possible for a building designed for anything except defenses, with optional parameters of what tiles are used already and where you want to build closest to
		TilePosition getBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());

		// Returns the closest build position possible for a building designed for defenses, with optional parameters of what tiles are used already and where you want to build closest to
		// Commenting this out because Im not using def positions anymore, stations instead.
		// TilePosition getDefBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());

		// Returns the closest build position possible, with optional parameters of what tiles are used already and where you want to build closest to
		// Commenting this out because Im not using def positions anymore, stations instead.
		// TilePosition getAnyBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());

		// Returns all the walls -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		map<BWEM::Area const *, Wall> getWalls() { return areaWalls; }

		// Returns the wall for this area if it exists -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		Wall getWall(BWEM::Area const* area);

		// Returns the block at this TilePosition if it exists
		// Commenting this out because it probably will never be used, would need to know where the block is and if it exists before grabbing it
		//Block getBlock(TilePosition here);

		// Returns the BWEM Area of the natural expansion
		BWEM::Area const * getNaturalArea() { return naturalArea; }

		// Returns the BWEM Area of the main
		BWEM::Area const * getMainArea() { return mainArea; }

		// Returns the estimated ground distance from a Position to another Position
		double getGroundDistance(Position, Position);

		// Returns all the BWEB Blocks
		vector<Block>& Blocks() { return blocks; }

		// Returns all the BWEB Stations
		vector<Station>& Stations() { return stations; }

		// Returns the closest BWEB Station to the given TilePosition
		Station getClosestStation(TilePosition);
		
		TilePosition getFirstChoke() { return firstChoke; }
		TilePosition getSecondChoke() { return secondChoke; }
		TilePosition getNatural() { return natural; }
	};

}