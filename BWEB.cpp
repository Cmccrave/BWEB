#include "BWEB.h"

namespace BWEB
{
	void Map::draw()
	{
		for (auto &block : blocks)
		{
			for (auto tile : block.SmallTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
			for (auto tile : block.MediumTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(3, 2)), Broodwar->self()->getColor());
			for (auto tile : block.LargeTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(4, 3)), Broodwar->self()->getColor());
		}

		for (auto &station : stations)
		{
			for (auto tile : station.DefenseLocations())
				Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
			Broodwar->drawBoxMap(Position(station.BWEMBase()->Location()), Position(station.BWEMBase()->Location() + TilePosition(4, 3)), Broodwar->self()->getColor());
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

		
		for (int x = 0; x <= Broodwar->mapWidth(); x++)
		{
			for (int y = 0; y <= Broodwar->mapHeight(); y++)
			{
				if (reservePath[x][y] > 0)
				{
					Broodwar->drawCircleMap(Position(TilePosition(x, y)) + Position(16, 16), 4, Colors::Blue, true);
				}
			}
		}
	}

	void Map::onStart()
	{
		// For reference: https://imgur.com/a/I6IwH
		// TODO:	
		// - Simplify accessor functions
		// - Blocks for areas other than main
		// - Improve reservePath, fails on some FFE on non SSCAIT maps
		// - Test goldrush again for overlapping egg
		// - Add pylon grid
		// - Add used tiles into BWEB rather than locally in McRave?
		// - Stricter definition on chokes

		findMain();
		findNatural();
		findFirstChoke();
		findSecondChoke();
		findStations();
		findStartBlocks();
		findWalls();
		findBlocks();
	}

	TilePosition Map::getBuildPosition(UnitType building, const set<TilePosition> *usedTiles, TilePosition searchCenter)
	{
		double distBest = DBL_MAX;
		TilePosition tileBest = TilePositions::Invalid;

		for (auto &block : blocks)
		{
			set<TilePosition> placements;
			if (building.tileWidth() == 4) placements = block.LargeTiles();
			else if (building.tileWidth() == 3) placements = block.MediumTiles();
			else placements = block.SmallTiles();
			for (auto position : placements)
			{
				double distToPos = position.getDistance(searchCenter);
				if (distToPos < distBest && usedTiles->find(position) == usedTiles->end())
					distBest = distToPos, tileBest = position;
			}
		}
		return tileBest;
	}

	void Map::findMain()
	{
		tStart = Broodwar->self()->getStartLocation();
		pStart = Position(tStart) + Position(64, 48);
		mainArea = BWEM::Map::Instance().GetArea(tStart);
	}

	void Map::findNatural()
	{
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
		// Exception for maps with a natural behind the main such as Crossing Fields
		if (getGroundDistance(pStart, BWEM::Map::Instance().Center()) < getGroundDistance(Position(natural), BWEM::Map::Instance().Center()))
		{
			secondChoke = firstChoke;
			return;
		}

		// Find area that shares the choke we need to defend
		double distBest = DBL_MAX;
		const BWEM::Area* second = nullptr;
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

	double Map::getGroundDistance(Position start, Position end)
	{
		double dist = 0.0;
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

	Wall Map::getWall(BWEM::Area const* area)
	{
		if (areaWalls.find(area) != areaWalls.end())
			return areaWalls[area];
		return Wall();
	}

	Map* Map::BWEBInstance = nullptr;

	Map & Map::Instance()
	{
		if (!BWEBInstance) BWEBInstance = new Map();
		return *BWEBInstance;
	}
}