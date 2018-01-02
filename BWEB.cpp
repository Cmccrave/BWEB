#include "BWEB.h"

namespace BWEB
{
	void Map::draw()
	{
		for (auto &b : prodBlocks)
		{
			Block block = b.second;
			for (auto tile : block.getPylon())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
			for (auto tile : block.getSmall())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
			for (auto tile : block.getMedium())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(3, 2)), Broodwar->self()->getColor());
			for (auto tile : block.getLarge())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(4, 3)), Broodwar->self()->getColor());
		}

		for (auto &b : expoBlocks)
		{
			Block block = b.second;
			for (auto tile : block.getPylon())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
			for (auto tile : block.getSmall())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
			for (auto tile : block.getMedium())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(3, 2)), Broodwar->self()->getColor());
			for (auto tile : block.getLarge())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(4, 3)), Broodwar->self()->getColor());
		}

		for (auto &w : areaWalls)
		{
			Wall wall = w.second;
			Broodwar->drawBoxMap(Position(wall.getSmallWall()), Position(wall.getSmallWall()) + Position(64, 64), Broodwar->self()->getColor());
			Broodwar->drawBoxMap(Position(wall.getMediumWall()), Position(wall.getMediumWall()) + Position(94, 64), Broodwar->self()->getColor());
			Broodwar->drawBoxMap(Position(wall.getLargeWall()), Position(wall.getLargeWall()) + Position(128, 96), Broodwar->self()->getColor());

			for (auto tile : wall.getDefenses())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
		}
	}

	void Map::onStart()
	{
		// For reference: https://imgur.com/a/I6IwH
		// TODO:	
		// - Simplify accessor functions
		// - Blocks for areas other than main
		// - Check why path isnt working like McRave did

		findNatural();
		findFirstChoke();
		findSecondChoke();
		findStartBlocks();
		findWalls();
		findBlocks();
	}

	TilePosition Map::getBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
	{
		double distBest = DBL_MAX;
		TilePosition tileBest = TilePositions::Invalid;

		for (auto b : prodBlocks)
		{
			Block block = b.second;
			set<TilePosition> placements;
			if (building.tileWidth() == 4) placements = block.getLarge();
			else if (building.tileWidth() == 3) placements = block.getMedium();
			else placements = block.getSmall();
			{
				for (auto position : placements)
				{
					double distToPos = position.getDistance(searchCenter);
					if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
						distBest = distToPos, tileBest = position;
				}
			}
		}

		/*switch (building.tileWidth())
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
		}*/
		return tileBest;
	}

	TilePosition Map::getDefBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
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

	TilePosition Map::getAnyBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
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

	void Map::findNatural()
	{
		// Find natural area
		double distBest = DBL_MAX;
		for (auto &area : BWEM::Map::Instance().Areas())
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

	void Map::findFirstChoke()
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

	void Map::findSecondChoke()
	{
		// Find area that shares the choke we need to defend
		double distBest = DBL_MAX;
		const Area* second = nullptr;
		for (auto &area : naturalArea->AccessibleNeighbours())
		{
			WalkPosition center = area->Top();
			double dist = Position(center).getDistance(BWEM::Map::Instance().Center());
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

	int Map::getGroundDistance(Position start, Position end)
	{
		int dist = 0;
		if (!start.isValid() || !end.isValid()) return INT_MAX;
		if (!BWEM::Map::Instance().GetArea(WalkPosition(start)) || !BWEM::Map::Instance().GetArea(WalkPosition(end))) return INT_MAX;

		for (auto cpp : BWEM::Map::Instance().GetPath(start, end))
		{
			auto center = Position{ cpp->Center() };
			dist += start.getDistance(center);
			start = center;
		}

		return dist += start.getDistance(end);
	}

	Wall Map::getWall(Area const* area)
	{
		if (areaWalls.find(area) != areaWalls.end())
			return areaWalls[area];
	}

	Map* Map::BWEBInstance = nullptr;

	Map & Map::Instance()
	{
		if (!BWEBInstance) BWEBInstance = new Map();
		return *BWEBInstance;
	}
}