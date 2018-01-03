#pragma once
#pragma warning(disable : 4351)
#include <BWAPI.h>
#include "..\BWEM\bwem.h"
#include "Base.h"
#include "Block.h"
#include "Wall.h"
#include "BWEBUtil.h"

namespace BWEB
{
	using namespace BWAPI;
	using namespace std;

	class Block;
	class Wall;
	class Base;
	class Map
	{
	private:
		TilePosition firstChoke, natural, secondChoke;
		map<BWEM::Area const *, Wall> areaWalls;
		BWEM::Area const * naturalArea;
		BWEM::Area const * mainArea;
		map<TilePosition, Block> expoBlocks, prodBlocks, techBlocks;
		set<TilePosition> smallPosition, mediumPosition, largePosition, expoPosition, sDefPosition, mDefPosition, resourceCenter;
		Position pStart;
		TilePosition tStart;

		vector<Base> bases;
		set<TilePosition> returnValues;

		void findMain(), findFirstChoke(), findSecondChoke(), findNatural(), findStartBlocks(), findBlocks(), findWalls(), findLargeWall(), findMediumWall(), findSmallWall(), findWallDefenses(), findPath();

		void findBases();

		int reservePathHome[256][256] = {};
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
		TilePosition getDefBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());

		// Returns the closest build position possible, with optional parameters of what tiles are used already and where you want to build closest to
		TilePosition getAnyBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());

		// Returns all the walls -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		map<BWEM::Area const *, Wall> getWalls() { return areaWalls; }

		// Returns the wall for this area if it exists -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		Wall getWall(BWEM::Area const* area);

		// Returns the block at this TilePosition if it exists
		Block getBlock(TilePosition here);

		// Returns the area of the natural expansion
		BWEM::Area const * getNaturalArea() { return naturalArea; }

		// Returns the area of the main
		BWEM::Area const * getMainArea() { return mainArea; }

		// Returns the estimated ground distance from a Position to another Position
		double getGroundDistance(Position, Position);

		// Returns all the production blocks and the TilePosition of their top left corner
		map<TilePosition, Block>& Production() { return prodBlocks; }

		// Returns all the BWEB bases with pointers to BWEM bases
		vector<Base>& Bases() { return bases; }
		
		TilePosition getFirstChoke() { return firstChoke; }
		TilePosition getSecondChoke() { return secondChoke; }
		TilePosition getNatural() { return natural; }
	};

}