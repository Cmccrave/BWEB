#pragma once
#include <BWAPI.h>
#include "..\BWEM\bwem.h"
#include "Block.h"
#include "Wall.h"
#include "BWEBUtil.h"

using namespace BWAPI;
using namespace BWEM;
using namespace std;

class BWEBClass
{
	set<TilePosition> smallPosition, mediumPosition, largePosition, expoPosition, sDefPosition, mDefPosition;
	set<TilePosition> resourceCenter;
	TilePosition main, firstChoke, natural, secondChoke;
	map<Area const *, Wall> walls;
	map<TilePosition, Block> blocks;
	Area const * naturalArea;
	Area const * mainArea;
	bool canAddBlock(TilePosition, int, int, bool);	
	void insertSmallBlock(TilePosition, bool, bool);
	void insertMediumBlock(TilePosition, bool, bool);
	void insertLargeBlock(TilePosition, bool, bool);
	void insertHExpoBlock(TilePosition, bool, bool);
	void insertVExpoBlock(TilePosition, bool, bool);
	void findFirstChoke();
	void findSecondChoke();
	void findNatural();
	void findWalls();
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

	// Returns all the blocks and the TilePosition of their top left corner
	map<TilePosition, Block>& getBlocks() { return blocks; }

	// Returns all the walls -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
	map<Area const *, Wall> getWalls() { return walls; }

	// Returns the block at this TilePosition if it exists
	Block getBlock(TilePosition here);

	// Returns the wall for this area if it exists -- CURRENTLY ONLY NATURAL WALL, use at own risk if not using the BWEB natural area
	Wall getWall(Area const* area);

	// Returns the area of the natural expansion
	Area const * getNaturalArea() { return naturalArea; }

	// Returns the estimated ground distance from a Position to another Position
	int getGroundDistance(Position, Position);

	set<TilePosition> getSmallPosition() { return smallPosition; }
	set<TilePosition> getMediumPosition() { return mediumPosition; }
	set<TilePosition> getLargePosition() { return largePosition; }
	set<TilePosition> getExpoPosition() { return expoPosition; }
	set<TilePosition> getSDefPosition() { return sDefPosition; }
	set<TilePosition> getMDefPosition() { return mDefPosition; }
	set<TilePosition> getResourceCenter() { return resourceCenter; }

	TilePosition getFirstChoke() { return firstChoke; }
	TilePosition getSecondChoke() { return secondChoke; }
	TilePosition getNatural() { return natural; }
};