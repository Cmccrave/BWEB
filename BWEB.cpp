#include "BWEB.h"

// TODO:
// Restructure - NEW CRITICAL
// Wall improvements - NEW HIGH
// Performance improvements - NEW MEDIUM
// Block improvements - NEW MEDIUM
// Dynamic block addition (insert UnitTypes, get block) - NEW MEDIUM
// Improve logic for mirroring blocks - NEW LOW
// Code cleanup - NEW LOW
// Limit number of building sizes per area - ONGOING LOW

// Completed Changes:
// Re-add Doors to Walls - COMPLETE
// Improve "Overlap functions", they are probably fairly expensive - COMPLETE
// Changed Terran stations to reserve room for CC addons - COMPLETE
// Reduce number of tiles searched in wall tight search to only buildable tiles - COMPLETE
// Final check on wall tight walls needs to check for tightness against terrain - NFG

namespace BWEB
{
	void Map::onStart()
	{
		findMain();
		findNatural();
		findMainChoke();
		findNaturalChoke();
		findStations();

		for (auto &unit : Broodwar->neutral()->getUnits())
			addOverlap(unit->getTilePosition(), unit->getType().tileWidth(), unit->getType().tileHeight());
	}

	void Map::onUnitDiscover(const Unit unit)
	{
		if (!unit || !unit->exists() || !unit->getType().isBuilding() || unit->isFlying()) return;
		if (unit->getType() == UnitTypes::Resource_Vespene_Geyser) return;

		const auto tile(unit->getTilePosition());
		auto type(unit->getType());

		for (auto x = tile.x; x < tile.x + type.tileWidth(); x++)
		{
			for (auto y = tile.y; y < tile.y + type.tileHeight(); y++)
			{
				TilePosition t(x, y);
				if (!t.isValid()) continue;
				usedTiles.insert(t);
			}
		}
	}

	void Map::onUnitMorph(const Unit unit)
	{
		onUnitDiscover(unit);
	}

	void Map::onUnitDestroy(const Unit unit)
	{
		if (!unit || !unit->getType().isBuilding() || unit->isFlying()) return;

		const auto tile(unit->getTilePosition());
		auto type(unit->getType());

		for (auto x = tile.x; x < tile.x + type.tileWidth(); x++)
		{
			for (auto y = tile.y; y < tile.y + type.tileHeight(); y++)
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
		mainPosition = static_cast<Position>(mainTile) + Position(64, 48);
		mainArea = BWEM::Map::Instance().GetArea(mainTile);
	}

	void Map::findNatural()
	{
		auto distBest = DBL_MAX;
		for (auto& area : BWEM::Map::Instance().Areas())
		{
			for (auto& base : area.Bases())
			{
				if (base.Starting() || base.Geysers().empty() || area.AccessibleNeighbours().empty()) continue;
				const auto dist = getGroundDistance(base.Center(), mainPosition);
				if (dist < distBest)
				{
					distBest = dist;
					naturalArea = base.GetArea();
					naturalTile = base.Location();
					naturalPosition = static_cast<Position>(naturalTile) + Position(64, 48);
				}
			}
		}
	}

	void Map::findMainChoke()
	{
		auto distBest = DBL_MAX;
		for (auto& choke : naturalArea->ChokePoints())
		{
			const auto dist = getGroundDistance(Position(choke->Center()), mainPosition);
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
		auto distBest = DBL_MAX;
		const BWEM::Area* second = nullptr;
		for (auto& area : naturalArea->AccessibleNeighbours())
		{
			auto center = area->Top();
			const auto dist = Position(center).getDistance(BWEM::Map::Instance().Center());
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
			const auto dist = Position(choke->Center()).getDistance(Position(Broodwar->self()->getStartLocation()));
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
			for (auto& tile : wall.getDefenses())
				Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
			Broodwar->drawBoxMap(Position(wall.getDoor()), Position(wall.getDoor()) + Position(33, 33), Broodwar->self()->getColor(), true);
		}

		//for (int x = 0; x < Broodwar->mapWidth()*4; x++)
		//{
		//	for (int y = 0; y < Broodwar->mapHeight()*4; y++)
		//	{
		//		WalkPosition t(x, y);
		//		if (!Broodwar->isWalkable(t))
		//			Broodwar->drawBoxMap(Position(t), Position(t) + Position(9, 9), Broodwar->self()->getColor(), true);
		//	}
		//}

		//Broodwar->drawCircleMap(Position(startTile), 8, Colors::Green, true);
		//Broodwar->drawCircleMap(Position(endTile), 8, Colors::Orange, true);
		//Broodwar->drawCircleMap(naturalPosition, 8, Colors::Red, true);
		////Broodwar->drawCircleMap(Position(mainChoke->Center()), 8, Colors::Green, true);
		//Broodwar->drawCircleMap(Position(naturalChoke->Center()), 8, Colors::Yellow, true);
	}

	template <class PositionType>
	double Map::getGroundDistance(PositionType start, PositionType end)
	{
		auto dist = 0.0;
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

	TilePosition Map::getBuildPosition(UnitType type, const TilePosition searchCenter)
	{
		auto distBest = DBL_MAX;
		auto tileBest = TilePositions::Invalid;

		// Search through each block to find the closest block and valid position
		for (auto& block : blocks)
		{
			set<TilePosition> placements;
			if (type.tileWidth() == 4) placements = block.LargeTiles();
			else if (type.tileWidth() == 3) placements = block.MediumTiles();
			else placements = block.SmallTiles();

			for (auto& tile : placements)
			{
				const auto distToPos = tile.getDistance(searchCenter);
				if (distToPos < distBest && isPlaceable(type, tile))
					distBest = distToPos, tileBest = tile;
			}
		}
		return tileBest;
	}

	bool Map::isPlaceable(UnitType type, const TilePosition location)
	{
		// Placeable is valid if buildable and not overlapping neutrals
		// Note: Must check neutrals due to the terrain below them technically being buildable
		const auto creepCheck = type.requiresCreep() ? 1 : 0;
		const auto powerCheck = type.requiresPsi();
		for (auto x = location.x; x < location.x + type.tileWidth(); x++)
		{
			for (auto y = location.y; y < location.y + type.tileHeight() + creepCheck; y++)
			{
				TilePosition tile(x, y);
				if (powerCheck && !Broodwar->hasPower(tile)) continue;
				if (!tile.isValid() || !Broodwar->isBuildable(tile)) return false;
				if (usedTiles.find(tile) != usedTiles.end()) return false;
				if (type.isResourceDepot() && !Broodwar->canBuildHere(tile, type)) return false;
			}
		}
		return true;
	}

	void Map::addOverlap(const TilePosition t, const int w, const int h)
	{
		for (auto x = t.x; x < t.x + w; x++)
		{
			for (auto y = t.y; y < t.y + h; y++)
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