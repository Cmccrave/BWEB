#include "Wall.h"
#include "AStar.h"
#include <tuple>
#include <chrono>
#include "BWEBUtil.h"

namespace BWEB
{
	void Map::findFullWall(vector<UnitType>& buildings)
	{
		chrono::steady_clock::time_point start = chrono::high_resolution_clock::now();
		TilePosition tileBest = TilePositions::Invalid;
		double distBest = DBL_MAX;
		TilePosition end = secondChoke;

		//// TODO: Find a better method to find end point of A* pathfinding
		//// Finds the end point of the pathfinding somewhere near the middle of the map
		//for (int x = end.x - 25; x <= end.x + 25; x++)
		//{
		//	for (int y = end.y - 25; y <= end.y + 25; y++)
		//	{
		//		TilePosition test = TilePosition(x, y);
		//		double dist = Position(test).getDistance(BWEM::Map::Instance().Center());
		//		if (test.isValid() && !BWEB::BWEBUtil().overlapsAnything(test) && BWEM::Map::Instance().GetTile(test).Walkable() && dist < distBest)
		//			distBest = dist, tileBest = test;
		//	}
		//}
		//testTile = (TilePosition)BWEM::Map::Instance().GetNearestArea(tileBest)->Top();
		//eraseBlock(testTile);

		const BWEM::Area* thirdArea = (naturalChoke->GetAreas().first != naturalArea) ? naturalChoke->GetAreas().first : naturalChoke->GetAreas().second;

		if (thirdArea)
			testTile = (TilePosition)thirdArea->Top();
		

		sort(buildings.begin(), buildings.end());

		// For each permutation, find walling score
		do
		{
			double current = 0;
			currentWall.clear();

			for (auto type : buildings)
			{
				// TODO: Improve A* performance re: Hannes A* is O(log(N)) while mine is O(N)
				// TODO: Store collision in the A* pathfinding for all overlap functions found in BWEBUtil()
				// Find the current hole in the wall to try and patch it
				currentHole = TilePositions::None;
				for (auto tile : BWEB::AStar().findPath(firstChoke, testTile, false))
				{
					double closestGeo = DBL_MAX;
					for (auto geo : naturalChoke->Geometry())
					{
						if (overlapsCurrentWall(tile) == UnitTypes::None && TilePosition(geo) == tile && tile.getDistance(firstChoke) * tile.getDistance(natural) < closestGeo)
							currentHole = tile, closestGeo = tile.getDistance(firstChoke) * tile.getDistance(natural);
					}
				}

				// Find wall segment, return value of placement
				// Add total score up, compare with best, if best, store set and score
				tuple<double, TilePosition> currentTuple = scoreWallSegment(type);
				current += get<0>(currentTuple);

				if (get<1>(currentTuple) != TilePositions::Invalid)
					currentWall[get<1>(currentTuple)] = type;
			}

			current = current * exp(currentWall.size());

			if (current > bestWallScore)
				bestWall = currentWall, bestWallScore = current;
		} while (next_permutation(buildings.begin(), buildings.end()));

		// For each type in best set, store into wall
		for (auto placement : bestWall)
			areaWalls[naturalArea].insertSegment(placement.first, placement.second);
		double dur = std::chrono::duration <double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();

		Broodwar << "BWEB took " << dur << "ms to find walls." << endl;
	}

	UnitType Map::overlapsCurrentWall(TilePosition here, int width, int height)
	{
		for (int x = here.x; x < here.x + width; x++)
		{
			for (int y = here.y; y < here.y + height; y++)
			{
				for (auto placement : currentWall)
				{
					TilePosition tile = placement.first;
					if (x >= tile.x && x < tile.x + placement.second.tileWidth() && y >= tile.y && y < tile.y + placement.second.tileHeight())
						return placement.second;
				}
			}
		}
		return UnitTypes::None;
	}

	tuple<double, TilePosition> Map::scoreWallSegment(UnitType building, bool tight)
	{
		double best = 0.0;
		TilePosition tileBest = TilePositions::Invalid;
		Position chokeCenter = Position(secondChoke) + Position(16, 16);
		Position natCenter = Position(natural) + Position(64, 48);

		for (int x = secondChoke.x - 16; x <= secondChoke.x + 16; x++)
		{
			for (int y = secondChoke.y - 16; y <= secondChoke.y + 16; y++)
			{
				TilePosition tile = TilePosition(x, y);
				if (!tile.isValid()) continue;
				Position center = Position(tile) + Position(building.tileWidth() * 16, building.tileHeight() * 16);

				if (overlapsCurrentWall(tile, building.tileWidth(), building.tileHeight()) != UnitTypes::None) continue;	// Testing
				if (BWEBUtil().overlapsAnything(tile, building.tileWidth(), building.tileHeight(), true) || !canPlaceHere(building, TilePosition(x, y))) continue;				// If this is overlapping anything	

				if (building == UnitTypes::Protoss_Pylon)
				{
					//if (BWEM::Map::Instance().GetArea(TilePosition(center)) != BWEM::Map::Instance().GetArea(natural)) continue;
					double distChoke = log(center.getDistance(chokeCenter));
					double distNat = center.getDistance(natCenter);
					double overallScore = distChoke / distNat;

					if (powersWall(tile) && overallScore > best)
						tileBest = tile, best = overallScore;
				}
				else
				{
					if (Broodwar->self()->getRace() != Races::Zerg && tight && !wallTight(building, tile)) continue;
					double distHole =  1.0; //(currentHole == TilePositions::None) ? 1.0 : (center.getDistance(Position(currentHole) + Position(16, 16)));
					double distNat = 1.0; // log(center.getDistance(natCenter));
					double distGeo = distToChokeGeo(center);
					double overallScore = 1.0 / (distHole * distNat * distGeo);

					if (overallScore > best)
						tileBest = tile, best = overallScore;
				}
			}
		}

		return make_tuple(best, tileBest);
	}

	double Map::distToChokeGeo(Position here)
	{
		double closest = DBL_MAX;
		for (auto &geo : naturalChoke->Geometry())
		{
			double dist = Position(geo).getDistance(here);
			if (dist < closest)
				closest = dist;
		}
		return closest;
	}

	void Map::findWallSegment(UnitType building, bool tight)
	{
		tuple<double, TilePosition> currentTuple = scoreWallSegment(building, tight);
		if (get<1>(currentTuple).isValid())
			areaWalls[naturalArea].insertSegment(get<1>(currentTuple), building);
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
				if (test.isValid() && !BWEB::BWEBUtil().overlapsAnything(test) && BWEM::Map::Instance().GetTile(test).Walkable() && dist < distBest)
					distBest = dist, tileBest = test;
			}
		}
		testTile = (TilePosition)BWEM::Map::Instance().GetNearestArea(tileBest)->Top();
		eraseBlock(testTile);

		auto path = AStar().findPath(firstChoke, testTile, false);
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

	bool Map::wallTight(UnitType building, TilePosition here)
	{
		bool L, R, T, B;
		L = R = T = B = false;
		int height = building.tileHeight() * 4;
		int width = building.tileWidth() * 4;

		int htSize = building.tileHeight() * 16;
		int wtSize = building.tileWidth() * 16;

		if (htSize - building.dimensionDown() - 1 < 16)
			B = true;
		if (htSize - building.dimensionUp() < 16)
			T = true;
		if (wtSize - building.dimensionLeft() < 16)
			L = true;
		if (wtSize - building.dimensionRight() - 1 < 16)
			R = true;

		WalkPosition right = WalkPosition(here) + WalkPosition(width, 0);
		WalkPosition left = WalkPosition(here) - WalkPosition(1, 0);
		WalkPosition top = WalkPosition(here) - WalkPosition(0, 1);
		WalkPosition bottom = WalkPosition(here) + WalkPosition(0, height);

		bool walled = false;
		for (int y = right.y; y < right.y + height; y++)
		{
			int x = right.x;
			WalkPosition w(x, y);
			TilePosition t(w);
			UnitType wallPiece = overlapsCurrentWall(t);

			// TODO: Could reduce this to a tri-state function (tightState function below)
			// return false = 0, return true = 1, return nothing = 2
			if (R && (currentWall.size() == 0 || Broodwar->self()->getRace() == Races::Protoss))
			{
				if (!w.isValid() || !Broodwar->isWalkable(w) || BWEBUtil().overlapsNeutrals(t)) return true;
				if (reservePath[t.x][t.y] == 1) return true;
			}
			if (wallPiece != UnitTypes::None)
			{
				int r = (building.tileWidth() * 16) - building.dimensionRight() - 1;
				int l = (wallPiece.tileWidth() * 16) - wallPiece.dimensionLeft();
				walled = true;

				if (l + r >= 16)
					return false;
			}
		}

		for (int y = left.y; y < left.y + height; y++)
		{
			int x = left.x;
			WalkPosition w(x, y);
			TilePosition t(w);
			UnitType wallPiece = overlapsCurrentWall(t);

			if (L && (currentWall.size() == 0 || Broodwar->self()->getRace() == Races::Protoss))
			{
				if (!w.isValid() || !Broodwar->isWalkable(w) || BWEBUtil().overlapsNeutrals(t)) return true;
				if (reservePath[t.x][t.y] == 1) return true;
			}
			if (wallPiece != UnitTypes::None)
			{
				int l = (building.tileWidth() * 16) - building.dimensionLeft();
				int r = (wallPiece.tileWidth() * 16) - wallPiece.dimensionRight() - 1;
				walled = true;

				if (l + r >= 16)
					return false;
			}
		}

		for (int x = top.x; x < top.x + width; x++)
		{
			int y = top.y;
			WalkPosition w(x, y);
			TilePosition t(w);
			UnitType wallPiece = overlapsCurrentWall(t);

			if (T && (currentWall.size() == 0 || Broodwar->self()->getRace() == Races::Protoss))
			{
				if (!w.isValid() || !Broodwar->isWalkable(w) || BWEBUtil().overlapsNeutrals(t)) return true;
				if (reservePath[t.x][t.y] == 1) return true;
			}
			if (wallPiece != UnitTypes::None)
			{
				int u = (building.tileHeight() * 16) - building.dimensionUp();
				int d = (wallPiece.tileHeight() * 16) - wallPiece.dimensionDown() - 1;
				walled = true;

				if (u + d >= 16)
					return false;
			}
		}

		for (int x = bottom.x; x < bottom.x + width; x++)
		{
			int y = bottom.y;
			WalkPosition w(x, y);
			TilePosition t(w);
			UnitType wallPiece = overlapsCurrentWall(t);

			if (B && (currentWall.size() == 0 || Broodwar->self()->getRace() == Races::Protoss))
			{
				if (!w.isValid() || !Broodwar->isWalkable(w) || BWEBUtil().overlapsNeutrals(t)) return true;
				if (reservePath[t.x][t.y] == 1) return true;
			}
			if (wallPiece != UnitTypes::None)
			{
				int d = (building.tileHeight() * 16) - building.dimensionDown() - 1;
				int u = (wallPiece.tileHeight() * 16) - wallPiece.dimensionUp();
				walled = true;

				if (u + d >= 16)
					return false;
			}
		}

		if (walled)
			return true;

		return false;
	}

	int Map::tightState(WalkPosition here, UnitType building)
	{
		return true;
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