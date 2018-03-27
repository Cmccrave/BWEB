#include "Block.h"
#include <chrono>

namespace BWEB
{
	void Map::findStartBlock()
	{
		bool h, v;
		h = v = false;

		TilePosition tileBest;
		double distBest = DBL_MAX;
		for (int x = mainTile.x - 8; x <= mainTile.x + 5; x++)
		{
			for (int y = mainTile.y - 5; y <= mainTile.y + 4; y++)
			{
				TilePosition tile = TilePosition(x, y);
				if (!tile.isValid()) continue;
				Position blockCenter = Position(tile) + Position(128, 80);
				double dist = blockCenter.getDistance(mainPosition) + blockCenter.getDistance(Position(mainChoke->Center()));
				if (dist < distBest && ((Broodwar->self()->getRace() == Races::Protoss && canAddBlock(tile, 8, 5, true)) || (Broodwar->self()->getRace() == Races::Terran && canAddBlock(tile, 6, 5, true))))
				{
					tileBest = tile;
					distBest = dist;

					if (blockCenter.x < mainPosition.x) h = true;
					if (blockCenter.y < mainPosition.y) v = true;
				}


			}
		}

		if (tileBest.isValid())
			insertStartBlock(tileBest, h, v);
	}

	void Map::findHiddenTechBlock()
	{
		double distBest = 0.0;
		TilePosition best;
		for (int x = mainTile.x - 30; x <= mainTile.x + 30; x++)
		{
			for (int y = mainTile.y - 30; y <= mainTile.y + 30; y++)
			{
				TilePosition tile = TilePosition(x, y);
				if (!tile.isValid() || BWEM::Map::Instance().GetArea(tile) != mainArea) continue;
				Position blockCenter = Position(tile) + Position(80, 64);
				double dist = blockCenter.getDistance(Position(mainChoke->Center()));
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
		chrono::steady_clock::time_point start;
		start = chrono::high_resolution_clock::now();
		findStartBlock();
		map<const BWEM::Area *, int> typePerArea;

		for (int x = 0; x <= Broodwar->mapWidth(); x++)
		{
			for (int y = 0; y <= Broodwar->mapHeight(); y++)
			{
				TilePosition tile(x, y);
				if (!TilePosition(x, y).isValid()) continue;

				const BWEM::Area * area = BWEM::Map::Instance().GetArea(tile);
				if (!area || area->Bases().empty()) continue;

				bool mirrorHorizontal = false, mirrorVertical = false;
				if (BWEM::Map::Instance().Center().x > mainPosition.x) mirrorHorizontal = true;
				if (BWEM::Map::Instance().Center().y > mainPosition.y) mirrorVertical = true;

				if (Broodwar->self()->getRace() == Races::Protoss)
				{
					if (canAddBlock(tile, 10, 6, false) && typePerArea[area] < 12)
					{
						x+=10;
						typePerArea[area] += 4;
						insertLargeBlock(tile, mirrorHorizontal, mirrorVertical);
					}
					else if (canAddBlock(tile, 6, 8, false) && typePerArea[area] < 12)
					{
						x+=6;
						typePerArea[area] += 2;
						insertMediumBlock(tile, mirrorHorizontal, mirrorVertical);
					}
					else if(canAddBlock(tile, 4, 5, false) && typePerArea[area] < 12)
					{
						typePerArea[area] += 1;
						insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
					}
					else if(canAddBlock(tile, 5, 2, false))
						insertTinyBlock(tile, mirrorHorizontal, mirrorVertical);
				}
				else if (Broodwar->self()->getRace() == Races::Terran)
				{
					if (canAddBlock(tile, 6, 6, false))					
						insertMediumBlock(tile, mirrorHorizontal, mirrorVertical);

					else if(canAddBlock(tile, 6, 4, false) && typePerArea[area] < 12)
					{
						typePerArea[area] += 4;
						insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
					}
					else if(canAddBlock(tile, 3, 2, false) && typePerArea[area] < 12)
					{
						typePerArea[area] += 1;
						insertTinyBlock(tile, mirrorHorizontal, mirrorVertical);
					}
				}
			}
		}

		double dur = std::chrono::duration <double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
		Broodwar << "Block time:" << dur << endl;
	}

	bool Map::canAddBlock(TilePosition here, int width, int height, bool lowReq)
	{
		// Check 4 corners before checking the rest
		TilePosition one(here.x, here.y);
		TilePosition two(here.x + width - 1, here.y);
		TilePosition three(here.x, here.y + height - 1);
		TilePosition four(here.x + width - 1, here.y + height - 1);

		if (!one.isValid() || !two.isValid() || !three.isValid() || !four.isValid()) return false;
		if (!BWEM::Map::Instance().GetTile(one).Buildable() || overlapsAnything(one)) return false;
		if (!BWEM::Map::Instance().GetTile(two).Buildable() || overlapsAnything(two)) return false;
		if (!BWEM::Map::Instance().GetTile(three).Buildable() || overlapsAnything(three)) return false;
		if (!BWEM::Map::Instance().GetTile(four).Buildable() || overlapsAnything(four)) return false;

		int offset = lowReq ? 0 : 1;
		// Check if a block of specified size would overlap any bases, resources or other blocks
		for (int x = here.x - offset; x < here.x + width + offset; x++)
		{
			for (int y = here.y - offset; y < here.y + height + offset; y++)
			{
				TilePosition tile(x, y);
				if (tile == one || tile == two || tile == three || tile == four) continue;
				if (!tile.isValid()) return false;
				if (!BWEM::Map::Instance().GetTile(TilePosition(x, y)).Buildable()) return false;
				if (overlapsAnything(TilePosition(x, y))) return false;
			}
		}
		return true;
	}

	void Map::insertTinyBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			Block newBlock(5, 4, here);
			addOverlap(here, 5, 4);
			newBlock.insertSmall(here);
			newBlock.insertMedium(here + TilePosition(2, 0));
			blocks.push_back(newBlock);
		}
		else if (Broodwar->self()->getRace() == Races::Terran)
		{
			Block newBlock(3, 2, here);
			addOverlap(here, 3, 2);
			newBlock.insertMedium(here);
			blocks.push_back(newBlock);
		}
	}

	void Map::insertSmallBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			Block newBlock(4, 5, here);
			addOverlap(here, 4, 5);
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
			Block newBlock(6, 4, here);
			addOverlap(here, 6, 4);
			newBlock.insertMedium(here);
			newBlock.insertMedium(here + TilePosition(0, 2));
			newBlock.insertMedium(here + TilePosition(3, 0));
			newBlock.insertMedium(here + TilePosition(3, 2));
			blocks.push_back(newBlock);
		}
	}

	void Map::insertMediumBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (Broodwar->self()->getRace() == Races::Protoss)
		{
			Block newBlock(6, 8, here);
			addOverlap(here, 6, 8);
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
			Block newBlock(6, 6, here);
			addOverlap(here, 6, 6);
			newBlock.insertSmall(here + TilePosition(4, 1));
			newBlock.insertSmall(here + TilePosition(4, 4));
			newBlock.insertLarge(here);
			newBlock.insertLarge(here + TilePosition(0, 3));
			blocks.push_back(newBlock);
		}
	}

	void Map::insertLargeBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		Block newBlock(10, 6, here);
		addOverlap(here, 10, 6);
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
					addOverlap(here, 8, 5);
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
					addOverlap(here, 8, 5);
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
					addOverlap(here, 8, 5);
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
					addOverlap(here, 8, 5);
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
			addOverlap(here, 6, 5);
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
			addOverlap(here, 5, 4);
			newBlock.insertSmall(here);
			newBlock.insertSmall(here + TilePosition(0, 2));
			newBlock.insertMedium(here + TilePosition(2, 0));
			newBlock.insertMedium(here + TilePosition(2, 2));
			blocks.push_back(newBlock);
		}
	}

	void Map::eraseBlock(TilePosition here)
	{
		for (auto it = blocks.begin(); it != blocks.end(); it++)
		{
			auto&  block = *it;
			if (here.x >= block.Location().x && here.x < block.Location().x + block.width() && here.y >= block.Location().y && here.y < block.Location().y + block.height())
			{
				blocks.erase(it);
				// Remove overlap
				return;
			}
		}
	}

	const Block* Map::getClosestBlock(TilePosition here) const
	{
		double distBest = DBL_MAX;
		const Block* bestBlock = nullptr;
		for (auto& block : blocks)
		{
			TilePosition tile = block.Location() + TilePosition(block.width() / 2, block.height() / 2);
			double dist = here.getDistance(tile);

			if (dist < distBest)
			{
				distBest = dist;
				bestBlock = &block;
			}
		}
		return bestBlock;
	}

	Block::Block(int width, int height, TilePosition tile)
	{ 
		w = width, h = height, t = tile;
	}

}