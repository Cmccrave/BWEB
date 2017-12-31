#pragma once
#include <BWAPI.h>
#include "..\BWEM\bwem.h"
#include "Block.h"
#include "Wall.h"
#include "BWEBUtil.h"

using namespace BWAPI;
using namespace BWEM;
using namespace BWEB;
using namespace std;

namespace BWEB
{
	class Block;
	class Wall;

	static TilePosition firstChoke, natural, secondChoke;
	static map<Area const *, Wall> areaWalls;
	static Area const * naturalArea;
	static Area const * mainArea;
	static map<TilePosition, Block> blocks;
	static set<TilePosition> resourceCenter;
	static set<TilePosition> smallPosition, mediumPosition, largePosition, expoPosition, sDefPosition, mDefPosition;

	void findFirstChoke();
	void findSecondChoke();
	void findNatural();

	class BWEBClass
	{			
	private:
		static BWEBClass* bInstance;
		
	public:
		void draw();
		void onStart();
		static BWEBClass &Instance();

		// Returns the closest build position possible for a building designed for anything except defenses, with optional parameters of what tiles are used already and where you want to build closest to
		TilePosition getBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());

		// Returns the closest build position possible for a building designed for defenses, with optional parameters of what tiles are used already and where you want to build closest to
		TilePosition getDefBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());

		// Returns the closest build position possible, with optional parameters of what tiles are used already and where you want to build closest to
		TilePosition getAnyBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());
		
		// Returns all the walls -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		static map<Area const *, Wall> getWalls() { return areaWalls; }

		// Returns the wall for this area if it exists -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
		Wall getWall(Area const* area);

		// Returns the block at this TilePosition if it exists
		Block getBlock(TilePosition here);
		
		// Returns the area of the natural expansion
		static Area const * getNaturalArea() { return naturalArea; }

		// Returns the area of the main
		static Area const * getMainArea() { return mainArea; }

		// Returns the estimated ground distance from a Position to another Position
		int getGroundDistance(Position, Position);

		// Returns all the blocks and the TilePosition of their top left corner
		map<TilePosition, Block>& getBlocks() { return blocks; }

		set<TilePosition> getSmallPosition() { return smallPosition; }
		set<TilePosition> getMediumPosition() { return mediumPosition; }
		set<TilePosition> getLargePosition() { return largePosition; }
		set<TilePosition> getExpoPosition() { return expoPosition; }
		set<TilePosition> getSDefPosition() { return sDefPosition; }
		set<TilePosition> getMDefPosition() { return mDefPosition; }
		set<TilePosition> getResourceCenter() { return resourceCenter; }
		
		static TilePosition getFirstChoke() { return firstChoke; }
		static TilePosition getSecondChoke() { return secondChoke; }
		static TilePosition getNatural() { return natural; }
	};

}