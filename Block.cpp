#include "Block.h"

namespace BWEB
{
	void Map::findStartBlocks()
	{
		TilePosition tStart = Broodwar->self()->getStartLocation();
		Position pStart = Position(tStart) + Position(64, 48);

		TilePosition best;
		double distBest = DBL_MAX;
		for (int x = tStart.x - 8; x <= tStart.x + 5; x++)
		{
			for (int y = tStart.y - 5; y <= tStart.y + 4; y++)
			{
				TilePosition tile = TilePosition(x, y);
				if (!tile.isValid()) continue;
				Position blockCenter = Position(tile) + Position(128, 80);
				double dist = blockCenter.getDistance(pStart) + blockCenter.getDistance(Position(firstChoke));
				if (dist < distBest && canAddBlock(tile, 8, 5, true))
				{
					best = tile;
					distBest = dist;
				}
			}
		}
		insertStartBlock(best, false, false);
	}

	void Map::findBlocks()
	{		
		set<TilePosition> mainTiles;
		for (int x = 0; x <= Broodwar->mapWidth(); x++)
		{
			for (int y = 0; y <= Broodwar->mapHeight(); y++)
			{
				if (!TilePosition(x, y).isValid()) continue;
				if (!BWEM::Map::Instance().GetArea(TilePosition(x, y))) continue;
				if (BWEM::Map::Instance().GetArea(TilePosition(x, y)) == mainArea) mainTiles.insert(TilePosition(x, y));
			}
		}

		// Mirror check, normally production above and left
		bool mirrorHorizontal = false, mirrorVertical = false;
		if (BWEM::Map::Instance().Center().x > pStart.x) mirrorHorizontal = true;
		if (BWEM::Map::Instance().Center().y > pStart.y) mirrorVertical = true;

		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			for (auto tile : mainTiles)
				if (canAddBlock(tile, 12, 6, false)) insertLargeBlock(tile, mirrorHorizontal, mirrorVertical);
			for (auto tile : mainTiles)
				if (canAddBlock(tile, 6, 8, false)) insertMediumBlock(tile, mirrorHorizontal, mirrorVertical);
			for (auto tile : mainTiles)
				if (canAddBlock(tile, 4, 5, false)) insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			for (auto tile : mainTiles)
				if (canAddBlock(tile, 6, 8, false)) insertMediumBlock(tile, mirrorHorizontal, mirrorVertical);
			for (auto tile : mainTiles)
				if (canAddBlock(tile, 3, 4, false)) insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
		}
	}

	bool Map::canAddBlock(TilePosition here, int width, int height, bool baseBlock)
	{
		int offset = baseBlock ? 0 : 1;
		// Check if a block of specified size would overlap any bases, resources or other blocks
		for (int x = here.x - offset; x < here.x + width + offset; x++)
		{
			for (int y = here.y - offset; y < here.y + height + offset; y++)
			{
				if (!TilePosition(x, y).isValid()) return false;
				if (!BWEM::Map::Instance().GetTile(TilePosition(x, y)).Buildable()) return false;
				if (BWEBUtil().overlapsBlocks(TilePosition(x, y))) return false;
				if (BWEBUtil().overlapsBases(TilePosition(x, y))) return false;
				if (BWEBUtil().overlapsNeutrals(TilePosition(x, y))) return false;
				if (BWEBUtil().overlapsMining(TilePosition(x, y))) return false;
				if (BWEBUtil().overlapsWalls(TilePosition(x, y))) return false;
			}
		}
		return true;
	}

	void Map::insertSmallBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			prodBlocks[here] = Block(4, 5, here);
			if (mirrorVertical)
			{
				prodBlocks[here].insertPylon(here);
				prodBlocks[here].insertPylon(here + TilePosition(2, 0));
				prodBlocks[here].insertLarge(here + TilePosition(0, 2));
			}
			else
			{
				prodBlocks[here].insertPylon(here + TilePosition(0, 3));
				prodBlocks[here].insertPylon(here + TilePosition(2, 3));
				prodBlocks[here].insertLarge(here);
			}
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			prodBlocks[here] = Block(3, 4, here);
			prodBlocks[here].insertMedium(here);
			prodBlocks[here].insertMedium(here + TilePosition(0, 2));
		}
	}

	void Map::insertMediumBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			prodBlocks[here] = Block(6, 8, here);
			if (mirrorHorizontal)
			{
				if (mirrorVertical)
				{
					prodBlocks[here].insertPylon(here + TilePosition(0, 2));
					prodBlocks[here].insertPylon(here + TilePosition(0, 4));
					prodBlocks[here].insertPylon(here + TilePosition(0, 6));
					prodBlocks[here].insertMedium(here);
					prodBlocks[here].insertMedium(here + TilePosition(3, 0));
					prodBlocks[here].insertLarge(here + TilePosition(2, 2));
					prodBlocks[here].insertLarge(here + TilePosition(2, 5));
				}
				else
				{
					prodBlocks[here].insertPylon(here);
					prodBlocks[here].insertPylon(here + TilePosition(0, 2));
					prodBlocks[here].insertPylon(here + TilePosition(0, 4));
					prodBlocks[here].insertMedium(here + TilePosition(0, 6));
					prodBlocks[here].insertMedium(here + TilePosition(3, 6));
					prodBlocks[here].insertLarge(here + TilePosition(2, 0));
					prodBlocks[here].insertLarge(here + TilePosition(2, 3));
				}
			}
			else
			{
				if (mirrorVertical)
				{
					prodBlocks[here].insertPylon(here + TilePosition(4, 2));
					prodBlocks[here].insertPylon(here + TilePosition(4, 4));
					prodBlocks[here].insertPylon(here + TilePosition(4, 6));
					prodBlocks[here].insertMedium(here);
					prodBlocks[here].insertMedium(here + TilePosition(3, 0));
					prodBlocks[here].insertLarge(here + TilePosition(0, 2));
					prodBlocks[here].insertLarge(here + TilePosition(0, 5));
				}
				else
				{
					prodBlocks[here].insertPylon(here + TilePosition(4, 0));
					prodBlocks[here].insertPylon(here + TilePosition(4, 2));
					prodBlocks[here].insertPylon(here + TilePosition(4, 4));
					prodBlocks[here].insertMedium(here + TilePosition(0, 6));
					prodBlocks[here].insertMedium(here + TilePosition(3, 6));
					prodBlocks[here].insertLarge(here);
					prodBlocks[here].insertLarge(here + TilePosition(0, 3));
				}
			}
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			prodBlocks[here] = Block(6, 8, here);
			prodBlocks[here].insertSmall(here + TilePosition(4, 1));
			prodBlocks[here].insertSmall(here + TilePosition(4, 4));
			prodBlocks[here].insertMedium(here + TilePosition(0, 6));
			prodBlocks[here].insertMedium(here + TilePosition(3, 6));
			prodBlocks[here].insertLarge(here);
			prodBlocks[here].insertLarge(here + TilePosition(0, 3));
		}
	}

	void Map::insertLargeBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		prodBlocks[here] = Block(12, 6, here);
		prodBlocks[here].insertLarge(here);
		prodBlocks[here].insertLarge(here + TilePosition(0, 3));
		prodBlocks[here].insertLarge(here + TilePosition(8, 0));
		prodBlocks[here].insertLarge(here + TilePosition(8, 3));
		prodBlocks[here].insertPylon(here + TilePosition(4, 0));
		prodBlocks[here].insertPylon(here + TilePosition(4, 2));
		prodBlocks[here].insertPylon(here + TilePosition(4, 4));
		prodBlocks[here].insertPylon(here + TilePosition(6, 0));
		prodBlocks[here].insertPylon(here + TilePosition(6, 2));
		prodBlocks[here].insertPylon(here + TilePosition(6, 4));
	}

	void Map::insertStartBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		// TODO -- mirror based on gas position	
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			prodBlocks[here] = Block(8, 5, here);
			prodBlocks[here].insertLarge(here);
			prodBlocks[here].insertLarge(here + TilePosition(4, 0));
			prodBlocks[here].insertPylon(here + TilePosition(0, 3));
			prodBlocks[here].insertMedium(here + TilePosition(2, 3));
			prodBlocks[here].insertMedium(here + TilePosition(5, 3));
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			prodBlocks[here] = Block(8, 5, here);
		}
	}
}