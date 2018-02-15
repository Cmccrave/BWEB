#include "Wall.h"
#include "AStar.h"

namespace BWEB
{
	void Map::findWallSegment(UnitType building, bool fullWall)
	{
		double best = DBL_MAX;
		TilePosition tileBest = TilePositions::Invalid;
		Position chokeCenter = Position(secondChoke) + Position(16, 16);
		Position natCenter = Position(natural) + Position(64, 48);

		for (int x = secondChoke.x - 16; x <= secondChoke.x + 16; x++)
		{
			for (int y = secondChoke.y - 16; y <= secondChoke.y + 16; y++)
			{
				int score = 1;
				TilePosition tile = TilePosition(x, y);
				if (!tile.isValid()) continue;

				Position center = Position(tile) + Position(building.tileWidth() * 16, building.tileHeight() * 16);

				if (BWEBUtil().overlapsAnything(tile, building.tileWidth(), building.tileHeight(), true) || !canPlaceHere(building, TilePosition(x, y))) continue;						// If this is overlapping anything
				if (building != UnitTypes::Protoss_Pylon && fullWall && !wallTight(building, tile, fullWall)) continue;																							// If expecting it to be a full wall off and it's not wall tight
				if (building == UnitTypes::Protoss_Pylon && BWEM::Map::Instance().GetArea(TilePosition(center)) != BWEM::Map::Instance().GetArea(natural)) continue;					// If this is not within our natural area

				if (!BWEBUtil().insideNatArea(tile, building.tileWidth(), building.tileHeight())) continue;		// Testing this				

				//score += BWEBUtil().insideNatArea(tile, building.tileWidth(), building.tileHeight());			// Increase score if it's within the natural area (redundant, TODO)
				score += overlapsChoke(building, tile);															// Increase score for each tile on the choke (TODO for each or just one?)

				if (building == UnitTypes::Protoss_Pylon)														// Pylons want to be as far away from the choke as possible while still powering buildings
				{
					double distChoke = center.getDistance(chokeCenter);
					double distNat = center.getDistance(natCenter);
					double overallScore = distNat / (double(score) * distChoke);
					if (powersWall(tile) && overallScore < best)
						tileBest = tile, best = overallScore;
				}
				else
				{
					double distChoke = exp(center.getDistance(chokeCenter));
					double distNat = log(center.getDistance(natCenter));
					double overallScore = (distNat * distChoke) / double(score);
					//double overallScore = (distNat * distChoke * log((double)BWEM::Map::Instance().GetTile(tile).MinAltitude())) / (double(score));
					if (overallScore < best)
						tileBest = tile, best = overallScore;
				}
			}
		}

		for (int x = tileBest.x; x < tileBest.x + building.tileWidth(); x++)
		{
			for (int y = tileBest.y; y < tileBest.y + building.tileHeight(); y++)
			{
				for (auto it = blocks.begin(); it != blocks.end(); it++)
				{
					auto & block = *it;
					TilePosition test = TilePosition(x, y);
					if (test.x >= block.Location().x && test.x < block.Location().x + block.width() && test.y >= block.Location().y && test.y < block.Location().y + block.height())
					{
						blocks.erase(it);
						break;
					}
				}
			}
		}

		if (tileBest.isValid())
			areaWalls[naturalArea].insertSegment(tileBest, building);
		else
			Broodwar << "BWEB: Failed to place " << building.c_str() << " wall segment." << endl;
	}

	void Map::findWallDefenses(UnitType building, int count)
	{
		for (int i = 0; i < count; i++)
		{
			double distance = DBL_MAX;
			TilePosition tileBest = TilePositions::Invalid;
			TilePosition start = secondChoke;
			for (int x = start.x - 20; x <= start.x + 20; x++)
			{
				for (int y = start.y - 20; y <= start.y + 20; y++)
				{
					// Missing: within nat area, buildable, not on reserve path
					if (!TilePosition(x, y).isValid()) continue;
					if (BWEBUtil().overlapsAnything(TilePosition(x, y), 2, 2, true)) continue;
					if (!canPlaceHere(building, TilePosition(x, y))) continue;

					Position center = Position(TilePosition(x, y)) + Position(32, 32);
					Position hold = Position(secondChoke);
					double dist = center.getDistance(hold);

					if (BWEM::Map::Instance().GetArea(TilePosition(center)) != naturalArea) continue;
					if ((dist >= 128 || Broodwar->self()->getRace() == Races::Zerg) && dist < distance)
						tileBest = TilePosition(x, y), distance = dist;
				}
			}

			for (int x = tileBest.x; x < tileBest.x + building.tileWidth(); x++)
			{
				for (int y = tileBest.y; y < tileBest.y + building.tileHeight(); y++)
				{
					for (auto it = blocks.begin(); it != blocks.end(); it++)
					{
						auto & block = *it;
						TilePosition test = TilePosition(x, y);
						if (test.x >= block.Location().x && test.x < block.Location().x + block.width() && test.y >= block.Location().y && test.y < block.Location().y + block.height())
						{
							blocks.erase(it);
							break;
						}
					}
				}
			}

			if (tileBest.isValid()) areaWalls[naturalArea].insertDefense(tileBest);
			else return;
		}
	}

	void Map::findPath()
	{
		TilePosition tileBest = TilePositions::Invalid;
		double distBest = DBL_MAX;
		TilePosition end = secondChoke;

		for (int x = end.x - 25; x <= end.x + 25; x++)
		{
			for (int y = end.y - 25; y <= end.y + 25; y++)
			{
				TilePosition test = TilePosition(x, y);
				double dist = Position(test).getDistance(BWEM::Map::Instance().Center());
				if (test.isValid() && !BWEB::BWEBUtil().overlapsAnything(test) && BWEB::BWEBUtil().isWalkable(test) && dist < distBest)
					distBest = dist, tileBest = test;
			}
		}
		testTile = tileBest;

		auto path = AStar().findPath(firstChoke, tileBest, false);
		for (auto& tile : path) {
			reservePath[tile.x][tile.y] = 1;
		}


		// Create a door for the path
		if (naturalChoke)
		{
			distBest = DBL_MAX;
			for (auto geo : naturalChoke->Geometry())
			{
				TilePosition tile = (TilePosition)geo;
				double dist = tile.getDistance(firstChoke);
				if (reservePath[tile.x][tile.y] == 1 && dist < distBest)
					tileBest = tile, distBest = dist;
			}
			areaWalls[naturalArea].setWallDoor(tileBest);
		}
	}

	bool Map::canPlaceHere(UnitType building, TilePosition here)
	{
		for (int x = here.x; x < here.x + building.tileWidth(); x++)
		{
			for (int y = here.y; y < here.y + building.tileHeight(); y++)
			{
				if (reservePath[x][y] > 0 || !Broodwar->isBuildable(TilePosition(x, y))) return false;
			}
		}
		return true;
	}

	int Map::overlapsChoke(UnitType building, TilePosition here)
	{
		int score = 0;
		for (int x = here.x; x < here.x + building.tileWidth(); x++)
		{
			for (int y = here.y; y < here.y + building.tileHeight(); y++)
			{
				if (!TilePosition(x, y).isValid()) continue;
				if (chokeTiles.find(TilePosition(x, y)) != chokeTiles.end()) score++;
			}
		}
		return score;
	}

	bool Map::wallTight(UnitType building, TilePosition here, bool fullWall)
	{
		bool L, LL, R, RR, T, TT, B, BB;
		L = LL = R = RR = T = TT = B = BB = false;
		int height = building.tileHeight() * 4;
		int width = building.tileWidth() * 4;

		if (fullWall)
		{
			if (building == UnitTypes::Protoss_Gateway)
				R = true, B = true, BB = true;
			if (building == UnitTypes::Protoss_Forge)
				L = true, R = true, T = true, B = true;
			if (building == UnitTypes::Protoss_Cybernetics_Core)
				L = true, R = true, RR = true, T = true, B = true, BB = true;

			if (building == UnitTypes::Terran_Barracks)
				T = true, R = true, RR = true, B = true;
			if (building == UnitTypes::Terran_Supply_Depot)
				L = true, R = true, T = true, B = true, BB = true;
			if (building == UnitTypes::Terran_Bunker)
				T = true, R = true, B = true;

			if (building == UnitTypes::Zerg_Hatchery || building == UnitTypes::Zerg_Evolution_Chamber)
				L = true, R = true, T = true, B = true;
		}
		else L = LL = R = RR = T = TT = B = BB = true;


		WalkPosition right = WalkPosition(here) + WalkPosition(width, 0);
		WalkPosition left = WalkPosition(here) - WalkPosition(1, 0);
		WalkPosition top = WalkPosition(here) - WalkPosition(0, 1);
		WalkPosition bottom = WalkPosition(here) + WalkPosition(0, height);

		if (R)
		{
			int dx = right.x;
			for (int dy = right.y; dy < right.y + height; dy++)
			{
				if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || BWEBUtil().overlapsWalls(TilePosition(WalkPosition(dx, dy))) || BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx, dy))) || BWEBUtil().overlapsStations(TilePosition(WalkPosition(dx, dy)))) return true;
				if (RR && WalkPosition(dx, dy).isValid() && !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx + 1, dy)).Walkable()) return true;
				if (reservePath[dx / 4][dy / 4] == 1) return true;				
			}
		}

		if (L)
		{
			int dx = left.x;
			for (int dy = left.y; dy < left.y + height; dy++)
			{
				if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || BWEBUtil().overlapsWalls(TilePosition(WalkPosition(dx, dy))) || BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx, dy))) || BWEBUtil().overlapsStations(TilePosition(WalkPosition(dx, dy)))) return true;
				if (LL && WalkPosition(dx, dy).isValid() && !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx - 1, dy)).Walkable()) return true;
				if (reservePath[dx / 4][dy / 4] == 1) return true;
			}
		}

		if (T)
		{
			int dy = top.y;
			for (int dx = top.x; dx < top.x + width; dx++)
			{
				if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || BWEBUtil().overlapsWalls(TilePosition(WalkPosition(dx, dy)))  || BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx, dy))) || BWEBUtil().overlapsStations(TilePosition(WalkPosition(dx, dy)))) return true;
				if (TT && WalkPosition(dx, dy).isValid() && !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy - 1)).Walkable()) return true;
				if (reservePath[dx / 4][dy / 4] == 1) return true;
			}
		}

		if (B)
		{
			int dy = bottom.y;
			for (int dx = bottom.x; dx < bottom.x + width; dx++)
			{
				if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || BWEBUtil().overlapsWalls(TilePosition(WalkPosition(dx, dy))) || BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx, dy))) || BWEBUtil().overlapsStations(TilePosition(WalkPosition(dx, dy)))) return true;
				if (BB && WalkPosition(dx, dy).isValid() && !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy + 1)).Walkable()) return true;
				if (reservePath[dx / 4][dy / 4] == 1) return true;
			}
		}
		return false;
	}

	bool Map::powersWall(TilePosition here)
	{
		for (auto tile : areaWalls[naturalArea].largeTiles())
		{
			bool powersThis = false;
			if (tile.y - here.y == -5 || tile.y - here.y == 4)
			{
				if (tile.x - here.x >= -4 && tile.x - here.x <= 1) powersThis = true;
			}
			if (tile.y - here.y == -4 || tile.y - here.y == 3)
			{
				if (tile.x - here.x >= -7 && tile.x - here.x <= 4) powersThis = true;
			}
			if (tile.y - here.y == -3 || tile.y - here.y == 2)
			{
				if (tile.x - here.x >= -8 && tile.x - here.x <= 5) powersThis = true;
			}
			if (tile.y - here.y >= -2 && tile.y - here.y <= 1)
			{
				if (tile.x - here.x >= -8 && tile.x - here.x <= 6) powersThis = true;
			}
			if (!powersThis) return false;
		}

		for (auto tile : areaWalls[naturalArea].mediumTiles())
		{
			bool powersThis = false;
			if (tile.y - here.y == 4)
			{
				if (tile.x - here.x >= -3 && tile.x - here.x <= 2) powersThis = true;
			}
			if (tile.y - here.y == -4 || tile.y - here.y == 3)
			{
				if (tile.x - here.x >= -6 && tile.x - here.x <= 5) powersThis = true;
			}
			if (tile.y - here.y >= -3 && tile.y - here.y <= 2)
			{
				if (tile.x - here.x >= -7 && tile.x - here.x <= 6) powersThis = true;
			}
			if (!powersThis) return false;
		}
		return true;
	}

	void Wall::insertSegment(TilePosition here, UnitType building)
	{
		if (building.tileWidth() >= 4) large.insert(here);
		else if (building.tileWidth() >= 3) medium.insert(here);
		else small.insert(here);
	}
}