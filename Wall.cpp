#include "Wall.h"
#include "AStar.h"
#include <tuple>
#include <chrono>

namespace BWEB
{
	Wall::Wall(const BWEM::Area * a, const BWEM::ChokePoint * c)
	{
		area = a;
		choke = c;
		door = TilePositions::Invalid;
	}

	void Map::createWall(vector<UnitType>& _buildings, const BWEM::Area * _area, const BWEM::ChokePoint * _choke, UnitType _tight, const vector<UnitType>& _defenses, bool _reservePath)
	{
		if (!_area || !_choke || _buildings.empty())
			return;

		// Start a clock to time walls
		chrono::high_resolution_clock clock;
		chrono::steady_clock::time_point start;
		start = chrono::high_resolution_clock::now();

		// I got sick of passing the parameters everywhere, sue me
		buildings = _buildings, area = _area, choke = _choke, tight = _tight, reservePath = _reservePath;

		// Create a new wall object
		Wall newWall(area, choke);
		const BWEM::Area* thirdArea = (choke->GetAreas().first != area) ? choke->GetAreas().first : choke->GetAreas().second;

		// Finding start and end points of our pathing
		double distBest = DBL_MAX;
		Position mc = (Position)mainChoke->Center();
		Position nc = (Position)naturalChoke->Center();

		double slope1 = double(nc.x - naturalPosition.x) / 32.0;
		double slope2 = double(nc.y - naturalPosition.y) / 32.0;

		endTile = TilePosition(nc + Position(16.0 * slope1, 16.0 * slope2));
		startTile = (TilePosition(mainChoke->Center()) + TilePosition(mainChoke->Center()) + naturalTile) / 3;

		//endTile = TilePosition(naturalChoke->Center()) - (naturalTile - TilePosition(naturalChoke->Center()));
		if (!BWEM::Map::Instance().GetArea(endTile))
		{
			for (int x = endTile.x - 2; x < endTile.x + 2; x++)
			{
				for (int y = endTile.y - 2; y < endTile.y + 2; y++)
				{
					TilePosition t(x, y);
					double dist = t.getDistance(startTile);
					if (BWEM::Map::Instance().GetArea(t) && dist < distBest)
						endTile = TilePosition(x, y), distBest = dist;
				}
			}
		}

		// Iterate pieces, try to find best location
		if (iteratePieces())
		{
			for (auto& placement : bestWall)
			{
				newWall.insertSegment(placement.first, placement.second);
				addOverlap(placement.first, placement.second.tileWidth(), placement.second.tileHeight());
			}
			currentWall = bestWall;
		}

		findCurrentHole();
		if (reservePath) 
		{			
			for (auto& tile : currentPath)
			{
				reserveGrid[tile.x][tile.y] = 1;
				if (!BWEM::Map::Instance().GetArea(tile))
					newWall.setWallDoor(tile);
			}
		}

		// Set the Walls centroid
		Position centroid;
		for (auto piece : currentWall)
			centroid += (Position)piece.first + Position(piece.second.tileWidth() * 16, piece.second.tileHeight() * 16);
		newWall.setCentroid(centroid / currentWall.size());

		// Add wall defenses if requested
		if (!_defenses.empty())
			addWallDefenses(_defenses, newWall);

		// Push wall into the vector
		walls.push_back(newWall);

		// Print time
		double dur = chrono::duration <double, milli>(chrono::high_resolution_clock::now() - start).count();
		Broodwar << "Wall time: " << dur << endl;
		return;
	}

	bool Map::iteratePieces()
	{
		// Sort functionality for Pylons by Hannes
		if (find(buildings.begin(), buildings.end(), UnitTypes::Protoss_Pylon) != buildings.end())
		{
			sort(buildings.begin(), buildings.end(), [](UnitType l, UnitType r){ return (l == UnitTypes::Protoss_Pylon) < (r == UnitTypes::Protoss_Pylon); }); // Moves pylons to end
			sort(buildings.begin(), find(buildings.begin(), buildings.end(), UnitTypes::Protoss_Pylon)); // Sorts everything before pylons
		}
		else		
			sort(buildings.begin(), buildings.end());		

		do {
			currentWall.clear();
			typeIterator = buildings.begin();
			checkPiece((TilePosition)choke->Center());
		} while (next_permutation(buildings.begin(), find(buildings.begin(), buildings.end(), UnitTypes::Protoss_Pylon)));

		return !bestWall.empty();
	}

	bool Map::checkPiece(TilePosition start)
	{
		UnitType parentType = overlapsCurrentWall(start);
		UnitType currentType = (*typeIterator);
		TilePosition parentSize;
		TilePosition currentSize;
		int tightnessFactor = tight == UnitTypes::None ? 32 : min(tight.width(), tight.height());

		if (currentType.isValid())
			currentSize = ((*typeIterator).tileSize());

		// If we have a previous piece, only iterate the pieces around it
		if (parentType != UnitTypes::None && currentType != UnitTypes::Protoss_Pylon)
		{
			parentSize = (parentType.tileSize());

			int parentRight = (parentSize.x * 16) - parentType.dimensionRight() - 1;
			int currentLeft = (currentSize.x * 16) - (*typeIterator).dimensionLeft();
			int parentLeft = (parentSize.x * 16) - parentType.dimensionLeft();
			int currentRight = (currentSize.x * 16) - (*typeIterator).dimensionRight() - 1;

			// Left edge and right edge
			if (parentRight + currentLeft < tightnessFactor || parentLeft + currentRight < tightnessFactor)
			{
				int xLeft = start.x - currentSize.x;
				int xRight = start.x + parentSize.x;
				for (int y = 1 + start.y - currentSize.y; y < start.y + parentSize.y; y++)
				{
					TilePosition left(xLeft, y); TilePosition right(xRight, y);

					if (visited[currentType].location[left.x][left.y] != 2)
					{
						if (identicalPiece(start, parentType, left, currentType) || (parentRight + currentLeft < tightnessFactor && testPiece(left)))
							placePiece(left);
					}
					if (visited[currentType].location[right.x][right.y] != 2)
					{
						if (identicalPiece(start, parentType, right, currentType) || (parentLeft + currentRight < tightnessFactor && testPiece(right)))
							placePiece(right);
					}
				}
			}

			// Top and bottom edge
			int parentTop = (parentSize.y * 16) - parentType.dimensionUp();
			int currentBottom = (currentSize.y * 16) - (*typeIterator).dimensionDown() - 1;
			int parentBottom = (parentSize.y * 16) - parentType.dimensionDown() - 1;
			int currentTop = (currentSize.y * 16) - (*typeIterator).dimensionUp();

			if (parentTop + currentBottom < tightnessFactor || parentBottom + currentTop < tightnessFactor)
			{
				int yTop = start.y - currentSize.y;
				int yBottom = start.y + parentSize.y;
				for (int x = 1 + start.x - currentSize.x; x < start.x + parentSize.x; x++)
				{
					TilePosition top(x, yTop); TilePosition bot(x, yBottom);
					if (visited[currentType].location[top.x][top.y] != 2)
					{
						if (identicalPiece(start, parentType, top, currentType) || (parentTop + currentBottom < tightnessFactor && testPiece(top)))
							placePiece(top);
					}
					if (visited[currentType].location[bot.x][bot.y] != 2)
					{
						if (identicalPiece(start, parentType, bot, currentType) || (parentBottom + currentTop < tightnessFactor && testPiece(bot)))
							placePiece(bot);
					}
				}
			}
		}
		// Otherwise we need to start the choke center
		else
		{
			for (int x = start.x - 6; x < start.x + 6; x++){
				for (int y = start.y - 6; y < start.y + 6; y++){

					TilePosition t(x, y);
					parentSame = false, currentSame = false;
					if ((isWallTight((*typeIterator), t) || currentType == UnitTypes::Protoss_Pylon) && testPiece(t))
						placePiece(t);
				}
			}
		}
		return true;
	}

	bool Map::identicalPiece(TilePosition parentTile, UnitType parentType, TilePosition currentTile, UnitType currentType)
	{
		// Want to store that it is physically possible to build this piece here so we don't waste time checking
		parentSame = false, currentSame = false;
		if (parentType != UnitTypes::None)
		{
			for (auto& node : visited)
			{
				auto& v = node.second;
				if (node.first == parentType && v.location[parentTile.x][parentTile.y] == 1)
					parentSame = true;
				if (node.first == currentType && v.location[currentTile.x][currentTile.y] == 1)
					currentSame = true;
				if (parentSame && currentSame)
					return true;
			}
		}
		return false;
	}

	bool Map::testPiece(TilePosition t)
	{
		// If this is not a valid type, not a valid tile, overlaps the current wall, overlaps anything, isn't within the area passed in, isn't placeable or isn't wall tight
		if ((*typeIterator) == UnitTypes::Protoss_Pylon && !isPoweringWall(t)) return false;
		if (currentSame && parentSame) return true;
		if (!(*typeIterator).isValid() || !t.isValid() || overlapsCurrentWall(t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight()) != UnitTypes::None) return false;

		// If we can't place here, regardless of what's currently placed, set as impossible to place
		if (overlapsAnything(t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight(), true) || tilesWithinArea(area, t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight()) == 0 || !isPlaceable(*typeIterator, t))
		{
			visited[(*typeIterator)].location[t.x][t.y] = 2;
			return false;
		}
		return true;
	}

	bool Map::placePiece(TilePosition t)
	{
		// If we haven't tried to place one here, set visited
		if (!currentSame)
			visited[(*typeIterator)].location[t.x][t.y] = 1;

		currentWall[t] = *typeIterator, typeIterator++;

		// If we have placed all pieces
		if (typeIterator == buildings.end())
		{
			// Try to find a hole in the wall
			findCurrentHole();
			double dist = 0.0;
			for (auto& piece : currentWall)
			{
				if (piece.second == UnitTypes::Protoss_Pylon || piece.second.getRace() == Races::Zerg)
					dist += piece.first.getDistance(naturalTile);
				else
					dist += piece.first.getDistance((TilePosition)choke->Center());
			}

			double score = currentPathSize / dist;
			if (score > bestWallScore && (!reservePath || currentHole != TilePositions::None))
			{
				bestWall = currentWall;
				bestWallScore = score;
			}
		}

		// Else check for another
		else
			checkPiece(t);

		// Erase current tile and reduce iterator
		currentWall.erase(t);
		typeIterator--;
		return true;
	}

	void Map::findCurrentHole()
	{
		// Reset hole and get a new path
		currentHole = TilePositions::None;
		currentPath = BWEB::AStar().findPath(startTile, endTile, true);
		currentPathSize = (double)currentPath.size();

		// Quick check to see if the path contains our end point
		if (find(currentPath.begin(), currentPath.end(), endTile) == currentPath.end())
		{
			currentHole = TilePositions::None;
			currentPathSize = 500.0;
			return;
		}

		// Otherwise iterate all tiles and locate the hole
		for (auto& tile : currentPath)
		{
			double closestGeo = DBL_MAX;
			for (auto& geo : choke->Geometry())
			{
				if (overlapsCurrentWall(tile) == UnitTypes::None && TilePosition(geo) == tile && tile.getDistance(startTile) < closestGeo)
					currentHole = tile, closestGeo = tile.getDistance(startTile);
			}
		}
	}

	UnitType Map::overlapsCurrentWall(TilePosition here, int width, int height)
	{
		for (int x = here.x; x < here.x + width; x++)
		{
			for (int y = here.y; y < here.y + height; y++)
			{
				for (auto& placement : currentWall)
				{
					TilePosition tile = placement.first;
					if (x >= tile.x && x < tile.x + placement.second.tileWidth() && y >= tile.y && y < tile.y + placement.second.tileHeight())
						return placement.second;
				}
			}
		}
		return UnitTypes::None;
	}

	void Map::addToWall(UnitType building, Wall& wall, UnitType tight)
	{
		double distance = DBL_MAX;
		TilePosition tileBest = TilePositions::Invalid;
		TilePosition start(wall.getCentroid());

		for (int x = start.x - 6; x <= start.x + 6; x++)
		{
			for (int y = start.y - 6; y <= start.y + 6; y++)
			{
				TilePosition t(x, y);
				Position p(t);

				if (!TilePosition(x, y).isValid()) continue;
				if (overlapsAnything(TilePosition(x, y), 2, 2, true) || overlapsWalls(t)) continue;
				if (!isPlaceable(building, TilePosition(x, y)) || overlapsCurrentWall(TilePosition(x, y), 2, 2) != UnitTypes::None) continue;

				Position center = Position(t) + Position(32, 32);
				Position hold = Position(start);
				double dist = center.getDistance(hold);

				if (BWEM::Map::Instance().GetArea(TilePosition(center)) != wall.getArea()) continue;
				if (p.getDistance(Position(endTile)) < wall.getCentroid().getDistance(Position(endTile))) continue;

				if (dist < distance)
					tileBest = TilePosition(x, y), distance = dist;
			}
		}

		if (tileBest.isValid())
		{
			currentWall[tileBest] = building;
			wall.insertDefense(tileBest);
			start = tileBest;
		}
	}

	void Map::addWallDefenses(const vector<UnitType>& types, Wall& wall)
	{
		for (auto& building : types)		
			addToWall(building, wall, tight);
	}
	
	void Map::findPath()
	{
		//// Use the closest stations resource centroid
		//TilePosition pathStart = startTile;
		////const Station* check = getClosestStation(startTile);
		////if (check)
		////	pathStart = (TilePosition)check->ResourceCentroid();

		//for (auto& tile : AStar().findPath(pathStart, endTile, false))
		//	reservePath[tile.x][tile.y] = 1;

		//double distBest = DBL_MAX;
		//TilePosition tileBest(TilePositions::Invalid);
		//for (auto &geo : naturalChoke->Geometry())
		//{
		//	TilePosition tile = (TilePosition)geo;
		//	double dist = tile.getDistance((TilePosition)naturalChoke->Center());
		//	if (reservePath[tile.x][tile.y] == 1 && dist < distBest)
		//		tileBest = tile, distBest = dist;
		//}
	}

	bool Map::isPoweringWall(TilePosition here)
	{
		for (auto& piece : currentWall)
		{
			TilePosition tile(piece.first);
			UnitType type(piece.second);
			if (type.tileWidth() == 4)
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
			else
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
		}
		return true;
	}

	void Wall::insertSegment(TilePosition here, UnitType building)
	{
		if (building.tileWidth() >= 4) large.insert(here);
		else if (building.tileWidth() >= 3) medium.insert(here);
		else small.insert(here);
	}

	const Wall * Map::getClosestWall(TilePosition here) const
	{
		double distBest = DBL_MAX;
		const Wall * bestWall = nullptr;
		for (auto& wall : walls)
		{
			double dist = here.getDistance((TilePosition)wall.getChokePoint()->Center());

			if (dist < distBest)
			{
				distBest = dist;
				bestWall = &wall;
			}
		}
		return bestWall;
	}

	const Wall* Map::getWall(const BWEM::Area * area, const BWEM::ChokePoint * choke)
	{
		if (!area && !choke) return nullptr;

		for (auto& wall : walls)
		{
			if ((!area || wall.getArea() == area) && (!choke || wall.getChokePoint() == choke))
				return &wall;
		}
		return nullptr;
	}

	bool Map::isWallTight(UnitType building, TilePosition here)
	{
		bool L, R, T, B;
		L = R = T = B = false;
		int height = building.tileHeight() * 4;
		int width = building.tileWidth() * 4;
		int htSize = building.tileHeight() * 16;
		int wtSize = building.tileWidth() * 16;

		if (tight != UnitTypes::None)
		{
			if (htSize - building.dimensionDown() - 1 < 16)
				B = true;
			if (htSize - building.dimensionUp() < 16)
				T = true;
			if (wtSize - building.dimensionLeft() < 16)
				L = true;
			if (wtSize - building.dimensionRight() - 1 < 16)
				R = true;
		}
		else
		{
			L = R = T = B = true;
		}

		WalkPosition right = WalkPosition(here) + WalkPosition(width, 0);
		WalkPosition left = WalkPosition(here) - WalkPosition(1, 0);
		WalkPosition top = WalkPosition(here) - WalkPosition(0, 1);
		WalkPosition bottom = WalkPosition(here) + WalkPosition(0, height);

		for (int y = right.y; y < right.y + height; y++)
		{
			int x = right.x;
			WalkPosition w(x, y);
			TilePosition t(w);
			if (R && (!w.isValid() || !Broodwar->isWalkable(w) || overlapsNeutrals(t))) return true;
		}

		for (int y = left.y; y < left.y + height; y++)
		{
			int x = left.x;
			WalkPosition w(x, y);
			TilePosition t(w);
			if (L && (!w.isValid() || !Broodwar->isWalkable(w) || overlapsNeutrals(t))) return true;
		}

		for (int x = top.x; x < top.x + width; x++)
		{
			int y = top.y;
			WalkPosition w(x, y);
			TilePosition t(w);
			if (T && (!w.isValid() || !Broodwar->isWalkable(w) || overlapsNeutrals(t))) return true;
		}

		for (int x = bottom.x; x < bottom.x + width; x++)
		{
			int y = bottom.y;
			WalkPosition w(x, y);
			TilePosition t(w);

			if (B && (!w.isValid() || !Broodwar->isWalkable(w) || overlapsNeutrals(t))) return true;
		}
		return false;
	}
}