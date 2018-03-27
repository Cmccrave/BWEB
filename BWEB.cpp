#include "BWEB.h"

// TODO:
// Re-add Doors to Walls - NEW
// Final check on wall tight walls needs to check for tightness against terrain - NEW
// Reduce number of tiles searched in wall tight search to only buildable tiles - ONGOING
// Dynamic block addition (insert UnitTypes, get block) - NEW
// Change findPath to be able to create doors for any walls - NEW
// Limit number of building sizes per area - NEW

// Completed Changes:
// Changed Terran stations to reserve room for CC addons - COMPLETE


namespace BWEB
{
	void Map::onStart()
	{
		findMain();
		findNatural();
		findMainChoke();
		findNaturalChoke();
		findStations();
	}

	void Map::onUnitDiscover(Unit unit)
	{
		if (!unit || !unit->exists() || !unit->getType().isBuilding() || unit->isFlying()) return;
		if (unit->getType() == UnitTypes::Resource_Vespene_Geyser) return;

		TilePosition tile(unit->getTilePosition());
		UnitType type(unit->getType());

		for (int x = tile.x; x < tile.x + type.tileWidth(); x++)
		{
			for (int y = tile.y; y < tile.y + type.tileHeight(); y++)
			{
				TilePosition t(x, y);
				if (!t.isValid()) continue;
				usedTiles.insert(t);
			}
		}
	}

	void Map::onUnitMorph(Unit unit)
	{
		onUnitDiscover(unit);
	}

	void Map::onUnitDestroy(Unit unit)
	{
		if (!unit || !unit->getType().isBuilding() || unit->isFlying()) return;

		TilePosition tile(unit->getTilePosition());
		UnitType type(unit->getType());

		for (int x = tile.x; x < tile.x + type.tileWidth(); x++)
		{
			for (int y = tile.y; y < tile.y + type.tileHeight(); y++)
			{
				TilePosition t(x, y);
				if (!t.isValid()) continue;
				usedTiles.erase(t);
			}
		}
	}	

	void Map::findMain()
	{
		mainTile = Broodwar->self()->getStartLocation();
		mainPosition = (Position)mainTile + Position(64, 48);
		mainArea = BWEM::Map::Instance().GetArea(mainTile);
	}

	void Map::findNatural()
	{
		double distBest = DBL_MAX;
		for (auto& area : BWEM::Map::Instance().Areas())
		{
			for (auto& base : area.Bases())
			{
				if (base.Starting() || base.Geysers().size() == 0 || area.AccessibleNeighbours().size() == 0) continue;
				double dist = getGroundDistance(base.Center(), mainPosition);
				if (dist < distBest)
				{
					distBest = dist;
					naturalArea = base.GetArea();
					naturalTile = base.Location();
					naturalPosition = (Position)naturalTile;
				}
			}
		}		
	}

	void Map::findMainChoke()
	{
		double distBest = DBL_MAX;
		for (auto& choke : naturalArea->ChokePoints())
		{
			double dist = getGroundDistance(Position(choke->Center()), mainPosition);
			if (choke && dist < distBest)
				mainChoke = choke, distBest = dist;
		}
	}

	void Map::findNaturalChoke()
	{
		// Exception for maps with a natural behind the main such as Crossing Fields
		if (getGroundDistance(mainPosition, BWEM::Map::Instance().Center()) < getGroundDistance(Position(naturalTile), BWEM::Map::Instance().Center()))
		{
			naturalChoke = mainChoke;
			return;
		}

		// Find area that shares the choke we need to defend
		double distBest = DBL_MAX;
		const BWEM::Area* second = nullptr;
		for (auto& area : naturalArea->AccessibleNeighbours())
		{
			WalkPosition center = area->Top();
			double dist = Position(center).getDistance(BWEM::Map::Instance().Center());
			if (center.isValid() && dist < distBest)
				second = area, distBest = dist;
		}

		// Find second choke based on the connected area
		distBest = DBL_MAX;
		for (auto& choke : naturalArea->ChokePoints())
		{
			if (choke->Center() == mainChoke->Center()) continue;
			if (choke->Blocked() || choke->Geometry().size() <= 3) continue;
			if (choke->GetAreas().first != second && choke->GetAreas().second != second) continue;
			double dist = Position(choke->Center()).getDistance(Position(Broodwar->self()->getStartLocation()));
			if (dist < distBest)
				naturalChoke = choke, distBest = dist;
		}
	}

	void Map::draw()
	{
		for (auto& block : blocks)
		{
			for (auto& tile : block.SmallTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
			for (auto& tile : block.MediumTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), Broodwar->self()->getColor());
			for (auto& tile : block.LargeTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), Broodwar->self()->getColor());
		}

		for (auto& station : stations)
		{
			for (auto& tile : station.DefenseLocations())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
			Broodwar->drawBoxMap(Position(station.BWEMBase()->Location()), Position(station.BWEMBase()->Location()) + Position(129, 97), Broodwar->self()->getColor());
		}

		for (auto& wall : walls)
		{
			for (auto& tile : wall.smallTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
			for (auto& tile : wall.mediumTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), Broodwar->self()->getColor());
			for (auto& tile : wall.largeTiles())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), Broodwar->self()->getColor());

			Broodwar->drawBoxMap(Position(wall.getDoor()), Position(wall.getDoor()) + Position(33, 33), Broodwar->self()->getColor(), true);

			for (auto& tile : wall.getDefenses())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
		}
	}

	template <class PositionType>
	double Map::getGroundDistance(PositionType start, PositionType end)
	{
		double dist = 0.0;
		if (!start.isValid() || !end.isValid() || !BWEM::Map::Instance().GetArea(WalkPosition(start)) || !BWEM::Map::Instance().GetArea(WalkPosition(end)))
			return DBL_MAX;

		for (auto& cpp : BWEM::Map::Instance().GetPath(start, end))
		{
			auto center = Position{ cpp->Center() };
			dist += start.getDistance(center);
			start = center;
		}

		return dist += start.getDistance(end);
	}

	TilePosition Map::getBuildPosition(UnitType type, TilePosition searchCenter)
	{
		double distBest = DBL_MAX;
		TilePosition tileBest = TilePositions::Invalid;

		// Search through each block to find the closest block and valid position
		for (auto& block : blocks)
		{
			set<TilePosition> placements;
			if (type.tileWidth() == 4) placements = block.LargeTiles();
			else if (type.tileWidth() == 3) placements = block.MediumTiles();
			else placements = block.SmallTiles();

			for (auto& tile : placements)
			{
				double distToPos = tile.getDistance(searchCenter);
				if (distToPos < distBest && isPlaceable(type, tile))
					distBest = distToPos, tileBest = tile;
			}
		}
		return tileBest;
	}

	bool Map::isPlaceable(UnitType type, TilePosition location)
	{
		// Placeable is valid if buildable and not overlapping neutrals
		// Note: Must check neutrals due to the terrain below them technically being buildable
		for (int x = location.x; x < location.x + type.tileWidth(); x++)
		{
			for (int y = location.y; y < location.y + type.tileHeight(); y++)
			{
				TilePosition tile(x, y);
				if (!tile.isValid() || !Broodwar->isBuildable(tile) || overlapsNeutrals(tile)) return false;
			}
		}
		return true;
	}

	void Map::addOverlap(TilePosition t, int w, int h)
	{
		for (int x = t.x; x < t.x + w; x++)
		{
			for (int y = t.y; y < t.y + h; y++)
			{
				overlapGrid[x][y] = 1;
			}
		}
	}

	Map* Map::BWEBInstance = nullptr;

	Map & Map::Instance()
	{
		if (!BWEBInstance) BWEBInstance = new Map();
		return *BWEBInstance;
	}
}