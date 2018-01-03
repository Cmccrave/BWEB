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
						if (BWEM::Map::Instance().GetArea(TilePosition(i, j)) && BWEM::Map::Instance().GetArea(TilePosition(i, j)) == naturalArea) valid = 1;
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
						if (BWEM::Map::Instance().GetArea(TilePosition(i, j)) && BWEM::Map::Instance().GetArea(TilePosition(i, j)) == naturalArea) within = true;
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
						if (BWEBUtil().overlapsBlocks(TilePosition(i, j))) buildable = false;
						if (BWEBUtil().overlapsStations(TilePosition(i, j))) buildable = false;
						if (i >= medium.x && i < medium.x + 3 && j >= medium.y && j < medium.y + 2) buildable = false;
						if (i >= large.x && i < large.x + 4 && j >= large.y && j < large.y + 3) buildable = false;
						if (i >= natural.x && i < natural.x + 4 && j >= natural.y && j < natural.y + 3) buildable = false;
					}
				}

				if (!buildable) continue;
				if (buildable && BWEM::Map::Instance().GetArea(TilePosition(center)) == BWEM::Map::Instance().GetArea(natural) && center.getDistance(bLargeCenter) <= 160 && center.getDistance(bMediumCenter) <= 160 && (center.getDistance(Position(secondChoke)) > distance || distance == 0.0))
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
							for (auto tile : areaWalls[naturalArea].getDefenses())
							{
								if (i >= tile.x && i < tile.x + 2 && j >= tile.y && j < tile.y + 2) buildable = false;
							}

							if (!Broodwar->isBuildable(TilePosition(i, j))) buildable = false;
							if (reservePathHome[i][j] > 0) buildable = false;
							if (BWEBUtil().overlapsNeutrals(TilePosition(i, j))) buildable = false;
							if (BWEBUtil().overlapsBlocks(TilePosition(i, j))) buildable = false;
							if (BWEBUtil().overlapsStations(TilePosition(i, j))) buildable = false;
							if (i >= small.x && i < small.x + 2 && j >= small.y && j < small.y + 2) buildable = false;
							if (i >= medium.x && i < medium.x + 3 && j >= medium.y && j < medium.y + 2) buildable = false;
							if (i >= large.x && i < large.x + 4 && j >= large.y && j < large.y + 3) buildable = false;
							if (i >= natural.x && i < natural.x + 4 && j >= natural.y && j < natural.y + 3) buildable = false;
						}
					}

					if (!buildable) continue;

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
				reservePathHome[bestTile.x][bestTile.y] = 1;
			}
			else break;

			if (start.getDistance(end) < 2)	break;
		}
	}
}