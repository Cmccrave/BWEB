#include "BWEB.h"

void BWEB::update()
{
	updateBlocks();
}

void BWEB::updateBlocks()
{
	// For reference: https://imgur.com/a/I6IwH

	// Currently missing features:
	// - Mirroring isn't fully implemented
	// - Counting of how many of each type of block
	// - Defensive blocks
	// - Blocks for areas other than main
	// - Low variations so usually unoptimized
	// - Variations based on build order (bio build or mech build)
	// - Expansion blocks
	// - Resource avoidance
	// - Smooth density

	TilePosition tStart = Broodwar->self()->getStartLocation();
	Position pStart = Position(tStart) + Position(64, 48);
	Area const * mainArea = Map::Instance().GetArea(tStart);
	set<TilePosition> mainTiles;

	for (int x = 0; x <= Broodwar->mapWidth(); x++)
	{
		for (int y = 0; y <= Broodwar->mapHeight(); y++)
		{
			if (!TilePosition(x, y).isValid()) continue;
			if (!Map::Instance().GetArea(TilePosition(x, y))) continue;
			if (Map::Instance().GetArea(TilePosition(x, y)) == mainArea) mainTiles.insert(TilePosition(x, y));
		}
	}

	// Mirror? (Gate on right) -- removed due to McRave specific for now
	bool mirrorHorizontal = false;
	bool mirrorVertical = false;
	//if (Position(Terrain().getFirstChoke()).x > Terrain().getPlayerStartingPosition().x) mirrorHorizontal = true;
	//if (Position(Terrain().getFirstChoke()).y > Terrain().getPlayerStartingPosition().y) mirrorVertical = true;

	// Find first chunk position
	if (Broodwar->getFrameCount() == 0)
	{
		double distA = DBL_MAX;
		TilePosition best;
		for (auto &tile : mainTiles)
		{			
			Position blockCenter;
			if (Broodwar->self()->getRace() == Races::Protoss)
			{
				if (!canAddBlock(tile, 6, 8)) continue;
				blockCenter = Position(TilePosition(tile.x + 3, tile.y + 4));
			}
			else if (Broodwar->self()->getRace() == Races::Terran)
			{
				if (!canAddBlock(tile, 3, 4)) continue;
				blockCenter = Position(TilePosition(tile.x + 3, tile.y + 2));
			}

			double distB = blockCenter.getDistance(pStart); //* blockCenter.getDistance(Position(Terrain().getFirstChoke()));
			if (distB < distA)
			{
				distA = distB;
				best = tile;
			}
		}
		if (Broodwar->self()->getRace() == Races::Protoss) insertMediumBlock(best, mirrorHorizontal, mirrorVertical);
		else if (Broodwar->self()->getRace() == Races::Terran) insertSmallBlock(best, mirrorHorizontal, mirrorVertical);

		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			for (auto tile : mainTiles)
				if (canAddBlock(tile, 6, 8)) insertMediumBlock(tile, mirrorHorizontal, mirrorVertical);
			for (auto tile : mainTiles)
				if (canAddBlock(tile, 4, 5)) insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			for (auto tile : mainTiles)
				if (canAddBlock(tile, 6, 8)) insertMediumBlock(tile, mirrorHorizontal, mirrorVertical);
			for (auto tile : mainTiles)
				if (canAddBlock(tile, 3, 4)) insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
		}
	}

	for (auto tile : smallPosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
	for (auto tile : mediumPosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(3, 2)), Broodwar->self()->getColor());
	for (auto tile : largePosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(4, 3)), Broodwar->self()->getColor());
}

bool BWEB::canAddBlock(TilePosition here, int width, int height)
{
	// Check if a block of specified size would overlap any bases, resources or other blocks
	for (int x = here.x - 1; x < here.x + width + 1; x++)
	{
		for (int y = here.y - 1; y < here.y + height + 1; y++)
		{
			if (!TilePosition(x, y).isValid()) return false;
			if (!Map::Instance().GetTile(TilePosition(x, y)).Buildable()) return false;				// BWEM call
			//if (Grids().getResourceGrid(x, y) > 0) return false;							// If you don't have anything to prevent building near minerals, leave this commented out
			//if (overlapsBlocks(TilePosition(x, y))) return false;
			if (overlapsBases(TilePosition(x, y))) return false;					// Function that iterates all bases to see if they overlap
			if (overlapsNeutrals(TilePosition(x, y))) return false;				// Function that iterates all minerals and geysers to see if they overlap
		}
	}
	return true;
}

bool BWEB::overlapsBlocks(TilePosition here)
{
	for (auto &b : blocks)
	{
		Block& block = b.second;
		if (here.x >= block.tile().x && here.x < block.tile().x + block.width() && here.y >= block.tile().y && here.y < block.tile().y + block.height()) return true;
	}
	return false;
}

void BWEB::insertSmallBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		blocks[here] = Block(4, 5, here);
		if (mirrorHorizontal)
		{
			smallPosition.insert(here);
			smallPosition.insert(here + TilePosition(2, 0));
			largePosition.insert(here + TilePosition(0, 2));
		}
		else
		{
			smallPosition.insert(here + TilePosition(0, 3));
			smallPosition.insert(here + TilePosition(2, 3));
			largePosition.insert(here);
		}
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		blocks[here] = Block(3, 4, here);
		mediumPosition.insert(here);		
		mediumPosition.insert(here + TilePosition(0, 2));
	}
}

void BWEB::insertMediumBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		blocks[here] = Block(6, 8, here);
		mediumPosition.insert(here + TilePosition(0, 6));
		mediumPosition.insert(here + TilePosition(3, 6));

		if (mirrorHorizontal)
		{
			smallPosition.insert(here);
			smallPosition.insert(here + TilePosition(0, 2));
			smallPosition.insert(here + TilePosition(0, 4));
			largePosition.insert(here + TilePosition(2, 0));
			largePosition.insert(here + TilePosition(2, 3));
		}
		else
		{
			smallPosition.insert(here + TilePosition(4, 0));
			smallPosition.insert(here + TilePosition(4, 2));
			smallPosition.insert(here + TilePosition(4, 4));
			largePosition.insert(here);
			largePosition.insert(here + TilePosition(0, 3));
		}
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		blocks[here] = Block(6, 8, here);
		smallPosition.insert(here + TilePosition(4, 1));
		smallPosition.insert(here + TilePosition(4, 4));
		mediumPosition.insert(here + TilePosition(0, 6));
		mediumPosition.insert(here + TilePosition(3, 6));
		largePosition.insert(here);
		largePosition.insert(here + TilePosition(0, 3));
	}
}

void BWEB::insertLargeBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
{

}

bool BWEB::overlapsBases(TilePosition here)
{
	for (auto &area : Map::Instance().Areas())
	{
		for (auto &base : area.Bases())
		{
			TilePosition tile = base.Location();
			if (here.x >= tile.x && here.x < tile.x + 4 && here.y >= tile.y && here.y < tile.y + 3) return true;
		}
	}
	return false;
}

bool BWEB::overlapsNeutrals(TilePosition here)
{
	for (auto &m : Map::Instance().Minerals())
	{
		TilePosition tile = m->TopLeft();		
		if (here.x >= tile.x && here.x < tile.x + 2 && here.y >= tile.y && here.y < tile.y + 1) return true;
	}

	for (auto &g : Map::Instance().Geysers())
	{
		TilePosition tile = g->TopLeft();
		if (here.x >= tile.x && here.x < tile.x + 2 && here.y >= tile.y && here.y < tile.y + 1) return true;
	}
	return false;
}

BWEB* BWEB::bInstance = nullptr;

BWEB & BWEB::Instance()
{
	if (bInstance) bInstance = new BWEB();

	return *bInstance;
}