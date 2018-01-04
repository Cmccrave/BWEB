#include "Wall.h"

namespace BWEB
{
	void Map::findWalls()
	{
		findLargeWall();
		findMediumWall();
		findSmallWall();
		findPath();
		findWallDefenses();
	}

	void Map::findLargeWall()
	{
		double distance = DBL_MAX;
		TilePosition large;

		// Large Building placement
		for (int x = secondChoke.x - 20; x <= secondChoke.x + 20; x++)
		{
			for (int y = secondChoke.y - 20; y <= secondChoke.y + 20; y++)
			{
				// Missing: within nat area, buildable
				if (!TilePosition(x, y).isValid()) continue;
				if (BWEBUtil().overlapsAnything(TilePosition(x, y), 4, 3)) continue;

				Position center = Position(TilePosition(x, y)) + Position(64, 48);
				Position chokeCenter = Position(secondChoke) + Position(16, 16);
				int valid = 0;
				int dx = x + 4;

				for (int dy = y; dy < y + 3; dy++)				
					if (!BWEBUtil().isWalkable(TilePosition(dx, dy))) valid++;				

				int dy = y + 3;
				for (int dx = x; dx < x + 4; dx++)			
					if (!BWEBUtil().isWalkable(TilePosition(dx, dy))) valid++;			

				double distNat = center.getDistance(Position(natural));
				double distChoke = center.getDistance(chokeCenter);
				if (valid >= 2 && distNat <= 512 && distChoke < distance)
					large = TilePosition(x, y), distance = distChoke;
			}
		}
		areaWalls[naturalArea].setLargeWall(large);
	}

	void Map::findMediumWall()
	{
		double distance = DBL_MAX;
		TilePosition medium;
		TilePosition large = areaWalls[naturalArea].getLargeWall();
		// Medium Building placement
		for (int x = secondChoke.x - 20; x <= secondChoke.x + 20; x++)
		{
			for (int y = secondChoke.y - 20; y <= secondChoke.y + 20; y++)
			{
				// Missing: within nat area, buildable
				if (!TilePosition(x, y).isValid()) continue;
				if (BWEBUtil().overlapsAnything(TilePosition(x, y), 3, 2)) continue;

				Position center = Position(TilePosition(x, y)) + Position(48, 32);
				Position chokeCenter = Position(secondChoke) + Position(16, 16);				
				int valid = 0;
				int dx = x - 1;

				for (int dy = y; dy < y + 2; dy++)				
					if (!BWEBUtil().isWalkable(TilePosition(dx, dy))) valid++;
			
				int dy = y - 1;
				for (int dx = x; dx < x + 3; dx++)				
					if (!BWEBUtil().isWalkable(TilePosition(dx, dy))) valid++;
				
				if (valid >= 1 && center.getDistance(Position(natural)) <= 512 && center.getDistance(chokeCenter) < distance)
					medium = TilePosition(x, y), distance = center.getDistance(chokeCenter);
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
				// Missing: within nat area, buildable, not on reserve path
				if (!TilePosition(x, y).isValid()) continue;
				if (TilePosition(x, y) == secondChoke) continue;
				if (BWEBUtil().overlapsAnything(TilePosition(x, y), 2, 2)) continue;

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
			TilePosition small = areaWalls[naturalArea].getSmallWall();
			TilePosition medium = areaWalls[naturalArea].getMediumWall();
			TilePosition large = areaWalls[naturalArea].getLargeWall();
			TilePosition start = (large + medium) / 2;
			for (int x = start.x - 20; x <= start.x + 20; x++)
			{
				for (int y = start.y - 20; y <= start.y + 20; y++)
				{
					// Missing: within nat area, buildable, not on reserve path
					if (!TilePosition(x, y).isValid()) continue;
					if (TilePosition(x, y) == secondChoke) continue;					
					if (BWEBUtil().overlapsAnything(TilePosition(x, y), 2, 2)) continue;

					Position center = Position(TilePosition(x, y)) + Position(32, 32);
					Position hold = Position(secondChoke);
					double dist = center.getDistance(hold);

					if (BWEM::Map::Instance().GetArea(TilePosition(center)) != naturalArea) continue;
					if (dist >= 96 && dist <= 320 && dist < distance)
						best = TilePosition(x, y), distance = dist;
				}
			}
			if (best.isValid())
			{
				areaWalls[naturalArea].insertDefense(best);
			}
			else return;
		}
	}

	void Map::findPath()
	{
		TilePosition small = areaWalls[naturalArea].getSmallWall();
		TilePosition medium = areaWalls[naturalArea].getMediumWall();
		TilePosition large = areaWalls[naturalArea].getLargeWall();

		// Create reserve path home
		TilePosition end = small + TilePosition(1, 1);
		TilePosition middle = (large + medium) / 2;
		TilePosition start = (large + medium) / 2;
		double distance = DBL_MAX;
		end = firstChoke;
		int range = int(firstChoke.getDistance(secondChoke));

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
				if (BWEBUtil().overlapsStations(tile)) continue;
				if (BWEM::Map::Instance().GetArea(Broodwar->self()->getStartLocation()) == BWEM::Map::Instance().GetArea(tile)) continue;
				if (!BWEBUtil().isWalkable(tile)) continue;

				double dist = Map::Instance().getGroundDistance(Position(tile), Position(end));
				if (dist < distBest)
				{
					distBest = dist;
					bestTile = tile;
				}
			}

			if (bestTile.isValid())
			{
				start = bestTile;
				reservePath[bestTile.x][bestTile.y] = 1;
			}
			else break;

			if (start.getDistance(end) < 2)	break;
		}
	}
}