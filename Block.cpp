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
				if (!tile.isValid() || mapBWEM.GetArea(tile) != mainArea) continue;
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
		vector<int> heights;
		vector<int> widths;

		if (race == Races::Protoss) {
			heights.insert(heights.end(), { 2, 5, 6, 8 });
			widths.insert(widths.end(), { 2, 4, 5, 8, 10 });
		}
		else if (race == Races::Terran) {
			heights.insert(heights.end(), { 2, 4, 5, 6 });
			widths.insert(widths.end(), { 3, 6, 10 });
		}

		// Iterate every tile
		for (int i = 10; i > 0; i--) {
			for (int j = 10; j > 0; j--) {
				for (auto y = 0; y <= Broodwar->mapHeight(); y++) {
					for (auto x = 0; x <= Broodwar->mapWidth(); x++) {
						if (find(heights.begin(), heights.end(), j) == heights.end() || find(widths.begin(), widths.end(), i) == widths.end()) continue;

						TilePosition tile(x, y);
						if (!tile.isValid()) continue;

						auto area = mapBWEM.GetArea(tile);
						if (!area) continue;

						if (canAddBlock(tile, i, j, false)) {
							insertBlock(race, tile, i, j);
							x += i+1;
						}
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
		if (!mapBWEM.GetTile(one).Buildable() || overlapsAnything(one)) return false;
		if (!mapBWEM.GetTile(two).Buildable() || overlapsAnything(two)) return false;
		if (!mapBWEM.GetTile(three).Buildable() || overlapsAnything(three)) return false;
		if (!mapBWEM.GetTile(four).Buildable() || overlapsAnything(four)) return false;

		const auto offset = lowReq ? 0 : 1;
		// Check if a block of specified size would overlap any bases, resources or other blocks
		for (auto x = here.x - offset; x < here.x + width + offset; x++)
		{
			for (auto y = here.y - offset; y < here.y + height + offset; y++)
			{
				TilePosition tile(x, y);
				if (tile == one || tile == two || tile == three || tile == four) continue;
				if (!tile.isValid()) return false;
				if (!mapBWEM.GetTile(TilePosition(x, y)).Buildable()) return false;
				if (overlapGrid[x][y] > 0 || overlapsMining(tile)) return false;
			}
		}
		return true;
	}

	void Map::insertBlock(BWAPI::Race race, TilePosition here, int width, int height)
	{
		Block newBlock(width, height, here);
		if (race == Races::Protoss)
		{
			if (height == 2) {
				// Just a pylon
				if (width == 2) {
					newBlock.insertSmall(here);
				}
				// Pylon and medium
				else if (width == 5) {
					newBlock.insertSmall(here);
					newBlock.insertMedium(here + TilePosition(2, 0));
				}
				else return;
			}

			else if (height == 5) {
				// Gate and 2 Pylons
				if (width == 4) {
					newBlock.insertLarge(here);
					newBlock.insertSmall(here + TilePosition(0, 3));
					newBlock.insertSmall(here + TilePosition(2, 3));
				}
				else return;
			}

			else  if (height == 8) {
				// 4 Gates and 4 Pylons
				if (width == 8) {
					newBlock.insertSmall(here + TilePosition(0, 3));
					newBlock.insertSmall(here + TilePosition(2, 3));
					newBlock.insertSmall(here + TilePosition(4, 3));
					newBlock.insertSmall(here + TilePosition(6, 3));
					newBlock.insertLarge(here);
					newBlock.insertLarge(here + TilePosition(4, 0));
					newBlock.insertLarge(here + TilePosition(0, 5));
					newBlock.insertLarge(here + TilePosition(4, 5));
				}
				else return;
			}

			else if (height == 6) {
				// 4 Gates and 3 Pylons
				if (width == 10) {
					newBlock.insertSmall(here + TilePosition(4, 0));
					newBlock.insertSmall(here + TilePosition(4, 2));
					newBlock.insertSmall(here + TilePosition(4, 4));
					newBlock.insertLarge(here);
					newBlock.insertLarge(here + TilePosition(0, 3));
					newBlock.insertLarge(here + TilePosition(6, 0));
					newBlock.insertLarge(here + TilePosition(6, 3));
				}
				else return;
			}
			else return;
		}
		if (race == Races::Terran)
		{
			if (height == 2) {
				if (width == 3) {
					newBlock.insertMedium(here);
				}
				else return;
			}
			else if (height == 4) {
				if (width == 3) {
					newBlock.insertMedium(here);
					newBlock.insertMedium(here + TilePosition(0, 2));
				}
				else if (width == 6) {
					newBlock.insertMedium(here);
					newBlock.insertMedium(here + TilePosition(0, 2));
					newBlock.insertMedium(here + TilePosition(3, 0));
					newBlock.insertMedium(here + TilePosition(3, 2));
				}
				else return;
			}
			else if (height == 5) {
				if (width == 6) {
					newBlock.insertLarge(here);
					newBlock.insertSmall(here + TilePosition(4, 1));
					newBlock.insertMedium(here + TilePosition(0, 3));
					newBlock.insertMedium(here + TilePosition(3, 3));
				}
				else return;
			}
			else if (height == 6) {
				if (width == 10) {
					newBlock.insertLarge(here);
					newBlock.insertLarge(here + TilePosition(4, 0));
					newBlock.insertLarge(here + TilePosition(0, 3));
					newBlock.insertLarge(here + TilePosition(4, 3));
					newBlock.insertSmall(here + TilePosition(8, 1));
					newBlock.insertSmall(here + TilePosition(8, 4));
				}
				else return;
			}
			else return;
		}
		blocks.push_back(newBlock);
		addOverlap(here, width, height);
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