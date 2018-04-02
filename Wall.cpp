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

	void Map::createWall(vector<UnitType>& buildings, const BWEM::Area * area, const BWEM::ChokePoint * choke, const UnitType tight, const vector<UnitType>& defenses, const bool reservePath)
	{
		if (!area || !choke || buildings.empty())
			return;

		// Start a clock to time walls
		const auto start{ chrono::high_resolution_clock::now() };

		// I got sick of passing the parameters everywhere, sue me
		this->buildings = buildings, this->area = area, this->choke = choke, this->tight = tight, this->reservePath = reservePath;

		// Create a new wall object
		Wall newWall(area, choke);

		// Setup pathing parameters
		resetStartEndTiles();
		setStartTile();
		setEndTile();

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
		if (reservePath) {
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
			centroid += static_cast<Position>(piece.first) + static_cast<Position>(piece.second.tileSize()) / 2;
		newWall.setCentroid(centroid / currentWall.size());

		// Add wall defenses if requested
		if (!defenses.empty())
			addWallDefenses(defenses, newWall);

		// Push wall into the vector
		walls.push_back(newWall);

		// Print time
		const auto dur = chrono::duration <double, milli>(chrono::high_resolution_clock::now() - start).count();
		Broodwar << "Wall time: " << dur << endl;
	}

	bool Map::iteratePieces()
	{
		// Sort functionality for Pylons by Hannes
		if (find(buildings.begin(), buildings.end(), UnitTypes::Protoss_Pylon) != buildings.end())
		{
			sort(buildings.begin(), buildings.end(), [](UnitType l, UnitType r) { return (l == UnitTypes::Protoss_Pylon) < (r == UnitTypes::Protoss_Pylon); }); // Moves pylons to end
			sort(buildings.begin(), find(buildings.begin(), buildings.end(), UnitTypes::Protoss_Pylon)); // Sorts everything before pylons
		}
		else
			sort(buildings.begin(), buildings.end());

		do {
			currentWall.clear();
			typeIterator = buildings.begin();
			checkPiece(static_cast<TilePosition>(choke->Center()));
		} while (next_permutation(buildings.begin(), find(buildings.begin(), buildings.end(), UnitTypes::Protoss_Pylon)));

		return !bestWall.empty();
	}

	bool Map::checkPiece(const TilePosition start)
	{
		auto parentType = overlapsCurrentWall(start);
		auto currentType = (*typeIterator);
		TilePosition currentSize;
		const auto tightnessFactor = tight == UnitTypes::None ? 32 : min(tight.width(), tight.height());

		if (currentType.isValid())
			currentSize = ((*typeIterator).tileSize());

		// If we have a previous piece, only iterate the pieces around it
		if (parentType != UnitTypes::None && currentType != UnitTypes::Protoss_Pylon)
		{
			const auto parentSize = (parentType.tileSize());

			const auto parentRight = (parentSize.x * 16) - parentType.dimensionRight() - 1;
			const auto currentLeft = (currentSize.x * 16) - (*typeIterator).dimensionLeft();
			const auto parentLeft = (parentSize.x * 16) - parentType.dimensionLeft();
			const auto currentRight = (currentSize.x * 16) - (*typeIterator).dimensionRight() - 1;

			// Left edge and right edge
			if (parentRight + currentLeft < tightnessFactor || parentLeft + currentRight < tightnessFactor)
			{
				const auto xLeft = start.x - currentSize.x;
				const auto xRight = start.x + parentSize.x;
				for (auto y = 1 + start.y - currentSize.y; y < start.y + parentSize.y; y++)
				{
					const TilePosition left(xLeft, y);
					const TilePosition right(xRight, y);

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
			const auto parentTop = (parentSize.y * 16) - parentType.dimensionUp();
			const auto currentBottom = (currentSize.y * 16) - (*typeIterator).dimensionDown() - 1;
			const auto parentBottom = (parentSize.y * 16) - parentType.dimensionDown() - 1;
			const auto currentTop = (currentSize.y * 16) - (*typeIterator).dimensionUp();

			if (parentTop + currentBottom < tightnessFactor || parentBottom + currentTop < tightnessFactor)
			{
				const auto yTop = start.y - currentSize.y;
				const auto yBottom = start.y + parentSize.y;
				for (auto x = 1 + start.x - currentSize.x; x < start.x + parentSize.x; x++)
				{
					const TilePosition top(x, yTop);
					const TilePosition bot(x, yBottom);
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
			const auto width = int(choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2)) / 4);
			for (auto x = start.x - width; x < start.x + width; x++)
			{
				for (auto y = start.y - width; y < start.y + width; y++)
				{
					const TilePosition t(x, y);
					parentSame = false, currentSame = false;
					if ((isWallTight((*typeIterator), t) || currentType == UnitTypes::Protoss_Pylon) && testPiece(t))
						placePiece(t);
				}
			}
		}
		return true;
	}

	bool Map::identicalPiece(const TilePosition parentTile, const UnitType parentType, const TilePosition currentTile, UnitType currentType)
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
				{
					return overlapsCurrentWall(currentTile, currentType.tileWidth(), currentType.tileHeight()) == UnitTypes::None;
				}
			}
		}
		return false;
	}

	bool Map::testPiece(TilePosition t)
	{
		// If this is not a valid type, not a valid tile, overlaps the current wall, overlaps anything, isn't within the area passed in, isn't placeable or isn't wall tight
		if ((*typeIterator) == UnitTypes::Protoss_Pylon && !isPoweringWall(t)) return false;
		if (!(*typeIterator).isValid() || !t.isValid() || overlapsCurrentWall(t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight()) != UnitTypes::None) return false;

		// If we can't place here, regardless of what's currently placed, set as impossible to place
		if (overlapsAnything(t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight(), true) || !isPlaceable(*typeIterator, t) || tilesWithinArea(area, t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight()) == 0)
		{
			visited[(*typeIterator)].location[t.x][t.y] = 2;
			return false;
		}
		return true;
	}

	bool Map::placePiece(const TilePosition t)
	{
		// If we haven't tried to place one here, set visited
		if (!currentSame)
			visited[(*typeIterator)].location[t.x][t.y] = 1;

		currentWall[t] = *typeIterator, ++typeIterator;

		// If we have placed all pieces
		if (typeIterator == buildings.end())
		{
			if (currentWall.size() == buildings.size())
			{
				findCurrentHole();
				double dist = 1.0;
				for (auto& piece : currentWall)
				{
					if (piece.second == UnitTypes::Protoss_Pylon || piece.second.getRace() == Races::Zerg)
						dist += piece.first.getDistance(naturalTile);
					else
						dist += piece.first.getDistance(static_cast<TilePosition>(choke->Center()));
				}

				const auto score = currentPathSize / dist;
				if (score > bestWallScore && (!reservePath || currentHole != TilePositions::None))
				{
					bestWall = currentWall;
					bestWallScore = score;
				}
			}
		}

		// Else check for another
		else
			checkPiece(t);

		// Erase current tile and reduce iterator
		currentWall.erase(t);
		--typeIterator;
		return true;
	}

	void Map::findCurrentHole()
	{
		if (overlapsCurrentWall(startTile) != UnitTypes::None)
			setStartTile();
		if (overlapsCurrentWall(endTile) != UnitTypes::None)
			setEndTile();

		// Reset hole and get a new path
		currentHole = TilePositions::None;
		currentPath = AStar().findPath(startTile, endTile, true);
		currentPathSize = static_cast<double>(currentPath.size());

		// Quick check to see if the path contains our end point
		if (find(currentPath.begin(), currentPath.end(), endTile) == currentPath.end())
		{
			currentHole = TilePositions::None;
			currentPathSize = DBL_MAX;
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
		resetStartEndTiles();
	}

	UnitType Map::overlapsCurrentWall(const TilePosition here, const int width, const int height)
	{
		for (auto x = here.x; x < here.x + width; x++)
		{
			for (auto y = here.y; y < here.y + height; y++)
			{
				for (auto& placement : currentWall)
				{
					const auto tile = placement.first;
					if (x >= tile.x && x < tile.x + placement.second.tileWidth() && y >= tile.y && y < tile.y + placement.second.tileHeight())
						return placement.second;
				}
			}
		}
		return UnitTypes::None;
	}

	void Map::addToWall(UnitType building, Wall& wall, UnitType tight)
	{
		auto distance = DBL_MAX;
		auto tileBest = TilePositions::Invalid;
		TilePosition start(wall.getCentroid());

		if (wall.getDoor().isValid())
			start = TilePosition(wall.getDoor());

		for (auto x = start.x - 6; x <= start.x + 6; x++)
		{
			for (auto y = start.y - 6; y <= start.y + 6; y++)
			{
				const TilePosition t(x, y);
				Position p(t);

				if (!TilePosition(x, y).isValid()) continue;
				if (overlapsAnything(TilePosition(x, y), building.tileWidth(), building.tileHeight(), true) || overlapsWalls(t)) continue;
				if (!isPlaceable(building, TilePosition(x, y)) || overlapsCurrentWall(TilePosition(x, y), building.tileWidth(), building.tileHeight()) != UnitTypes::None) continue;

				auto center = Position(t) + Position(32, 32);
				const auto hold = Position(start);

				const auto dist = center.getDistance(hold);

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
			addOverlap(tileBest, building.tileWidth(), building.tileHeight());
		}
	}

	void Map::addWallDefenses(const vector<UnitType>& types, Wall& wall)
	{
		for (auto& building : types)
			addToWall(building, wall, tight);
	}

	bool Map::isPoweringWall(const TilePosition here)
	{
		for (auto& piece : currentWall)
		{
			const auto tile(piece.first);
			auto type(piece.second);
			if (type.tileWidth() == 4)
			{
				auto powersThis = false;
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
				auto powersThis = false;
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

	void Wall::insertSegment(const TilePosition here, UnitType building)
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
			const auto dist = here.getDistance(static_cast<TilePosition>(wall.getChokePoint()->Center()));

			if (dist < distBest)
			{
				distBest = dist;
				bestWall = &wall;
			}
		}
		return bestWall;
	}

	Wall* Map::getWall(const BWEM::Area * area, const BWEM::ChokePoint * choke)
	{
		if (!area && !choke) return nullptr;

		for (auto& wall : walls)
		{
			if ((!area || wall.getArea() == area) && (!choke || wall.getChokePoint() == choke))
				return &wall;
		}
		return nullptr;
	}

	bool Map::isWallTight(UnitType building, const TilePosition here)
	{
		bool R, T, B;
		auto L = (R = T = B = false);
		const auto height = building.tileHeight() * 4;
		const auto width = building.tileWidth() * 4;
		const auto htSize = building.tileHeight() * 16;
		const auto wtSize = building.tileWidth() * 16;

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

		const auto right = WalkPosition(here) + WalkPosition(width, 0);
		const auto left = WalkPosition(here) - WalkPosition(1, 0);
		const auto top = WalkPosition(here) - WalkPosition(0, 1);
		const auto bottom = WalkPosition(here) + WalkPosition(0, height);

		for (auto y = right.y - 1; y < right.y + height + 1; y++)
		{
			const auto x = right.x;
			WalkPosition w(x, y);
			TilePosition t(w);
			if (R && (!w.isValid() || !Broodwar->isWalkable(w) || overlapGrid[x][y] > 0)) return true;
		}

		for (auto y = left.y - 1; y < left.y + height + 1; y++)
		{
			const auto x = left.x;
			WalkPosition w(x, y);
			TilePosition t(w);
			if (L && (!w.isValid() || !Broodwar->isWalkable(w) || overlapGrid[x][y] > 0)) return true;
		}

		for (auto x = top.x - 1; x < top.x + width + 1; x++)
		{
			const auto y = top.y;
			WalkPosition w(x, y);
			TilePosition t(w);
			if (T && (!w.isValid() || !Broodwar->isWalkable(w) || overlapGrid[x][y] > 0)) return true;
		}

		for (auto x = bottom.x - 1; x < bottom.x + width + 1; x++)
		{
			const auto y = bottom.y;
			WalkPosition w(x, y);
			TilePosition t(w);
			if (B && (!w.isValid() || !Broodwar->isWalkable(w) || overlapGrid[x][y] > 0)) return true;
		}
		return false;
	}

	void Map::setStartTile()
	{
		auto distBest = DBL_MAX;
		if (!BWEM::Map::Instance().GetArea(startTile) || !isWalkable(startTile))
		{
			for (auto x = startTile.x - 2; x < startTile.x + 2; x++)
			{
				for (auto y = startTile.y - 2; y < startTile.y + 2; y++)
				{
					TilePosition t(x, y);
					const auto dist = t.getDistance(endTile);
					if (overlapsCurrentWall(t) != UnitTypes::None) continue;
					if (BWEM::Map::Instance().GetArea(t) == area && dist < distBest)
						startTile = TilePosition(x, y), distBest = dist;
				}
			}
		}
	}

	void Map::setEndTile()
	{
		auto distBest = 0.0;
		if (!BWEM::Map::Instance().GetArea(endTile) || !isWalkable(endTile))
		{
			for (auto x = endTile.x - 2; x < endTile.x + 2; x++)
			{
				for (auto y = endTile.y - 2; y < endTile.y + 2; y++)
				{
					TilePosition t(x, y);
					const auto dist = t.getDistance(startTile);
					if (overlapsCurrentWall(t) != UnitTypes::None) continue;
					if (BWEM::Map::Instance().GetArea(t) && dist > distBest)
						endTile = TilePosition(x, y), distBest = dist;
				}
			}
		}
	}

	void Map::resetStartEndTiles()
	{
		const auto thirdArea = (choke->GetAreas().first != area) ? choke->GetAreas().first : choke->GetAreas().second;

		// Finding start and end points of our pathing
		auto mc = static_cast<Position>(mainChoke->Center());
		const auto nc = static_cast<Position>(naturalChoke->Center());
		if (area == naturalArea)
		{
			const auto slope1 = double(nc.x - naturalPosition.x) / 32.0;
			const auto slope2 = double(nc.y - naturalPosition.y) / 32.0;

			startTile = (TilePosition(mainChoke->Center()) + TilePosition(mainChoke->Center()) + TilePosition(mainChoke->Center()) + naturalTile) / 4;
			endTile = TilePosition(nc + Position(int(32.0 * slope1), int(32.0 * slope2)));
		}
		else if (area == mainArea)
		{
			startTile = (TilePosition(mainChoke->Center()) + mainTile) / 2;
			endTile = (TilePosition(mainChoke->Center()) + naturalTile) / 2;
		}
		else
		{
			startTile = static_cast<TilePosition>(area->Top());
			endTile = static_cast<TilePosition>(thirdArea->Top());
		}
	}
}