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
	set<TilePosition> smallPosition, mediumPosition, largePosition;
	void updateBlocks();

	bool overlapsBlocks(TilePosition);
	bool overlapsBases(TilePosition);
	bool overlapsNeutrals(TilePosition);
	bool canAddBlock(TilePosition, int, int);

	void insertSmallBlock(TilePosition, bool, bool);
	void insertMediumBlock(TilePosition, bool, bool);
	void insertLargeBlock(TilePosition, bool, bool);

	static BWEB* bInstance;

public:
	void update();	
	static BWEB &Instance();
	map<TilePosition, Block>& getBlocks() { return blocks; }
	set<TilePosition> getSmallPosition() { return smallPosition; }
	set<TilePosition> getMediumPosition() { return mediumPosition; }
	set<TilePosition> getLargePosition() { return largePosition; }
};