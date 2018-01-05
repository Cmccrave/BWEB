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

		// Blocks
		void findStartBlocks(), findBlocks();
		vector<Block> blocks;
		bool canAddBlock(TilePosition, int, int, bool);
		void insertSmallBlock(TilePosition, bool, bool);
		void insertMediumBlock(TilePosition, bool, bool);
		void insertLargeBlock(TilePosition, bool, bool);
		void insertStartBlock(TilePosition, bool, bool);

		// Wall
		void findWalls(), findLargeWall(), findMediumWall(), findSmallWall(), findWallDefenses(), findPath();
		bool canPlaceHere(UnitType, TilePosition);
		map<BWEM::Area const *, Wall> areaWalls;
		int reservePath[256][256] = {};

		// Map
		void findMain(), findFirstChoke(), findSecondChoke(), findNatural();
		TilePosition tStart, firstChoke, natural, secondChoke;
		BWEM::Area const * naturalArea;
		BWEM::Area const * mainArea;
		Position pStart;

		// Station
		void findStations();
		vector<Station> stations;
		set<TilePosition>& stationDefenses(TilePosition, bool, bool);
		set<TilePosition> returnValues;

		// General
		static Map* BWEBInstance;			

	public:
		void draw(), onStart(), onCreate(Unit), onDestroy(Unit);	
		static Map &Instance();

		// Returns the closest build position possible for a building, with optional parameters of what tiles are used already and where you want to build closest to
		TilePosition getBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());
		
		// Returns all the walls -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		map<BWEM::Area const *, Wall> getWalls() { return areaWalls; }

		// Returns the wall for this area if it exists -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		Wall getWall(BWEM::Area const* area);
		
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
		
		// Returns the TilePosition of the first chokepoint
		TilePosition getFirstChoke() { return firstChoke; }

		// Returns the TilePosition of the second chokepoint
		TilePosition getSecondChoke() { return secondChoke; }

		// Returns the TilePosition of the natural expansion
		TilePosition getNatural() { return natural; }
	};

}