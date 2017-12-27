#include "BWEB.h"

void BWEB::draw()
{
	for (auto tile : smallPosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
	for (auto tile : mediumPosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(3, 2)), Broodwar->self()->getColor());
	for (auto tile : largePosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(4, 3)), Broodwar->self()->getColor());
	for (auto tile : sDefPosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
	for (auto tile : mDefPosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(3, 2)), Broodwar->self()->getColor());
	for (auto tile : expoPosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(4, 3)), Broodwar->self()->getColor());
}

void BWEB::onStart()
{
	// For reference: https://imgur.com/a/I6IwH
	// Currently missing features:	
	// - Counting of how many of each type of block
	// - Defensive blocks - cannons/turrets
	// - Blocks for areas other than main
	// - Variations based on build order (bio build or mech build)
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

	// Mirror check, normally production above and left
	bool mirrorHorizontal = false, mirrorVertical = false;
	if (Map::Instance().Center().x > pStart.x) mirrorHorizontal = true;
	if (Map::Instance().Center().y > pStart.y) mirrorVertical = true;

	for (auto &area : Map::Instance().Areas())
	{
		for (auto &base : area.Bases())
		{
			TilePosition center;
			int cnt = 0;
			int h1 = Broodwar->self()->getRace() == Races::Protoss ? 4 : 2;
			int h2 = Broodwar->self()->getRace() == Races::Protoss ? 0 : 2;
			int v1 = Broodwar->self()->getRace() == Races::Protoss ? 3 : 2;
			int v2 = Broodwar->self()->getRace() == Races::Protoss ? 3 : 2;

			for (auto &mineral : base.Minerals())
				center += mineral->TopLeft(), cnt++;

			for (auto &gas : base.Geysers())
				center += gas->TopLeft(), cnt++;

			if (cnt > 0) center = center / cnt;
			resourceCenter.insert(center);

			if (abs(center.x - base.Location().x) > abs(center.y - base.Location().y))
			{
				if (center.x > base.Location().x)
					insertHExpoBlock(base.Location() - TilePosition(h1, 0), false, mirrorVertical);
				else
					insertHExpoBlock(base.Location() - TilePosition(h2, 0), true, mirrorVertical);
			}
			else
			{
				if (center.y > base.Location().y)
					insertVExpoBlock(base.Location() - TilePosition(0, v1), mirrorHorizontal, false);
				else
					insertVExpoBlock(base.Location() - TilePosition(0, v2), mirrorHorizontal, true);
			}
		}
	}

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

bool BWEB::canAddBlock(TilePosition here, int width, int height)
{
	// Check if a block of specified size would overlap any bases, resources or other blocks
	for (int x = here.x - 1; x < here.x + width + 1; x++)
	{
		for (int y = here.y - 1; y < here.y + height + 1; y++)
		{
			if (!TilePosition(x, y).isValid()) return false;
			if (!Map::Instance().GetTile(TilePosition(x, y)).Buildable()) return false;
			if (overlapsBlocks(TilePosition(x, y))) return false;
			if (overlapsBases(TilePosition(x, y))) return false;
			if (overlapsNeutrals(TilePosition(x, y))) return false;
			if (overlapsMining(TilePosition(x, y))) return false;
		}
	}
	return true;
}

void BWEB::insertSmallBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
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

void BWEB::insertMediumBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
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

void BWEB::insertLargeBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
{

}

void BWEB::insertHExpoBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
{
	// TODO -- mirror based on gas position	
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		blocks[here] = Block(8, 8, here);
		if (mirrorHorizontal)
		{
			expoPosition.insert(here);			
			largePosition.insert(here + TilePosition(4, 0));
			largePosition.insert(here + TilePosition(4, 5));
			mDefPosition.insert(here + TilePosition(0, 3));
			mediumPosition.insert(here + TilePosition(5, 3));
			smallPosition.insert(here + TilePosition(3, 3));
		}
		else
		{
			expoPosition.insert(here + TilePosition(4, 0));
			largePosition.insert(here);
			largePosition.insert(here + TilePosition(0, 5));
			mediumPosition.insert(here + TilePosition(0, 3));
			mDefPosition.insert(here + TilePosition(5, 3));
			smallPosition.insert(here + TilePosition(3, 3));
		}
	}
	else
	{
		blocks[here] = Block(8, 5, here);
		if (mirrorHorizontal)
		{
			expoPosition.insert(here + TilePosition(2, 0));
			mediumPosition.insert(here + TilePosition(5, 3));
			mDefPosition.insert(here + TilePosition(2, 3));
			smallPosition.insert(here + TilePosition(6, 1));
			sDefPosition.insert(here + TilePosition(0, 1));
		}
		else
		{
			expoPosition.insert(here + TilePosition(2, 0));
			mediumPosition.insert(here + TilePosition(0, 3));
			mDefPosition.insert(here + TilePosition(3, 3));
			smallPosition.insert(here + TilePosition(6, 1));
			sDefPosition.insert(here + TilePosition(0, 1));
		}
	}
}

void BWEB::insertVExpoBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		blocks[here] = Block(7, 6, here);
		if (mirrorVertical)
		{
			expoPosition.insert(here);
			largePosition.insert(here + TilePosition(0, 3));
			mDefPosition.insert(here + TilePosition(4, 0));
			mediumPosition.insert(here + TilePosition(4, 4));
			smallPosition.insert(here + TilePosition(4, 2));
		}
		else
		{
			expoPosition.insert(here);
			largePosition.insert(here + TilePosition(0, 3));
			mDefPosition.insert(here + TilePosition(4, 0));
			mediumPosition.insert(here + TilePosition(4, 4));
			smallPosition.insert(here + TilePosition(4, 2));
		}
	}
	else
	{
		blocks[here] = Block(6, 5, here);
		if (mirrorVertical)
		{
			expoPosition.insert(here);
			mediumPosition.insert(here + TilePosition(0, 3));
			mDefPosition.insert(here + TilePosition(3, 3));
			smallPosition.insert(here + TilePosition(4, 1));
		}
		else
		{
			expoPosition.insert(here + TilePosition(0, 2));
			mediumPosition.insert(here);
			mDefPosition.insert(here + TilePosition(3, 0));
			smallPosition.insert(here + TilePosition(4, 3));
		}
	}
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

bool BWEB::overlapsBlocks(TilePosition here)
{
	for (auto &b : blocks)
	{
		Block& block = b.second;
		if (here.x >= block.tile().x && here.x < block.tile().x + block.width() && here.y >= block.tile().y && here.y < block.tile().y + block.height()) return true;
	}
	return false;
}

bool BWEB::overlapsMining(TilePosition here)
{
	for (auto &tile : resourceCenter)
		if (tile.getDistance(here) < 5) return true;
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
		if (here.x >= tile.x && here.x < tile.x + 4 && here.y >= tile.y && here.y < tile.y + 2) return true;
	}
	return false;
}

BWEB* BWEB::bInstance = nullptr;

BWEB & BWEB::Instance()
{
	if (!bInstance) bInstance = new BWEB();
	return *bInstance;
}

TilePosition BWEB::getBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
{
	double distBest = DBL_MAX;
	TilePosition tileBest = TilePositions::Invalid;
	switch (building.tileWidth())
	{
	case 4:
		for (auto &position : largePosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		break;
	case 3:
		for (auto &position : mediumPosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		break;
	case 2:
		for (auto &position : smallPosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		break;
	}
	return tileBest;
}

TilePosition BWEB::getDefBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
{
	double distBest = DBL_MAX;
	TilePosition tileBest = TilePositions::Invalid;
	switch (building.tileWidth())
	{
	case 4:
		break;
	case 3:
		for (auto &position : mDefPosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		break;
	case 2:
		for (auto &position : sDefPosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		break;
	}
	return tileBest;
}

TilePosition BWEB::getAnyBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
{
	double distBest = DBL_MAX;
	TilePosition tileBest = TilePositions::Invalid;
	switch (building.tileWidth())
	{
	case 4:
		for (auto &position : largePosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		break;
	case 3:
		for (auto &position : mediumPosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		for (auto &position : mDefPosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		break;
	case 2:
		for (auto &position : smallPosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		for (auto &position : sDefPosition)
		{
			double distToPos = position.getDistance(searchCenter);
			if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
				distBest = distToPos, tileBest = position;
		}
		break;
	}
	return tileBest;
}