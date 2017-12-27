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

class BWEB
{
	map<TilePosition, Block> blocks;	
	set<TilePosition> smallPosition, mediumPosition, largePosition, expoPosition;
	set<TilePosition> sDefPosition, mDefPosition;
	set<TilePosition> resourceCenter;

	bool overlapsBlocks(TilePosition);
	bool overlapsBases(TilePosition);
	bool overlapsNeutrals(TilePosition);
	bool overlapsMining(TilePosition);
	bool canAddBlock(TilePosition, int, int);

	void insertSmallBlock(TilePosition, bool, bool);
	void insertMediumBlock(TilePosition, bool, bool);
	void insertLargeBlock(TilePosition, bool, bool);
	void insertHExpoBlock(TilePosition, bool, bool);
	void insertVExpoBlock(TilePosition, bool, bool);

	static BWEB* bInstance;

public:
	void draw();	
	void onStart();
	static BWEB &Instance();
	TilePosition getBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());
	TilePosition getDefBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());
	TilePosition getAnyBuildPosition(UnitType, const set<TilePosition>* = nullptr, TilePosition = Broodwar->self()->getStartLocation());
	map<TilePosition, Block>& getBlocks() { return blocks; }
	set<TilePosition> getSmallPosition() { return smallPosition; }
	set<TilePosition> getMediumPosition() { return mediumPosition; }
	set<TilePosition> getLargePosition() { return largePosition; }
	set<TilePosition> getExpoPosition() { return expoPosition; }

	set<TilePosition> getSDefPosition() { return sDefPosition; }
	set<TilePosition> getMDefPosition() { return mDefPosition; }
};