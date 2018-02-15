#include "Block.h"

namespace BWEB
{
	void Map::findStartBlocks()
	{
		TilePosition tStart = Broodwar->self()->getStartLocation();
		Position pStart = Position(tStart) + Position(64, 48);
		bool h, v;
		h = v = false;

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
				if (dist < distBest && ((Broodwar->self()->getRace() == Races::Protoss && canAddBlock(tile, 8, 5, true)) || (Broodwar->self()->getRace() == Races::Terran && canAddBlock(tile, 6, 5, true))))
				{
					best = tile;
					distBest = dist;

					if (blockCenter.x < pStart.x) h = true;
					if (blockCenter.y < pStart.y) v = true;
				}


			}
		}
		insertStartBlock(best, h, v);

		distBest = 0.0;
		for (int x = tStart.x - 30; x <= tStart.x + 30; x++)
		{
			for (int y = tStart.y - 30; y <= tStart.y + 30; y++)
			{
				TilePosition tile = TilePosition(x, y);
				if (!tile.isValid() || BWEM::Map::Instance().GetArea(tile) != mainArea) continue;
				Position blockCenter = Position(tile) + Position(80, 64);
				double dist = blockCenter.getDistance(Position(firstChoke));
				if (dist > distBest && canAddBlock(tile, 5, 4, true))
				{
					best = tile;
					distBest = dist;
				}
			}
		}
		insertTechBlock(best, false, false);
	}

	void Map::findBlocks()
	{
		map<const BWEM::Area *, set<TilePosition>> tilesByArea;
		for (auto &area : BWEM::Map::Instance().Areas())
		{
			if (area.Bases().size() == 0) continue;
			for (int x = 0; x <= Broodwar->mapWidth(); x++)
			{
				for (int y = 0; y <= Broodwar->mapHeight(); y++)
				{
					if (!TilePosition(x, y).isValid()) continue;
					if (!BWEM::Map::Instance().GetArea(TilePosition(x, y))) continue;
					tilesByArea[BWEM::Map::Instance().GetArea(TilePosition(x, y))].insert(TilePosition(x, y));
				}
			}
		}

		for (auto area : tilesByArea)
		{
			set<TilePosition> tileSet = area.second;
			// Mirror check, normally production above and left
			bool mirrorHorizontal = false, mirrorVertical = false;
			if (BWEM::Map::Instance().Center().x > pStart.x) mirrorHorizontal = true;
			if (BWEM::Map::Instance().Center().y > pStart.y) mirrorVertical = true;
			int i = 2;

			if (Broodwar->self()->getRace() == Races::Protoss)
			{
				for (auto tile : tileSet)
					if (canAddBlock(tile, 10, 6, false) && i < 12)
					{
						i += 4;
						insertLargeBlock(tile, mirrorHorizontal, mirrorVertical);
					}
				for (auto tile : tileSet)
					if (canAddBlock(tile, 6, 8, false) && i < 12)
					{
						i += 2;
						insertMediumBlock(tile, mirrorHorizontal, mirrorVertical);
					}
				for (auto tile : tileSet)
					if (canAddBlock(tile, 4, 5, false) && i < 12)
					{
						i += 1;
						insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
					}
				for (auto tile : tileSet)
					if (canAddBlock(tile, 5, 2, false)) insertTinyBlock(tile, mirrorHorizontal, mirrorVertical);
			}
			else if (Broodwar->self()->getRace() == Races::Terran)
			{
				for (auto tile : tileSet)
					if (canAddBlock(tile, 6, 8, false)) insertMediumBlock(tile, mirrorHorizontal, mirrorVertical);
				for (auto tile : tileSet)
					if (canAddBlock(tile, 3, 4, false)) insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
			}
		}
	}

	bool Map::canAddBlock(TilePosition here, int width, int height, bool lowReq)
	{
		int offset = lowReq ? 0 : 1;
		// Check if a block of specified size would overlap any bases, resources or other blocks
		for (int x = here.x - offset; x < here.x + width + offset; x++)
		{
			for (int y = here.y - offset; y < here.y + height + offset; y++)
			{
				if (!TilePosition(x, y).isValid()) return false;
				if (!BWEM::Map::Instance().GetTile(TilePosition(x, y)).Buildable()) return false;
				if (BWEBUtil().overlapsAnything(TilePosition(x, y))) return false;
				if (here.getDistance(secondChoke) < 4) return false;
			}
		}
		return true;
	}

	void Map::insertTinyBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		Block newBlock(5, 4, here);
		newBlock.insertSmall(here);
		newBlock.insertMedium(here + TilePosition(2, 0));
		blocks.push_back(newBlock);
	}

	void Map::insertSmallBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			Block newBlock(4, 5, here);
			if (mirrorVertical)
			{
				newBlock.insertSmall(here);
				newBlock.insertSmall(here + TilePosition(2, 0));
				newBlock.insertLarge(here + TilePosition(0, 2));
			}
			else
			{
				newBlock.insertSmall(here + TilePosition(0, 3));
				newBlock.insertSmall(here + TilePosition(2, 3));
				newBlock.insertLarge(here);
			}
			blocks.push_back(newBlock);

		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			Block newBlock(3, 4, here);
			newBlock.insertMedium(here);
			newBlock.insertMedium(here + TilePosition(0, 2));
			blocks.push_back(newBlock);
		}
	}

	void Map::insertMediumBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			Block newBlock(6, 8, here);
			if (mirrorHorizontal)
			{
				if (mirrorVertical)
				{
					newBlock.insertSmall(here + TilePosition(0, 2));
					newBlock.insertSmall(here + TilePosition(0, 4));
					newBlock.insertSmall(here + TilePosition(0, 6));
					newBlock.insertMedium(here);
					newBlock.insertMedium(here + TilePosition(3, 0));
					newBlock.insertLarge(here + TilePosition(2, 2));
					newBlock.insertLarge(here + TilePosition(2, 5));
				}
				else
				{
					newBlock.insertSmall(here);
					newBlock.insertSmall(here + TilePosition(0, 2));
					newBlock.insertSmall(here + TilePosition(0, 4));
					newBlock.insertMedium(here + TilePosition(0, 6));
					newBlock.insertMedium(here + TilePosition(3, 6));
					newBlock.insertLarge(here + TilePosition(2, 0));
					newBlock.insertLarge(here + TilePosition(2, 3));
				}
			}
			else
			{
				if (mirrorVertical)
				{
					newBlock.insertSmall(here + TilePosition(4, 2));
					newBlock.insertSmall(here + TilePosition(4, 4));
					newBlock.insertSmall(here + TilePosition(4, 6));
					newBlock.insertMedium(here);
					newBlock.insertMedium(here + TilePosition(3, 0));
					newBlock.insertLarge(here + TilePosition(0, 2));
					newBlock.insertLarge(here + TilePosition(0, 5));
				}
				else
				{
					newBlock.insertSmall(here + TilePosition(4, 0));
					newBlock.insertSmall(here + TilePosition(4, 2));
					newBlock.insertSmall(here + TilePosition(4, 4));
					newBlock.insertMedium(here + TilePosition(0, 6));
					newBlock.insertMedium(here + TilePosition(3, 6));
					newBlock.insertLarge(here);
					newBlock.insertLarge(here + TilePosition(0, 3));
				}
			}
			blocks.push_back(newBlock);
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			Block newBlock(6, 8, here);
			newBlock.insertSmall(here + TilePosition(4, 1));
			newBlock.insertSmall(here + TilePosition(4, 4));
			newBlock.insertMedium(here + TilePosition(0, 6));
			newBlock.insertMedium(here + TilePosition(3, 6));
			newBlock.insertLarge(here);
			newBlock.insertLarge(here + TilePosition(0, 3));
			blocks.push_back(newBlock);
		}
	}

	void Map::insertLargeBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		Block newBlock(10, 6, here);
		newBlock.insertLarge(here);
		newBlock.insertLarge(here + TilePosition(0, 3));
		newBlock.insertLarge(here + TilePosition(6, 0));
		newBlock.insertLarge(here + TilePosition(6, 3));
		newBlock.insertSmall(here + TilePosition(4, 0));
		newBlock.insertSmall(here + TilePosition(4, 2));
		newBlock.insertSmall(here + TilePosition(4, 4));
		blocks.push_back(newBlock);
	}

	void Map::insertStartBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		// TODO -- mirroring
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			if (mirrorHorizontal)
			{
				if (mirrorVertical)
				{
					Block newBlock(8, 5, here);
					newBlock.insertLarge(here);
					newBlock.insertLarge(here + TilePosition(4, 0));
					newBlock.insertSmall(here + TilePosition(6, 3));
					newBlock.insertMedium(here + TilePosition(0, 3));
					newBlock.insertMedium(here + TilePosition(3, 3));
					blocks.push_back(newBlock);
				}
				else
				{
					Block newBlock(8, 5, here);
					newBlock.insertLarge(here + TilePosition(0, 2));
					newBlock.insertLarge(here + TilePosition(4, 2));
					newBlock.insertSmall(here + TilePosition(6, 0));
					newBlock.insertMedium(here + TilePosition(0, 0));
					newBlock.insertMedium(here + TilePosition(3, 0));
					blocks.push_back(newBlock);					
				}
			}
			else
			{
				if (mirrorVertical)
				{
					Block newBlock(8, 5, here);
					newBlock.insertLarge(here);
					newBlock.insertLarge(here + TilePosition(4, 0));
					newBlock.insertSmall(here + TilePosition(0, 3));
					newBlock.insertMedium(here + TilePosition(2, 3));
					newBlock.insertMedium(here + TilePosition(5, 3));
					blocks.push_back(newBlock);					
				}
				else
				{
					Block newBlock(8, 5, here);
					newBlock.insertLarge(here + TilePosition(0, 2));
					newBlock.insertLarge(here + TilePosition(4, 2));
					newBlock.insertSmall(here + TilePosition(0, 0));
					newBlock.insertMedium(here + TilePosition(2, 0));
					newBlock.insertMedium(here + TilePosition(5, 0));
					blocks.push_back(newBlock);					
				}
			}
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			Block newBlock(6, 5, here);
			newBlock.insertLarge(here);
			newBlock.insertSmall(here + TilePosition(4, 1));
			newBlock.insertMedium(here + TilePosition(0, 3));
			newBlock.insertMedium(here + TilePosition(3, 3));
			blocks.push_back(newBlock);
		}
	}

	void Map::insertTechBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			Block newBlock(5, 4, here);
			newBlock.insertSmall(here);
			newBlock.insertSmall(here + TilePosition(0, 2));
			newBlock.insertMedium(here + TilePosition(2, 0));
			newBlock.insertMedium(here + TilePosition(2, 2));
			blocks.push_back(newBlock);
		}
	}
}