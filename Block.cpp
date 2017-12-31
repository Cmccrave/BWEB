#include "Block.h"

namespace BWEB
{
	void Map::findBlocks()
	{
		TilePosition tStart = Broodwar->self()->getStartLocation();
		Position pStart = Position(tStart) + Position(64, 48);
		set<TilePosition> mainTiles;
		mainArea = BWEM::Map::Instance().GetArea(tStart);		

		for (int x = 0; x <= Broodwar->mapWidth(); x++)
		{
			for (int y = 0; y <= Broodwar->mapHeight(); y++)
			{
				if (!TilePosition(x, y).isValid()) continue;
				if (!BWEM::Map::Instance().GetArea(TilePosition(x, y))) continue;
				if (BWEM::Map::Instance().GetArea(TilePosition(x, y)) == mainArea) mainTiles.insert(TilePosition(x, y));
			}
		}

		TilePosition best;
		double distBest = DBL_MAX;
		for (auto tile : mainTiles)
		{
			if (!tile.isValid()) continue;
			Position blockCenter = Position(tile) + Position(128, 80);
			double dist = pow(blockCenter.getDistance(pStart), 2.0) + blockCenter.getDistance(Position(firstChoke));
			if (dist < distBest && canAddBlock(tile, 8, 5, false))
			{
				best = tile;
				distBest = dist;
			}
		}
		insertStartBlock(best, false, false);

		// Mirror check, normally production above and left
		bool mirrorHorizontal = false, mirrorVertical = false;
		if (BWEM::Map::Instance().Center().x > pStart.x) mirrorHorizontal = true;
		if (BWEM::Map::Instance().Center().y > pStart.y) mirrorVertical = true;

		for (auto &area : BWEM::Map::Instance().Areas())
		{
			for (auto &base : area.Bases())
			{
				TilePosition center;
				int cnt = 0;
				for (auto &mineral : base.Minerals())
					center += mineral->TopLeft(), cnt++;

				for (auto &gas : base.Geysers())
					center += gas->TopLeft(), cnt++;

				if (cnt > 0) center = center / cnt;
				resourceCenter.insert(center);
			}
		}

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
		// Check if a block of specified size would overlap any bases, resources or other blocks
		for (int x = here.x - 1; x < here.x + width + 1; x++)
		{
			for (int y = here.y - 1; y < here.y + height + 1; y++)
			{
				if (!TilePosition(x, y).isValid()) return false;
				if (!BWEM::Map::Instance().GetTile(TilePosition(x, y)).Buildable()) return false;
				if (BWEBUtil().overlapsBlocks(TilePosition(x, y))) return false;
				if (BWEBUtil().overlapsBases(TilePosition(x, y)) && !baseBlock) return false;
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
			blocks[here] = Block(4, 5, here);
			if (mirrorVertical)
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

	void Map::insertMediumBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			blocks[here] = Block(6, 8, here);
			if (mirrorHorizontal)
			{
				if (mirrorVertical)
				{
					smallPosition.insert(here + TilePosition(0, 2));
					smallPosition.insert(here + TilePosition(0, 4));
					smallPosition.insert(here + TilePosition(0, 6));
					mediumPosition.insert(here);
					mediumPosition.insert(here + TilePosition(3, 0));
					largePosition.insert(here + TilePosition(2, 2));
					largePosition.insert(here + TilePosition(2, 5));
				}
				else
				{
					smallPosition.insert(here);
					smallPosition.insert(here + TilePosition(0, 2));
					smallPosition.insert(here + TilePosition(0, 4));
					mediumPosition.insert(here + TilePosition(0, 6));
					mediumPosition.insert(here + TilePosition(3, 6));
					largePosition.insert(here + TilePosition(2, 0));
					largePosition.insert(here + TilePosition(2, 3));
				}
			}
			else
			{
				if (mirrorVertical)
				{
					smallPosition.insert(here + TilePosition(4, 2));
					smallPosition.insert(here + TilePosition(4, 4));
					smallPosition.insert(here + TilePosition(4, 6));
					mediumPosition.insert(here);
					mediumPosition.insert(here + TilePosition(3, 0));
					largePosition.insert(here + TilePosition(0, 2));
					largePosition.insert(here + TilePosition(0, 5));
				}
				else
				{
					smallPosition.insert(here + TilePosition(4, 0));
					smallPosition.insert(here + TilePosition(4, 2));
					smallPosition.insert(here + TilePosition(4, 4));
					mediumPosition.insert(here + TilePosition(0, 6));
					mediumPosition.insert(here + TilePosition(3, 6));
					largePosition.insert(here);
					largePosition.insert(here + TilePosition(0, 3));
				}
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

	void Map::insertLargeBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		blocks[here] = Block(12, 6, here);
		largePosition.insert(here);
		largePosition.insert(here + TilePosition(0, 3));
		largePosition.insert(here + TilePosition(8, 0));
		largePosition.insert(here + TilePosition(8, 3));
		smallPosition.insert(here + TilePosition(4, 0));
		smallPosition.insert(here + TilePosition(4, 2));
		smallPosition.insert(here + TilePosition(4, 4));
		smallPosition.insert(here + TilePosition(6, 0));
		smallPosition.insert(here + TilePosition(6, 2));
		smallPosition.insert(here + TilePosition(6, 4));
	}

	void Map::insertStartBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		// TODO -- mirror based on gas position	
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			blocks[here] = Block(8, 5, here);
			largePosition.insert(here);
			largePosition.insert(here + TilePosition(4, 0));
			smallPosition.insert(here + TilePosition(0, 3));
			mediumPosition.insert(here + TilePosition(2, 3));
			mediumPosition.insert(here + TilePosition(5, 3));
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			blocks[here] = Block(8, 5, here);
		}
	}
}