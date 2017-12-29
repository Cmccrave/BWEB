#include "BWEB.h"

void BWEBClass::draw()
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

	for (auto &w : walls)
	{
		Wall wall = w.second;
		Broodwar->drawBoxMap(Position(wall.getSmallWall()), Position(wall.getSmallWall()) + Position(64, 64), Broodwar->self()->getColor());
		Broodwar->drawBoxMap(Position(wall.getMediumWall()), Position(wall.getMediumWall()) + Position(94, 64), Broodwar->self()->getColor());
		Broodwar->drawBoxMap(Position(wall.getLargeWall()), Position(wall.getLargeWall()) + Position(128, 96), Broodwar->self()->getColor());
	}
}

void BWEBClass::onStart()
{
	// For reference: https://imgur.com/a/I6IwH
	// Currently missing features:	
	// - Counting of how many of each type of block
	// - Defensive blocks - cannons/turrets
	// - Blocks for areas other than main
	// - Variations based on build order (bio build or mech build)
	// - Smooth density
	// - Optimize starting blocks

	TilePosition tStart = Broodwar->self()->getStartLocation();
	Position pStart = Position(tStart) + Position(64, 48);
	Area const * mainArea = Map::Instance().GetArea(tStart);
	set<TilePosition> mainTiles;

	findNatural();
	findFirstChoke();
	findSecondChoke();
	findWalls();

	for (int x = 0; x <= Broodwar->mapWidth(); x++)
	{
		for (int y = 0; y <= Broodwar->mapHeight(); y++)
		{
			if (!TilePosition(x, y).isValid()) continue;
			if (!Map::Instance().GetArea(TilePosition(x, y))) continue;
			if (Map::Instance().GetArea(TilePosition(x, y)) == mainArea) mainTiles.insert(TilePosition(x, y));
		}
	}

	TilePosition best;	
	double distBest = DBL_MAX;
	for (auto tile : mainTiles)
	{
		if (!tile.isValid()) continue;
		Position blockCenter = Position(tile) + Position(144, 96);
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
		}
	}

	if (Broodwar->self()->getRace() == Races::Protoss)
	{
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

bool BWEBClass::canAddBlock(TilePosition here, int width, int height, bool baseBlock)
{
	// Check if a block of specified size would overlap any bases, resources or other blocks
	for (int x = here.x - 1; x < here.x + width + 1; x++)
	{
		for (int y = here.y - 1; y < here.y + height + 1; y++)
		{
			if (!TilePosition(x, y).isValid()) return false;
			if (!Map::Instance().GetTile(TilePosition(x, y)).Buildable()) return false;
			if (BWEBUtil().overlapsBlocks(TilePosition(x, y))) return false;
			if (BWEBUtil().overlapsBases(TilePosition(x, y)) && !baseBlock) return false;
			if (BWEBUtil().overlapsNeutrals(TilePosition(x, y))) return false;
			if (BWEBUtil().overlapsMining(TilePosition(x, y))) return false;
			if (BWEBUtil().overlapsWalls(TilePosition(x, y))) return false;
		}
	}
	return true;
}

void BWEBClass::insertSmallBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
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

void BWEBClass::insertMediumBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
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

void BWEBClass::insertLargeBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
{

}

void BWEBClass::insertStartBlock(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
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
	else
	{
		blocks[here] = Block(8, 5, here);

	}
}

TilePosition BWEBClass::getBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
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

TilePosition BWEBClass::getDefBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
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

TilePosition BWEBClass::getAnyBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
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

void BWEBClass::findNatural()
{
	// Find natural area
	double distBest = DBL_MAX;
	for (auto &area : Map::Instance().Areas())
	{
		for (auto &base : area.Bases())
		{
			if (base.Starting() || base.Geysers().size() == 0 || area.AccessibleNeighbours().size() == 0) continue;
			double dist = getGroundDistance(base.Center(), Position(Broodwar->self()->getStartLocation()));
			if (dist < distBest)
			{
				distBest = dist;
				naturalArea = base.GetArea();
				natural = base.Location();
			}
		}
	}
}

void BWEBClass::findFirstChoke()
{
	// Find the first choke
	double distBest = DBL_MAX;
	for (auto &choke : naturalArea->ChokePoints())
	{
		double dist = getGroundDistance(Position(choke->Center()), Position(Broodwar->self()->getStartLocation()));
		if (choke && dist < distBest)
			firstChoke = TilePosition(choke->Center()), distBest = dist;
	}
}

void BWEBClass::findSecondChoke()
{
	// Find area that shares the choke we need to defend
	double distBest = DBL_MAX;
	const Area* second = nullptr;
	for (auto &area : naturalArea->AccessibleNeighbours())
	{
		WalkPosition center = area->Top();
		double dist = Position(center).getDistance(Map::Instance().Center());
		if (center.isValid() && dist < distBest)
			second = area, distBest = dist;
	}

	// Find second choke based on the connected area
	distBest = DBL_MAX;
	for (auto &choke : naturalArea->ChokePoints())
	{
		if (TilePosition(choke->Center()) == firstChoke) continue;
		if (choke->GetAreas().first != second && choke->GetAreas().second != second) continue;
		double dist = Position(choke->Center()).getDistance(Position(Broodwar->self()->getStartLocation()));
		if (dist < distBest)
			secondChoke = TilePosition(choke->Center()), distBest = dist;
	}
}

void BWEBClass::findWalls()
{
	TilePosition start = secondChoke;
	double distance = DBL_MAX;
	TilePosition small, medium, large;

	// Large Building placement
	for (int x = start.x - 20; x <= start.x + 20; x++)
	{
		for (int y = start.y - 20; y <= start.y + 20; y++)
		{
			if (!TilePosition(x, y).isValid()) continue;
			Position center = Position(TilePosition(x, y)) + Position(64, 48);
			Position chokeCenter = Position(secondChoke) + Position(16, 16);
			bool buildable = true;
			int valid = 0;
			for (int i = x; i < x + 4; i++)
			{
				for (int j = y; j < y + 3; j++)
				{
					if (!TilePosition(i, j).isValid()) continue;
					if (!Broodwar->isBuildable(TilePosition(i, j))) buildable = false;
					if (i >= natural.x && i < natural.x + 4 && j >= natural.y && j < natural.y + 3) buildable = false;
					if (Map::Instance().GetArea(TilePosition(i, j)) && Map::Instance().GetArea(TilePosition(i, j)) == naturalArea) valid = 1;
				}
			}
			if (!buildable) continue;


			int dx = x + 4;
			for (int dy = y; dy < y + 3; dy++)
			{
				if (!BWEBUtil().isWalkable(TilePosition(dx, dy))) valid++;
			}

			int dy = y + 3;
			for (int dx = x; dx < x + 4; dx++)
			{
				if (!BWEBUtil().isWalkable(TilePosition(dx, dy))) valid++;
			}

			double distNat = center.getDistance(Position(natural));
			double distChoke = center.getDistance(chokeCenter);
			if (valid >= 2 && distNat <= 512 && distChoke < distance)
				large = TilePosition(x, y), distance = distChoke;
		}
	}

	// Medium Building placement
	distance = 0.0;
	for (int x = start.x - 20; x <= start.x + 20; x++)
	{
		for (int y = start.y - 20; y <= start.y + 20; y++)
		{
			if (!TilePosition(x, y).isValid()) continue;
			Position center = Position(TilePosition(x, y)) + Position(48, 32);
			Position chokeCenter = Position(secondChoke) + Position(16, 16);
			Position bLargeCenter = Position(large) + Position(64, 48);

			bool buildable = true;
			bool within = false;
			int valid = 0;
			for (int i = x; i < x + 3; i++)
			{
				for (int j = y; j < y + 2; j++)
				{
					if (!TilePosition(i, j).isValid()) continue;
					if (!Broodwar->isBuildable(TilePosition(i, j))) buildable = false;
					if (i >= natural.x && i < natural.x + 4 && j >= natural.y && j < natural.y + 3) buildable = false;
					if (i >= large.x && i < large.x + 4 && j >= large.y && j < large.y + 3) buildable = false;
					if (Map::Instance().GetArea(TilePosition(i, j)) && Map::Instance().GetArea(TilePosition(i, j)) == naturalArea) within = true;
				}
			}

			for (int i = x - 1; i < x + 4; i++)
			{
				for (int j = y - 1; j < y + 3; j++)
				{
					if (i >= large.x && i < large.x + 4 && j >= large.y && j < large.y + 3) buildable = false;
				}
			}

			if (!buildable || !within) continue;

			int dx = x - 1;
			for (int dy = y; dy < y + 2; dy++)
			{
				if (dx >= large.x && dx < large.x + 4 && dy >= large.y && dy < large.y + 3) buildable = false;
				if (!BWEBUtil().isWalkable(TilePosition(dx, dy))) valid++;
			}

			int dy = y - 1;
			for (int dx = x; dx < x + 3; dx++)
			{
				if (dx >= large.x && dx < large.x + 4 && dy >= large.y && dy < large.y + 3) buildable = false;
				if (!BWEBUtil().isWalkable(TilePosition(dx, dy))) valid++;
			}

			if (!buildable) continue;

			if (valid >= 1 && center.getDistance(Position(natural)) <= 512 && (center.getDistance(chokeCenter) < distance || distance == 0.0)) medium = TilePosition(x, y), distance = center.getDistance(chokeCenter);
		}
	}

	// Pylon placement 
	distance = 0.0;
	for (int x = natural.x - 20; x <= natural.x + 20; x++)
	{
		for (int y = natural.y - 20; y <= natural.y + 20; y++)
		{
			if (!TilePosition(x, y).isValid()) continue;
			if (TilePosition(x, y) == secondChoke) continue;
			Position center = Position(TilePosition(x, y)) + Position(32, 32);
			Position bLargeCenter = Position(large) + Position(64, 48);
			Position bMediumCenter = Position(medium) + Position(48, 32);

			bool buildable = true;
			for (int i = x; i < x + 2; i++)
			{
				for (int j = y; j < y + 2; j++)
				{
					if (!Broodwar->isBuildable(TilePosition(i, j))) buildable = false;
					if (i >= medium.x && i < medium.x + 3 && j >= medium.y && j < medium.y + 2) buildable = false;
					if (i >= large.x && i < large.x + 4 && j >= large.y && j < large.y + 3) buildable = false;
					if (i >= natural.x && i < natural.x + 4 && j >= natural.y && j < natural.y + 3) buildable = false;
				}
			}

			if (!buildable) continue;
			if (buildable && Map::Instance().GetArea(TilePosition(center)) == Map::Instance().GetArea(natural) && center.getDistance(bLargeCenter) <= 160 && center.getDistance(bMediumCenter) <= 160 && (center.getDistance(Position(secondChoke)) > distance || distance == 0.0)) small = TilePosition(x, y), distance = center.getDistance(Position(secondChoke));
		}
	}

	int reservePathHome[256][256] = {};
	Wall naturalWall;
	naturalWall.setSmallWall(small);
	naturalWall.setMediumWall(medium);
	naturalWall.setLargeWall(large);

	walls[naturalArea] = naturalWall;

	// Create reserve path home
	TilePosition end = small + TilePosition(1, 1);
	TilePosition middle = (large + medium) / 2;

	start = (large + medium) / 2;
	end = firstChoke;
	int range = firstChoke.getDistance(secondChoke);

	for (int i = 0; i <= range; i++)
	{
		set<TilePosition> testCases;
		testCases.insert(TilePosition(start.x - 1, start.y));
		testCases.insert(TilePosition(start.x + 1, start.y));
		testCases.insert(TilePosition(start.x, start.y - 1));
		testCases.insert(TilePosition(start.x, start.y + 1));

		TilePosition bestTile;
		double distBest = DBL_MAX;
		for (auto tile : testCases)
		{
			if (!tile.isValid() || BWEBUtil().overlapsWalls(tile)) continue;
			if (BWEBUtil().overlapsBases(tile)) continue;
			if (Map::Instance().GetArea(Broodwar->self()->getStartLocation()) == Map::Instance().GetArea(tile)) continue;
			if (!BWEBUtil().isWalkable(tile)) continue;

			double dist = getGroundDistance(Position(tile), Position(end));
			if (dist < distBest)
			{
				distBest = dist;
				bestTile = tile;
			}
		}

		if (bestTile.isValid())
		{
			start = bestTile;
			reservePathHome[bestTile.x][bestTile.y] = 1;
		}
		else break;

		if (start.getDistance(end) < 2)	break;
	}

	for (int i = 0; i < 4; i++)
	{
		distance = DBL_MAX;
		TilePosition best;
		for (int x = start.x - 20; x <= start.x + 20; x++)
		{
			for (int y = start.y - 20; y <= start.y + 20; y++)
			{
				if (!TilePosition(x, y).isValid()) continue;
				if (TilePosition(x, y) == secondChoke) continue;
				Position center = Position(TilePosition(x, y)) + Position(32, 32);
				Position hold = Position(secondChoke);
				double dist = center.getDistance(hold);
				bool buildable = true;

				for (int i = x; i < x + 2; i++)
				{
					for (int j = y; j < y + 2; j++)
					{
						for (auto tile : sDefPosition)
						{
							if (i >= tile.x && i < tile.x + 2 && j >= tile.y && j < tile.y + 2) buildable = false;
						}

						if (!Broodwar->isBuildable(TilePosition(i, j))) buildable = false;
						if (reservePathHome[i][j] > 0) buildable = false;
						if (i >= small.x && i < small.x + 2 && j >= small.y && j < small.y + 2) buildable = false;
						if (i >= medium.x && i < medium.x + 3 && j >= medium.y && j < medium.y + 2) buildable = false;
						if (i >= large.x && i < large.x + 4 && j >= large.y && j < large.y + 3) buildable = false;
						if (i >= natural.x && i < natural.x + 4 && j >= natural.y && j < natural.y + 3) buildable = false;
					}
				}

				if (!buildable) continue;

				if (Map::Instance().GetArea(TilePosition(center)) != naturalArea) continue;
				if (dist >= 96 && dist <= 320 && dist < distance)
					best = TilePosition(x, y), distance = dist;

			}
		}
		sDefPosition.insert(best);
	}
}

int BWEBClass::getGroundDistance(Position start, Position end)
{
	int dist = 0;
	if (!start.isValid() || !end.isValid()) return INT_MAX;
	if (!Map::Instance().GetArea(WalkPosition(start)) || !Map::Instance().GetArea(WalkPosition(end))) return INT_MAX;

	for (auto cpp : Map::Instance().GetPath(start, end))
	{
		auto center = Position{ cpp->Center() };
		dist += start.getDistance(center);
		start = center;
	}

	return dist += start.getDistance(end);
}

Wall BWEBClass::getWall(Area const* area)
{
	if (walls.find(area) != walls.end())
		return walls[area];
}

BWEBClass* BWEBClass::bInstance = nullptr;

BWEBClass & BWEBClass::Instance()
{
	if (!bInstance) bInstance = new BWEBClass();
	return *bInstance;
}
