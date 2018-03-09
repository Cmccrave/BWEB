#pragma once
#pragma warning(disable : 4351)
#include <set>

#include "BWAPI.h"
#include "..\BWEM\bwem.h"
#include "Station.h"
#include "Block.h"
#include "Wall.h"

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

		TilePosition testTile;

		// Blocks
		void findStartBlocks();
		vector<Block> blocks;
		bool canAddBlock(TilePosition, int, int, bool);
		void insertSmallBlock(TilePosition, bool, bool);
		void insertMediumBlock(TilePosition, bool, bool);
		void insertLargeBlock(TilePosition, bool, bool);
		void insertTinyBlock(TilePosition, bool, bool);
		void insertStartBlock(TilePosition, bool, bool);
		void insertTechBlock(TilePosition, bool, bool);

		// Wall		
		map<BWEM::Area const *, Wall> areaWalls;
		int reservePath[256][256] ={};
		int enemyPath[256][256] ={};
		int testPath[256][256] ={};
		bool wallTight(UnitType, TilePosition);
		bool canPlaceHere(UnitType, TilePosition);
		bool powersWall(TilePosition);
		TilePosition currentHole;
		double distToChokeGeo(Position);

		double bestWallScore;
		map<TilePosition, UnitType> bestWall;
		map<TilePosition, UnitType> currentWall;
		tuple<double, TilePosition> scoreWallSegment(UnitType, bool tight = true);
		int tightState(WalkPosition, UnitType);
		bool walled, R, L, T, B;

		// Map
		void findMain(), findFirstChoke(), findSecondChoke(), findNatural();
		TilePosition tStart, firstChoke, natural, secondChoke;
		set<TilePosition> usedTiles;
		BWEM::Area const * naturalArea;
		BWEM::Area const * mainArea;
		BWEM::ChokePoint const * naturalChoke;
		Position pStart;
		bool buildingFits(TilePosition, UnitType, const set<TilePosition>& ={});

		// Station
		void findStations();
		vector<Station> stations;
		set<TilePosition>& stationDefenses(TilePosition, bool, bool);
		set<TilePosition> returnValues;

		// General
		static Map* BWEBInstance;

	public:
		void draw(), onStart(), onUnitDiscover(Unit), onUnitDestroy(Unit), onUnitMorph(Unit);
		static Map &Instance();

		UnitType overlapsCurrentWall(TilePosition, int width = 1, int height = 1);

		// Returns the closest build position possible for a building, with optional parameters of what tiles are used already and where you want to build closest to
		TilePosition getBuildPosition(UnitType, const set<TilePosition>& ={}, TilePosition = Broodwar->self()->getStartLocation());

		// Returns the closest build position possible for a defense building, with optional parameters of what tiles are used already and where you want to build closest to
		TilePosition getDefBuildPosition(UnitType, const set<TilePosition>& ={}, TilePosition = Broodwar->self()->getStartLocation());

		// Returns all the walls -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		map<BWEM::Area const *, Wall> getWalls() { return areaWalls; }

		// Returns the wall for this area if it exists -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		Wall* getWall(BWEM::Area const* area);

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
		const Station& getClosestStation(TilePosition) const;

		// Returns the TilePosition of the first chokepoint
		TilePosition getFirstChoke() { return firstChoke; }

		// Returns the TilePosition of the second chokepoint
		TilePosition getSecondChoke() { return secondChoke; }

		// Returns the TilePosition of the natural expansion
		TilePosition getNatural() { return natural; }

		// Returns the set of used TilePositions
		set<TilePosition>& getUsedTiles() { return usedTiles; }

		// Given a UnitType, this will store a position for this UnitType at your natural as a piece of the wall
		void findWallSegment(UnitType, bool = true);

		// Given a set of UnitTypes, finds an optimal wall placement
		void findFullWall(vector<UnitType>&);

		// This will store up to 6 small positions for defenses for a wall at your natural
		void findWallDefenses(UnitType, int = 1);

		// This will create a path that walls cannot be built on, connecting your main to your natural. Call it only once.
		void findPath();

		// Erases any blocks at the specified TilePosition
		void eraseBlock(TilePosition);

		// Experimenting with being able to build blocks after building a wall
		void findBlocks();

	};

}