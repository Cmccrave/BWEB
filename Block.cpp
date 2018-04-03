#include "Block.h"

namespace BWEB
{
	void Map::findStartBlock()
	{
		findStartBlock(Broodwar->self());
	}
	void Map::findStartBlock(BWAPI::Player player)
	{
		findStartBlock(player->getRace());
	}
	void Map::findStartBlock(BWAPI::Race race)
	{
		bool v;
		auto h = (v = false);

		TilePosition tileBest;
		auto distBest = DBL_MAX;
		for (auto x = mainTile.x - 8; x <= mainTile.x + 5; x++)
		{
			for (auto y = mainTile.y - 5; y <= mainTile.y + 4; y++)
			{
				auto tile = TilePosition(x, y);
				if (!tile.isValid()) continue;
				auto blockCenter = Position(tile) + Position(128, 80);
				const auto dist = blockCenter.getDistance(mainPosition) + blockCenter.getDistance(Position(mainChoke->Center()));
				if (dist < distBest && ((race == Races::Protoss && canAddBlock(tile, 8, 5, true)) || (race == Races::Terran && canAddBlock(tile, 6, 5, true))))
				{
					tileBest = tile;
					distBest = dist;

					if (blockCenter.x < mainPosition.x) h = true;
					if (blockCenter.y < mainPosition.y) v = true;
				}
			}
		}

		if (tileBest.isValid())
			insertStartBlock(race, tileBest, h, v);
	}

	void Map::findHiddenTechBlock()
	{
		findHiddenTechBlock(Broodwar->self());
	}
	void Map::findHiddenTechBlock(BWAPI::Player player)
	{
		findHiddenTechBlock(player->getRace());
	}
	void Map::findHiddenTechBlock(BWAPI::Race race)
	{
		auto distBest = 0.0;
		TilePosition best;
		for (auto x = mainTile.x - 30; x <= mainTile.x + 30; x++)
		{
			for (auto y = mainTile.y - 30; y <= mainTile.y + 30; y++)
			{
				auto tile = TilePosition(x, y);
				if (!tile.isValid() || BWEM::Map::Instance().GetArea(tile) != mainArea) continue;
				auto blockCenter = Position(tile) + Position(80, 64);
				const auto dist = blockCenter.getDistance(Position(mainChoke->Center()));
				if (dist > distBest && canAddBlock(tile, 5, 4, true))
				{
					best = tile;
					distBest = dist;
				}
			}
		}
		insertTechBlock(race, best, false, false);
	}

	void Map::findBlocks()
	{
		findBlocks(Broodwar->self());
	}
	void Map::findBlocks(BWAPI::Player player)
	{
		findBlocks(player->getRace());
	}
	void Map::findBlocks(BWAPI::Race race)
	{
		findStartBlock(race);
		map<const BWEM::Area *, int> typePerArea;

		// Iterate every tile
		for (auto y = 0; y <= Broodwar->mapHeight(); y++)
		{
			for (auto x = 0; x <= Broodwar->mapWidth(); x++)
			{
				TilePosition tile(x, y);
				if (!tile.isValid()) continue;

				auto area = BWEM::Map::Instance().GetArea(tile);
				if (!area) continue;

				// Check if we should mirror our blocks - TODO: Improve the decisions for these
				auto mirrorHorizontal = false, mirrorVertical = false;
				if (BWEM::Map::Instance().Center().x > mainPosition.x) mirrorHorizontal = true;
				if (BWEM::Map::Instance().Center().y > mainPosition.y) mirrorVertical = true;

				if (race == Races::Protoss)
				{
					if (canAddBlock(tile, 10, 6, false) && typePerArea[area] < 12)
					{
						typePerArea[area] += 4;
						insertLargeBlock(race, tile, mirrorHorizontal, mirrorVertical);
						x += 9;
					}
					else if (canAddBlock(tile, 6, 8, false) && typePerArea[area] < 12)
					{
						typePerArea[area] += 2;
						insertMediumBlock(race, tile, mirrorHorizontal, mirrorVertical);
						x += 5;
					}
					else if (canAddBlock(tile, 4, 5, false))
					{
						typePerArea[area] += 1;
						insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
						x += 3;
					}
					else if (canAddBlock(tile, 5, 2, false))
					{
						insertTinyBlock(tile, mirrorHorizontal, mirrorVertical);
						x += 4;
					}
				}
				else if (race == Races::Terran)
				{
					if (canAddBlock(tile, 8, 6, false))
					{
						x += 7;
						insertLargeBlock(race, tile, mirrorHorizontal, mirrorVertical);
					}

					else if (canAddBlock(tile, 7, 6, false))
					{
						x += 6;
						insertMediumBlock(race, tile, mirrorHorizontal, mirrorVertical);
					}

					else if (canAddBlock(tile, 6, 6, false))
					{
						x += 5;
						insertSmallBlock(tile, mirrorHorizontal, mirrorVertical);
					}
					else if (canAddBlock(tile, 6, 4, false))
					{
						x += 5;
						insertTinyBlock(tile, mirrorHorizontal, mirrorVertical);
					}
				}
			}
		}
	}

	bool Map::canAddBlock(const TilePosition here, const int width, const int height, const bool lowReq)
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

		const auto offset = lowReq ? 0 : 1;
		// Check if a block of specified size would overlap any bases, resources or other blocks
		for (auto x = here.x - offset; x < here.x + width + offset; x++)
		{
			for (auto y = here.y - offset; y < here.y + height + offset; y++)
			{
				TilePosition tile(x, y);
				if (tile == one || tile == two || tile == three || tile == four) continue;
				if (!tile.isValid()) return false;
				if (!BWEM::Map::Instance().GetTile(TilePosition(x, y)).Buildable()) return false;
				if (overlapGrid[x][y] > 0) return false;
			}
		}
		return true;
	}

	void Map::insertTinyBlock(const TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		insertTinyBlock(Broodwar->self(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertTinyBlock(BWAPI::Player player, const TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		insertTinyBlock(player->getRace(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertTinyBlock(BWAPI::Race race, const TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (race == Races::Protoss)
		{
			Block newBlock(5, 2, here);
			addOverlap(here, 5, 2);
			newBlock.insertSmall(here);
			newBlock.insertMedium(here + TilePosition(2, 0));
			blocks.push_back(newBlock);
		}
		else if (race == Races::Terran)
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

	void Map::insertSmallBlock(const TilePosition here, bool mirrorHorizontal, const bool mirrorVertical)
	{
		insertSmallBlock(Broodwar->self(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertSmallBlock(BWAPI::Player player, const TilePosition here, bool mirrorHorizontal, const bool mirrorVertical)
	{
		insertSmallBlock(player->getRace(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertSmallBlock(BWAPI::Race race, const TilePosition here, bool mirrorHorizontal, const bool mirrorVertical)
	{
		if (race == Races::Protoss)
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
		else if (race == Races::Terran)
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

	void Map::insertMediumBlock(const TilePosition here, const bool mirrorHorizontal, const bool mirrorVertical)
	{
		insertMediumBlock(Broodwar->self(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertMediumBlock(BWAPI::Player player, const TilePosition here, const bool mirrorHorizontal, const bool mirrorVertical)
	{
		insertMediumBlock(player->getRace(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertMediumBlock(BWAPI::Race race, const TilePosition here, const bool mirrorHorizontal, const bool mirrorVertical)
	{
		if (race == Races::Protoss)
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
		else if (race == Races::Terran)
		{
			Block newBlock(7, 6, here);
			addOverlap(here, 7, 6);
			newBlock.insertMedium(here);
			newBlock.insertMedium(here + TilePosition(0, 2));
			newBlock.insertMedium(here + TilePosition(0, 4));
			newBlock.insertLarge(here + TilePosition(3, 0));
			newBlock.insertLarge(here + TilePosition(3, 3));
			blocks.push_back(newBlock);
		}
	}

	void Map::insertLargeBlock(const TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		insertLargeBlock(Broodwar->self(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertLargeBlock(BWAPI::Player player, const TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		insertLargeBlock(player->getRace(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertLargeBlock(BWAPI::Race race, const TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (race == Races::Protoss)
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
		else if (race == Races::Terran)
		{
			Block newBlock(8, 6, here);
			addOverlap(here, 8, 6);
			newBlock.insertLarge(here);
			newBlock.insertLarge(here + TilePosition(0, 3));
			newBlock.insertLarge(here + TilePosition(4, 0));
			newBlock.insertLarge(here + TilePosition(4, 3));
			blocks.push_back(newBlock);
		}
	}

	void Map::insertStartBlock(const TilePosition here, const bool mirrorHorizontal, const bool mirrorVertical)
	{
		insertStartBlock(Broodwar->self(), here, mirrorHorizontal, mirrorVertical);
	}

	void Map::insertStartBlock(BWAPI::Player player, const TilePosition here, const bool mirrorHorizontal, const bool mirrorVertical)
	{
		insertStartBlock(player->getRace(), here, mirrorHorizontal, mirrorVertical);
	}

	void Map::insertStartBlock(BWAPI::Race race, const TilePosition here, const bool mirrorHorizontal, const bool mirrorVertical)
	{
		// TODO -- mirroring
		if (race == Races::Protoss)
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
		else if (race == Races::Terran)
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
		insertTechBlock(Broodwar->self(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertTechBlock(BWAPI::Player player, TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		insertTechBlock(player->getRace(), here, mirrorHorizontal, mirrorVertical);
	}
	void Map::insertTechBlock(BWAPI::Race race, TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		if (race == Races::Protoss)
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

	void Map::eraseBlock(const TilePosition here)
	{
		for (auto it = blocks.begin(); it != blocks.end(); ++it)
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
			const auto tile = block.Location() + TilePosition(block.width() / 2, block.height() / 2);
			const auto dist = here.getDistance(tile);

			if (dist < distBest)
			{
				distBest = dist;
				bestBlock = &block;
			}
		}
		return bestBlock;
	}

	Block::Block(const int width, const int height, const TilePosition tile)
	{
		w = width, h = height, t = tile;
	}
}