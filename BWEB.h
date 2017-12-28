#pragma once
#include <BWAPI.h>
#include "..\BWEM\bwem.h"

using namespace BWAPI;
using namespace BWEM;
using namespace std;

class Block
{
	int w, h;
	TilePosition t;
public:
	Block() { };
	Block(int width, int height, TilePosition tile) { w = width, h = height, t = tile; }
	int width() { return w; }
	int height() { return h; }
	TilePosition tile() { return t; }
};

class BWEBClass
{
	map<TilePosition, Block> blocks;	
	set<TilePosition> smallPosition, mediumPosition, largePosition, expoPosition;
	set<TilePosition> sDefPosition, mDefPosition;
	set<TilePosition> resourceCenter;
	TilePosition main, firstChoke, natural, secondChoke;
	TilePosition wallSmall, wallMedium, wallLarge;

	set<TilePosition> wallDefense;

	Area const * naturalArea;
	Area const * mainArea;

	bool overlapsBlocks(TilePosition);
	bool overlapsBases(TilePosition);
	bool overlapsNeutrals(TilePosition);
	bool overlapsMining(TilePosition);
	bool overlapsWalls(TilePosition);
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

	bool isWalkable(TilePosition);

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

	map<TilePosition, Block>& getBlocks() { return blocks; }
	set<TilePosition> getSmallPosition() { return smallPosition; }
	set<TilePosition> getMediumPosition() { return mediumPosition; }
	set<TilePosition> getLargePosition() { return largePosition; }
	set<TilePosition> getExpoPosition() { return expoPosition; }
	set<TilePosition> getSDefPosition() { return sDefPosition; }
	set<TilePosition> getMDefPosition() { return mDefPosition; }

	TilePosition getFirstChoke() { return firstChoke; }
	TilePosition getSecondChoke() { return secondChoke; }
	TilePosition getNatural() { return natural; }
	TilePosition getSmallWall() { return wallSmall; }
	TilePosition getMediumWall() { return wallMedium; }
	TilePosition getLargeWall() { return wallLarge; }

	int getGroundDistance(Position, Position);
};