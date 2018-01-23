#include "Wall.h"

namespace BWEB
{
	void Map::findWalls()
	{
		if (naturalChoke)
		{
			for (auto tile : naturalChoke->Geometry())
			{
				chokeTiles.insert(TilePosition(tile));
			}
		}

		findLargeWall();
		findPath();
		findMediumWall();
		findSmallWall();
		findWallDefenses();
	}

	void Map::findLargeWall()
	{
		double best = 0.0;
		TilePosition large;
		Position chokeCenter = Position(secondChoke) + Position(16, 16);
		Position natCenter = Position(natural) + Position(64, 48);

		// Large Building placement
		for (int x = secondChoke.x - 16; x <= secondChoke.x + 16; x++)
		{
			for (int y = secondChoke.y - 16; y <= secondChoke.y + 16; y++)
			{
				int score = 0;
				if (!TilePosition(x, y).isValid()) continue;
				if (BWEBUtil().overlapsAnything(TilePosition(x, y), 4, 3)) continue;
				if (!canPlaceHere(UnitTypes::Protoss_Gateway, TilePosition(x, y))) continue;

				score += BWEBUtil().insideNatArea(TilePosition(x, y), 4, 3);
				score += overlapsChoke(UnitTypes::Protoss_Gateway, TilePosition(x, y));

				Position center = Position(TilePosition(x, y)) + Position(64, 48);
				
				int valid = 0;

				WalkPosition right = WalkPosition(TilePosition(x + 4, y));
				WalkPosition bottom = WalkPosition(TilePosition(x, y + 3));

				int dx = right.x;
				for (int dy = right.y; dy < right.y + 12; dy++)
				{
					if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || (score > 1 && BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx,dy))))) valid++;
				}

				int dy = bottom.y;
				for (int dx = bottom.x; dx < bottom.x + 16; dx++)
				{
					if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy + 1)).Walkable() || (score > 1 && BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx, dy))))) valid++;
				}

				double distChoke = center.getDistance(chokeCenter);
				double distNat = center.getDistance(natCenter);
				double overallScore = double(score) / (distNat * distChoke);
				if (valid >= 1 && overallScore > best)
					large = TilePosition(x, y), best = overallScore;
			}
		}
		areaWalls[naturalArea].setLargeWall(large);
	}

	void Map::findMediumWall()
	{
		double best = 0.0;
		TilePosition medium;
		Position chokeCenter = Position(secondChoke) + Position(16, 16);
		Position natCenter = Position(natural) + Position(64, 48);
		// Medium Building placement
		for (int x = secondChoke.x - 16; x <= secondChoke.x + 16; x++)
		{
			for (int y = secondChoke.y - 16; y <= secondChoke.y + 16; y++)
			{
				int score = 0;
				if (!TilePosition(x, y).isValid()) continue;
				if (BWEBUtil().overlapsAnything(TilePosition(x, y), 3, 2)) continue;
				if (!canPlaceHere(UnitTypes::Protoss_Forge, TilePosition(x, y))) continue;

				score += BWEBUtil().insideNatArea(TilePosition(x, y), 3, 2);
				score += overlapsChoke(UnitTypes::Protoss_Forge, TilePosition(x, y));

				Position center = Position(TilePosition(x, y)) + Position(48, 32);
				
				int valid = 0;

				WalkPosition left = WalkPosition(TilePosition(x, y)) - WalkPosition(1, 0);
				WalkPosition top = WalkPosition(TilePosition(x, y)) - WalkPosition(0, 1);
				WalkPosition right = WalkPosition(TilePosition(x + 3, y));
				WalkPosition bottom = WalkPosition(TilePosition(x, y + 2));

				int dx = left.x;
				for (int dy = left.y; dy < left.y + 8; dy++)
				{
					if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || BWEBUtil().overlapsWalls(TilePosition(WalkPosition(dx, dy))) || (score > 1 && BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx, dy))))) valid++;
				}

				int dy = top.y;
				for (int dx = top.x; dx < top.x + 12; dx++)
				{
					if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || BWEBUtil().overlapsWalls(TilePosition(WalkPosition(dx, dy))) || (score > 1 && BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx, dy))))) valid++;
				}

				dx = right.x;
				for (int dy = right.y; dy < right.y + 8; dy++)
				{
					if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || BWEBUtil().overlapsWalls(TilePosition(WalkPosition(dx, dy))) || (score > 1 && BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx, dy))))) valid++;
				}

				dy = bottom.y;
				for (int dx = bottom.x; dx < bottom.x + 12; dx++)
				{
					if (!WalkPosition(dx, dy).isValid() || !BWEM::Map::Instance().GetMiniTile(WalkPosition(dx, dy)).Walkable() || BWEBUtil().overlapsWalls(TilePosition(WalkPosition(dx, dy))) || (score > 1 && BWEBUtil().overlapsNeutrals(TilePosition(WalkPosition(dx, dy))))) valid++;
				}

				double distChoke = center.getDistance(chokeCenter);
				double distNat = center.getDistance(natCenter);
				double overallScore = double(score) / (distNat * distChoke);
				if (valid >= 1 && overallScore > best)
					medium = TilePosition(x, y), best = overallScore;
			}
		}
		areaWalls[naturalArea].setMediumWall(medium);
	}

	void Map::findSmallWall()
	{
		// Pylon placement 
		double distance = 0.0;
		TilePosition small;
		TilePosition medium = areaWalls[naturalArea].getMediumWall();
		TilePosition large = areaWalls[naturalArea].getLargeWall();
		for (int x = natural.x - 20; x <= natural.x + 20; x++)
		{
			for (int y = natural.y - 20; y <= natural.y + 20; y++)
			{
				// Missing: within nat area
				if (!TilePosition(x, y).isValid() || TilePosition(x, y) == secondChoke) continue;
				if (BWEBUtil().overlapsAnything(TilePosition(x, y), 2, 2)) continue;
				if (!canPlaceHere(UnitTypes::Protoss_Pylon, TilePosition(x, y))) continue;
				if (!BWEBUtil().insideNatArea(TilePosition(x, y), 2, 2)) continue;

				Position center = Position(TilePosition(x, y)) + Position(32, 32);
				Position bLargeCenter = Position(large) + Position(64, 48);
				Position bMediumCenter = Position(medium) + Position(48, 32);

				if (BWEM::Map::Instance().GetArea(TilePosition(center)) == BWEM::Map::Instance().GetArea(natural) && center.getDistance(bLargeCenter) <= 160 && center.getDistance(bMediumCenter) <= 160 && (center.getDistance(Position(secondChoke)) > distance || distance == 0.0))
					small = TilePosition(x, y), distance = center.getDistance(Position(secondChoke));
			}
		}
		areaWalls[naturalArea].setSmallWall(small);
	}

	void Map::findWallDefenses()
	{
		for (int i = 0; i < 6; i++)
		{
			double distance = DBL_MAX;
			TilePosition best = TilePositions::Invalid;
			TilePosition start = (areaWalls[naturalArea].getLargeWall() + areaWalls[naturalArea].getMediumWall()) / 2;
			for (int x = start.x - 20; x <= start.x + 20; x++)
			{
				for (int y = start.y - 20; y <= start.y + 20; y++)
				{
					// Missing: within nat area, buildable, not on reserve path
					if (!TilePosition(x, y).isValid() || TilePosition(x, y) == secondChoke) continue;
					if (BWEBUtil().overlapsAnything(TilePosition(x, y), 2, 2)) continue;
					if (!canPlaceHere(UnitTypes::Protoss_Photon_Cannon, TilePosition(x, y))) continue;

					Position center = Position(TilePosition(x, y)) + Position(32, 32);
					Position hold = Position(secondChoke);
					double dist = center.getDistance(hold);

					if (BWEM::Map::Instance().GetArea(TilePosition(center)) != naturalArea) continue;
					if (dist >= 128 && dist <= 320 && dist < distance)
						best = TilePosition(x, y), distance = dist;
				}
			}
			if (best.isValid()) areaWalls[naturalArea].insertDefense(best);
			else return;
		}
	}

	void Map::findPath()
	{
		TilePosition small = areaWalls[naturalArea].getSmallWall();
		TilePosition medium = areaWalls[naturalArea].getMediumWall();
		TilePosition large = areaWalls[naturalArea].getLargeWall();
		TilePosition end = natural;
		TilePosition start, middle;
		set<TilePosition> testedTiles;

		double distBest = 0.0;
		if (naturalChoke)
		{
			for (auto tile : naturalChoke->Geometry())
			{
				double dist = tile.getDistance(WalkPosition(large));
				if (dist > distBest && !BWEBUtil().overlapsAnything(TilePosition(tile)))
					start = TilePosition(tile), distBest = dist;
			}
		}
		else start = (medium + large) / 2;

		areaWalls[naturalArea].setWallDoor(start);

		middle = start;
		for (int i = 0; i <= 20; i++)
		{
			set<TilePosition> testCases;
			testCases.insert(TilePosition(middle.x - 1, middle.y));
			testCases.insert(TilePosition(middle.x + 1, middle.y));
			testCases.insert(TilePosition(middle.x, middle.y - 1));
			testCases.insert(TilePosition(middle.x, middle.y + 1));

			TilePosition bestTile;
			distBest = DBL_MAX;
			for (auto tile : testCases)
			{
				if (!tile.isValid() || BWEBUtil().overlapsAnything(tile) || !BWEBUtil().isWalkable(tile) || reservePath[tile.x][tile.y] != 0) continue;
				if (BWEM::Map::Instance().GetArea(tStart) == BWEM::Map::Instance().GetArea(tile)) continue;

				double dist = tile.getDistance(end);
				if (dist < distBest)
				{
					distBest = dist;
					bestTile = tile;
				}
			}

			if (bestTile.isValid())
			{
				middle = bestTile;
				reservePath[bestTile.x][bestTile.y] = 1;
				if (i > 4)
				{
					end = firstChoke;
				}
			}
			else break;
		}

		end = TilePosition(BWEM::Map::Instance().GetNearestArea(TilePosition(BWEM::Map::Instance().Center()))->Top());
		middle = start;
		for (int i = 0; i <= 20; i++)
		{
			set<TilePosition> testCases;
			testCases.insert(TilePosition(middle.x - 1, middle.y));
			testCases.insert(TilePosition(middle.x + 1, middle.y));
			testCases.insert(TilePosition(middle.x, middle.y - 1));
			testCases.insert(TilePosition(middle.x, middle.y + 1));

			TilePosition bestTile;
			distBest = DBL_MAX;
			for (auto tile : testCases)
			{
				if (!tile.isValid() || BWEBUtil().overlapsAnything(tile) || !BWEBUtil().isWalkable(tile)) continue;
				if (BWEM::Map::Instance().GetArea(tStart) == BWEM::Map::Instance().GetArea(tile)) continue;

				double dist = Map::Instance().getGroundDistance(Position(tile), Position(end));
				if (dist < distBest)
				{
					distBest = dist;
					bestTile = tile;
				}
			}

			if (bestTile.isValid())
			{
				middle = bestTile;
				reservePath[bestTile.x][bestTile.y] = 1;
			}
			else break;
		}

		reservePath[start.x][start.y] = 1;
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
}