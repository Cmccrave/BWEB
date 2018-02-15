#include "BWEB.h"

namespace BWEB
{
	void Map::onStart()
	{
		findMain();
		findNatural();
		findFirstChoke();
		findSecondChoke();
		findStations();
		findStartBlocks();
		findBlocks();
	}

	void Map::onUnitDiscover(Unit unit)
	{
		if (!unit || !unit->exists() || !unit->getType().isBuilding() || unit->isFlying()) return;
		if (unit->getType() == UnitTypes::Resource_Vespene_Geyser) return;
		TilePosition here = unit->getTilePosition();
		UnitType building = unit->getType();
		for (int x = here.x; x < here.x + building.tileWidth(); x++)
		{
			for (int y = here.y; y < here.y + building.tileHeight(); y++)
			{
				if (!TilePosition(x, y).isValid()) continue;
				usedTiles.insert(TilePosition(x, y));
			}
		}
	}

	void Map::onUnitDestroy(Unit unit)
	{
		if (!unit || !unit->getType().isBuilding() || unit->isFlying()) return;
		TilePosition here = unit->getTilePosition();
		UnitType building = unit->getType();
		for (int x = here.x; x < here.x + building.tileWidth(); x++)
		{
			for (int y = here.y; y < here.y + building.tileHeight(); y++)
			{
				if (!TilePosition(x, y).isValid()) continue;
				usedTiles.erase(TilePosition(x, y));
			}
		}
	}

	bool Map::buildingFits(TilePosition here, UnitType building, const set<TilePosition>& reservedTiles)
	{
		for (int x = here.x; x < here.x + building.tileWidth(); x++)
		{
			for (int y = here.y; y < here.y + building.tileHeight(); y++)
			{
				if (!TilePosition(x, y).isValid()) return false;
				if (reservedTiles.find(TilePosition(x, y)) != reservedTiles.end() || usedTiles.find(TilePosition(x, y)) != usedTiles.end()) return false;
			}
		}
		return true;
	}

	TilePosition Map::getBuildPosition(UnitType building, const set<TilePosition>& reservedTiles, TilePosition searchCenter)
	{
		double distBest = DBL_MAX;
		TilePosition tileBest = TilePositions::Invalid;

		for (auto &block : blocks)
		{
			set<TilePosition> placements;
			if (building.tileWidth() == 4) placements = block.LargeTiles();
			else if (building.tileWidth() == 3) placements = block.MediumTiles();
			else placements = block.SmallTiles();
			for (auto tile : placements)
			{
				double distToPos = tile.getDistance(searchCenter);
				if (distToPos < distBest && buildingFits(tile, building, reservedTiles))
					distBest = distToPos, tileBest = tile;
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
			if (choke->Blocked() || choke->Geometry().size() <= 3) continue;
			if (choke->GetAreas().first != second && choke->GetAreas().second != second) continue;
			double dist = Position(choke->Center()).getDistance(Position(Broodwar->self()->getStartLocation()));
			if (dist < distBest)
				secondChoke = TilePosition(choke->Center()), naturalChoke = choke, distBest = dist;
		}

		// Add the geometry of these tiles to a set
		if (naturalChoke)
		{
			for (auto tile : naturalChoke->Geometry())			
				chokeTiles.insert(TilePosition(tile));			
		}
	}

	void Map::draw()
	{
		//for (auto &block : blocks)
		//{
		//	for (auto tile : block.SmallTiles())
		//		Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
		//	for (auto tile : block.MediumTiles())
		//		Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), Broodwar->self()->getColor());
		//	for (auto tile : block.LargeTiles())
		//		Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), Broodwar->self()->getColor());
		//}

		//for (auto &station : stations)
		//{
		//	for (auto tile : station.DefenseLocations())
		//		Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
		//	Broodwar->drawBoxMap(Position(station.BWEMBase()->Location()), Position(station.BWEMBase()->Location()) + Position(129, 97), Broodwar->self()->getColor());
		//}

		for (auto &w : areaWalls)
		{
			Wall wall = w.second;
			for (auto tile : wall.smallTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
			for (auto tile : wall.mediumTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), Broodwar->self()->getColor());
			for (auto tile : wall.largeTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), Broodwar->self()->getColor());

			Broodwar->drawBoxMap(Position(wall.getDoor()), Position(wall.getDoor()) + Position(32, 32), Colors::Red);

			for (auto tile : wall.getDefenses())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
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

		Broodwar->drawCircleMap(Position(testTile), 16, Colors::Green, true);
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

	Wall* Map::getWall(BWEM::Area const* area)
	{
		if (areaWalls.find(area) != areaWalls.end())
			return &areaWalls.at(area);
		return nullptr;
	}

	Map* Map::BWEBInstance = nullptr;

	Map & Map::Instance()
	{
		if (!BWEBInstance) BWEBInstance = new Map();
		return *BWEBInstance;
	}
}