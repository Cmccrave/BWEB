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
		wallDoor = TilePositions::Invalid;
	}

	void Map::createWall(vector<UnitType>& _buildings, const BWEM::Area * _area, const BWEM::ChokePoint * _choke, UnitType _tight, const vector<UnitType>& _defenses)
	{
		if (!_area || !_choke || _buildings.empty())
			return;

		// I got sick of passing the parameters everywhere, sue me
		buildings = _buildings, area = _area, choke = _choke, tight = _tight;

		Wall newWall(area, choke);
		double distBest = DBL_MAX;
		const BWEM::Area* thirdArea = (choke->GetAreas().first != area) ? choke->GetAreas().first : choke->GetAreas().second;

		// Find our end position for A* pathfinding
		endTile = thirdArea != nullptr ? (TilePosition)thirdArea->Top() : TilePositions::Invalid;
		startTile = (TilePosition)mainArea->Top();

		// Sort buildings first
		sort(buildings.begin(), buildings.end());

		// If we don't care about tightness, just get best scored wall
		std::chrono::high_resolution_clock clock;
		chrono::steady_clock::time_point start;
		start = chrono::high_resolution_clock::now();

		// Iterate pieces, try to find best location
		if (iteratePieces())
		{
			for (auto& placement : bestWall)
				newWall.insertSegment(placement.first, placement.second);
		}

		currentWall = bestWall;

		if (!_defenses.empty())
			addWallDefenses(_defenses, newWall);
		walls.push_back(newWall);


		double dur = std::chrono::duration <double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
		Broodwar << "Wall time: " << dur << endl;
		Broodwar << visited.size() << endl;
		return;
	}

	bool Map::iteratePieces()
	{
		// Sort functionality for Pylons by Hannes
		sort(buildings.begin(), buildings.end(), [](UnitType l, UnitType r){ return (l == UnitTypes::Protoss_Pylon) < (r == UnitTypes::Protoss_Pylon); }); // Moves pylons to end
		sort(buildings.begin(), find(buildings.begin(), buildings.end(), UnitTypes::Protoss_Pylon)); // Sorts everything before pylons

		do {
			currentWall.clear();
			typeIterator = buildings.begin();
			checkPiece((TilePosition)choke->Center());
		} while (next_permutation(buildings.begin(), buildings.end()));

		return !bestWall.empty();
	}

	bool Map::checkPiece(TilePosition start)
	{
		UnitType parentType = overlapsCurrentWall(start);
		UnitType currentType = (*typeIterator);
		TilePosition parentSize;
		TilePosition currentSize;

		if (currentType.isValid())
			currentSize = ((*typeIterator).tileSize());

		// If we have a previous piece, only iterate the pieces around it
		if (parentType != UnitTypes::None)
		{
			parentSize = (parentType.tileSize());

			int parentRight = (parentSize.x * 16) - parentType.dimensionRight() - 1;
			int currentLeft = (currentSize.x * 16) - (*typeIterator).dimensionLeft();
			int parentLeft = (parentSize.x * 16) - parentType.dimensionLeft();
			int currentRight = (currentSize.x * 16) - (*typeIterator).dimensionRight() - 1;

			// Left edge and right edge
			if ((parentRight + currentLeft < 16) || (parentLeft + currentRight < 16) || tight == UnitTypes::None)
			{
				int xLeft = start.x - currentSize.x;
				int xRight = start.x + parentSize.x;
				for (int y = 1 + start.y - currentSize.y; y < start.y + parentSize.y; y++)
				{
					TilePosition left(xLeft, y); TilePosition right(xRight, y);
					if (identicalPiece(start, parentType, left, currentType) || ((parentRight + currentLeft < 16 || tight == UnitTypes::None) && testPiece(left)))
						placePiece(left);
					if (identicalPiece(start, parentType, right, currentType) || ((parentLeft + currentRight < 16 || tight == UnitTypes::None) && testPiece(right)))
						placePiece(right);
				}
			}

			// Top and bottom edge
			int parentTop = (parentSize.y * 16) - parentType.dimensionUp();
			int currentBottom = (currentSize.y * 16) - (*typeIterator).dimensionDown() - 1;
			int parentBottom = (parentSize.y * 16) - parentType.dimensionDown() - 1;
			int currentTop = (currentSize.y * 16) - (*typeIterator).dimensionUp();

			if ((parentTop + currentBottom < 16) || (parentBottom + currentTop < 16) || tight == UnitTypes::None)
			{
				int yTop = start.y - currentSize.y;
				int yBottom = start.y + parentSize.y;
				for (int x = 1 + start.x - currentSize.x; x < start.x + parentSize.x; x++)
				{
					TilePosition top(x, yTop); TilePosition bot(x, yBottom);
					if (identicalPiece(start, parentType, top, currentType) || ((parentTop + currentBottom < 16 || tight == UnitTypes::None) && testPiece(top)))
						placePiece(top);
					if (identicalPiece(start, parentType, bot, currentType) || ((parentBottom + currentTop < 16 || tight == UnitTypes::None) && testPiece(bot)))
						placePiece(bot);
				}
			}
		}
		// Otherwise we need to start the choke center
		else
		{
			TilePosition chokeCenter(choke->Center());
			for (int x = chokeCenter.x - 6; x < chokeCenter.x + 6; x++)
			{
				for (int y = chokeCenter.y - 6; y < chokeCenter.y + 6; y++)
				{
					TilePosition t(x, y);
					if (isWallTight((*typeIterator), t, tight) && testPiece(t))
						placePiece(t);
				}
			}
		}
		
		return true;
	}

	bool Map::identicalPiece(TilePosition parentTile, UnitType parentType, TilePosition currentTile, UnitType currentType)
	{
		// Want to store that it is physically possible to build this piece here (EXCEPT overlapping current wall) - IMPORTANT
		// If BOTH are true, not only is it physically possible, but also is wall tight
		parentSame = false; currentSame = false;
		if (parentType != UnitTypes::None)
		{
			for (auto& node : visited)
			{
				// If previous piece is identical and current piece is identical, move on
				if (node.type == parentType && node.tile == parentTile)
					parentSame = true;
				if (node.type == currentType && node.tile == currentTile)
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
		if (currentSame && parentSame) return true;
		if (!(*typeIterator).isValid()
			|| !t.isValid()
			|| overlapsCurrentWall(t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight()) != UnitTypes::None
			|| overlapsAnything(t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight(), true)
			|| tilesWithinArea(area, t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight()) == 0
			|| !isPlaceable(*typeIterator, t)) return false;
		return true;
	}

	bool Map::placePiece(TilePosition t)
	{
		if (!currentSame)
		{
			PieceNode newNode;
			newNode.tile = t;
			newNode.type = *typeIterator;
			visited.push_back(newNode);
		}

		currentWall[t] = *typeIterator, typeIterator++;

		// If we haven't placed all the pieces yet, try placing another
		if (typeIterator != buildings.end())
		{			
			checkPiece(t);
		}
		else
		{
			// Try to find a hole in the wall
			findCurrentHole(startTile, endTile, choke);
			double dist = 0.0;

			// If we're not fully walled, check score
			if (tight == UnitTypes::None)
			{
				for (auto& piece : currentWall)
					dist += piece.first.getDistance((TilePosition)choke->Center());
				if ((double)currentPath.size() / dist > bestWallScore)
				{
					bestWall = currentWall;
					bestWallScore = (double)currentPath.size() / dist;
				}
			}

			// If we are fully walled, check no hole
			else
			{
				if (!currentHole.isValid())
				{
					for (auto& piece : currentWall)
						dist += piece.first.getDistance((TilePosition)choke->Center());
					if (dist < closest)
						bestWall = currentWall;
				}
			}
		}
		// Erase current tile and reduce iterator
		currentWall.erase(t);		
		typeIterator--;
		return true;
	}

	void Map::findCurrentHole(TilePosition startTile, TilePosition endTile, const BWEM::ChokePoint * choke)
	{
		currentHole = TilePositions::None;
		currentPath = BWEB::AStar().findPath(startTile, endTile, true);

		// Quick check to see if the path contains our end point
		if (find(currentPath.begin(), currentPath.end(), endTile) == currentPath.end())
		{
			currentHole = TilePositions::None;
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

	void Map::addToWall(UnitType building, Wall * wall, UnitType tight)
	{
		// TODO: Remake this
	}

	void Map::addWallDefenses(const vector<UnitType>& types, Wall& wall)
	{
		for (auto& building : types)
		{
			double distance = DBL_MAX;
			TilePosition tileBest = TilePositions::Invalid;
			TilePosition start(wall.getChokePoint()->Center());

			for (int x = start.x - 10; x <= start.x + 10; x++)
			{
				for (int y = start.y - 10; y <= start.y + 10; y++)
				{
					TilePosition t(x, y);

					if (!TilePosition(x, y).isValid()) continue;
					if (overlapsAnything(TilePosition(x, y), 2, 2, true) || overlapsWalls(t) || reservePath[x][y] > 0) continue;
					if (!isPlaceable(building, TilePosition(x, y)) || overlapsCurrentWall(TilePosition(x, y), 2, 2) != UnitTypes::None) continue;


					Position center = Position(TilePosition(x, y)) + Position(32, 32);
					Position hold = Position(start);
					double dist = center.getDistance(hold);

					if (BWEM::Map::Instance().GetArea(TilePosition(center)) != wall.getArea()) continue;
					if ((dist >= 128 || Broodwar->self()->getRace() == Races::Zerg) && dist < distance)
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
		return;
	}

	//void Map::findPath(const BWEM::Area * start, const BWEM::Area * end)
	void Map::findPath()
	{
		// TODO: Add start->end, currently just does main->natural

		const BWEM::Area* thirdArea = (naturalChoke->GetAreas().first != naturalArea) ? naturalChoke->GetAreas().first : naturalChoke->GetAreas().second;
		TilePosition tile(thirdArea->Top());
		eraseBlock(tile);

		auto path = AStar().findPath((TilePosition)mainChoke->Center(), tile, false);
		for (auto& tile : path) {
			reservePath[tile.x][tile.y] = 1;
		}

		// Create a door for the path
		// TODO: Need a way better option than this
		if (naturalChoke)
		{
			double distBest = DBL_MAX;
			TilePosition tileBest(TilePositions::Invalid);
			for (auto &geo : naturalChoke->Geometry())
			{
				TilePosition tile = (TilePosition)geo;
				double dist = tile.getDistance((TilePosition)naturalChoke->Center());
				if (reservePath[tile.x][tile.y] == 1 && dist < distBest)
					tileBest = tile, distBest = dist;
			}
		}
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

	bool Map::isWallTight(UnitType building, TilePosition here, UnitType type)
	{
		bool L, R, T, B;
		L = R = T = B = false;
		int height = building.tileHeight() * 4;
		int width = building.tileWidth() * 4;
		int htSize = building.tileHeight() * 16;
		int wtSize = building.tileWidth() * 16;

		if (type != UnitTypes::None)
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