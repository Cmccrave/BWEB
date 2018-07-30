#include "Wall.h"
#include <tuple>

namespace BWEB
{
	Wall::Wall(const BWEM::Area * a, const BWEM::ChokePoint * c, vector<UnitType> b, vector<UnitType> d)
	{
		area = a;
		choke = c;
		door = TilePositions::Invalid;
		rawBuildings = b;
		rawDefenses = d;
	}

	void Map::createWall(vector<UnitType>& buildings, const BWEM::Area * area, const BWEM::ChokePoint * choke, const UnitType tight, const vector<UnitType>& defenses, const bool reservePath, const bool requireTight)
	{
		if (!area || !choke || buildings.empty())
			return;

		// Create a new wall object
		Wall wall(area, choke, buildings, defenses);

		// I got sick of passing the parameters everywhere, sue me
		this->tight = tight, this->reservePath = reservePath;
		this->requireTight = requireTight;

		double distBest = DBL_MAX;
		for (auto &base : wall.getArea()->Bases()) {
			double dist = base.Center().getDistance((Position)choke->Center());
			if (dist < distBest)
				distBest = dist, wallBase = base.Location();
		}

		chokeWidth = max(6, int(choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2)) / 8));

		// Setup pathing parameters
		resetStartEndTiles(wall);
		setStartTile(wall);
		setEndTile(wall);

		// Iterate pieces, try to find best location
		if (iteratePieces(wall)) {
			for (auto &placement : bestWall) {
				wall.insertSegment(placement.first, placement.second);
				addOverlap(placement.first, placement.second.tileWidth(), placement.second.tileHeight());
			}

			currentWall = bestWall;
			findCurrentHole(wall);

			if (requireTight && currentHole.isValid())
				return;

			addReservePath(wall);
			addCentroid(wall);
			addDefenses(wall);
			addDoor(wall);

			// Push wall into the vector
			walls.push_back(wall);
		}
	}

	bool Map::iteratePieces(Wall& wall)
	{
		TilePosition start = static_cast<TilePosition>(wall.getChokePoint()->Center());

		int i = 0;
		while (!Broodwar->isBuildable(start)) {

			if (i == 10)
				break;

			double distBest = DBL_MAX;
			TilePosition test = start;
			for (int x = test.x - 1; x <= test.x + 1; x++) {
				for (int y = test.y - 1; y <= test.y + 1; y++) {
					TilePosition t(x, y);
					if (!t.isValid()) continue;
					double dist = Position(t).getDistance((Position)wall.getArea()->Top());

					if (dist < distBest) {
						distBest = dist;
						start = t;
					}
				}
			}
			i++;
		}

		// Sort functionality for Pylons by Hannes
		if (find(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), UnitTypes::Protoss_Pylon) != wall.getRawBuildings().end()) {
			sort(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), [](UnitType l, UnitType r) { return (l == UnitTypes::Protoss_Pylon) < (r == UnitTypes::Protoss_Pylon); }); // Moves pylons to end
			sort(wall.getRawBuildings().begin(), find(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), UnitTypes::Protoss_Pylon)); // Sorts everything before pylons
		}
		else
			sort(wall.getRawBuildings().begin(), wall.getRawBuildings().end());

		do {
			currentWall.clear();
			typeIterator = wall.getRawBuildings().begin();
			checkPiece(wall, start);
		} while (next_permutation(wall.getRawBuildings().begin(), find(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), UnitTypes::Protoss_Pylon)));

		return !bestWall.empty();
	}

	bool Map::checkPiece(Wall& wall, const TilePosition start)
	{
		auto parentType = overlapsCurrentWall(start);
		auto currentType = (*typeIterator);
		TilePosition currentSize;
		const auto tightnessFactor = tight == UnitTypes::None ? 32 : min(tight.width(), tight.height());

		if (currentType.isValid())
			currentSize = ((*typeIterator).tileSize());

		// If we have a previous piece, only iterate the pieces around it
		if (parentType != UnitTypes::None && currentType != UnitTypes::Protoss_Pylon) {
			const auto parentSize = (parentType.tileSize());

			const auto parentRight = (parentSize.x * 16) - parentType.dimensionRight() - 1;
			const auto currentLeft = (currentSize.x * 16) - (*typeIterator).dimensionLeft();
			const auto parentLeft = (parentSize.x * 16) - parentType.dimensionLeft();
			const auto currentRight = (currentSize.x * 16) - (*typeIterator).dimensionRight() - 1;

			// Left edge and right edge
			if (parentRight + currentLeft < tightnessFactor || parentLeft + currentRight < tightnessFactor) {
				const auto xLeft = start.x - currentSize.x;
				const auto xRight = start.x + parentSize.x;
				for (auto y = 1 + start.y - currentSize.y; y < start.y + parentSize.y; y++) {
					const TilePosition left(xLeft, y);
					const TilePosition right(xRight, y);

					if (left.isValid() && visited[currentType].location[left.x][left.y] != 2) {
						if (parentLeft + currentRight < tightnessFactor && testPiece(wall, left))
							placePiece(wall, left);
					}
					if (right.isValid() && visited[currentType].location[right.x][right.y] != 2) {
						if (parentRight + currentLeft < tightnessFactor && testPiece(wall, right))
							placePiece(wall, right);
					}
				}
			}

			// Top and bottom edge
			const auto parentTop = (parentSize.y * 16) - parentType.dimensionUp();
			const auto currentBottom = (currentSize.y * 16) - (*typeIterator).dimensionDown() - 1;
			const auto parentBottom = (parentSize.y * 16) - parentType.dimensionDown() - 1;
			const auto currentTop = (currentSize.y * 16) - (*typeIterator).dimensionUp();

			if (parentTop + currentBottom < tightnessFactor || parentBottom + currentTop < tightnessFactor) {
				const auto yTop = start.y - currentSize.y;
				const auto yBottom = start.y + parentSize.y;
				for (auto x = 1 + start.x - currentSize.x; x < start.x + parentSize.x; x++) {
					const TilePosition top(x, yTop);
					const TilePosition bot(x, yBottom);
					if (top.isValid() && visited[currentType].location[top.x][top.y] != 2) {
						if (parentTop + currentBottom < tightnessFactor && testPiece(wall, top))
							placePiece(wall, top);
					}
					if (bot.isValid() && visited[currentType].location[bot.x][bot.y] != 2) {
						if (parentBottom + currentTop < tightnessFactor && testPiece(wall, bot))
							placePiece(wall, bot);
					}
				}
			}
		}

		// Otherwise we need to start the choke center
		else {
			for (auto x = start.x - chokeWidth; x < start.x + chokeWidth; x++) {
				for (auto y = start.y - chokeWidth; y < start.y + chokeWidth; y++) {
					const TilePosition t(x, y);
					parentSame = false, currentSame = false;
					if (t.isValid() && testPiece(wall, t) && (isWallTight((*typeIterator), t) || currentType == UnitTypes::Protoss_Pylon))
						placePiece(wall, t);

				}
			}
		}
		return true;
	}

	bool Map::identicalPiece(const TilePosition parentTile, const UnitType parentType, const TilePosition currentTile, UnitType currentType)
	{
		// Want to store that it is physically possible to build this piece here so we don't waste time checking
		parentSame = false, currentSame = false;
		if (parentType != UnitTypes::None) {
			for (auto &node : visited) {
				auto& v = node.second;
				if (node.first == parentType && v.location[parentTile.x][parentTile.y] == 1)
					parentSame = true;
				if (node.first == currentType && v.location[currentTile.x][currentTile.y] == 1)
					currentSame = true;
				if (parentSame && currentSame)
					return overlapsCurrentWall(currentTile, currentType.tileWidth(), currentType.tileHeight()) == UnitTypes::None;
			}
		}
		return false;
	}

	bool Map::testPiece(Wall& wall, TilePosition t)
	{
		UnitType currentType = *typeIterator;
		Position c = Position(t) + Position(currentType.tileSize());

		// If this is not a valid type, not a valid tile, overlaps the current wall, overlaps anything, isn't within the area passed in, isn't placeable or isn't wall tight
		if (currentType == UnitTypes::Protoss_Pylon && !isPoweringWall(t)) return false;
		if (!currentType.isValid() || !t.isValid() || overlapsCurrentWall(t, currentType.tileWidth(), currentType.tileHeight()) != UnitTypes::None) return false;
		if (currentType == UnitTypes::Terran_Supply_Depot && chokeWidth < 4 && c.getDistance((Position)wall.getChokePoint()->Center()) < 48) return false;

		// If we can't place here, regardless of what's currently placed, set as impossible to place
		if (overlapsAnything(t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight(), true) || !isPlaceable(*typeIterator, t) || tilesWithinArea(wall.getArea(), t, (*typeIterator).tileWidth(), (*typeIterator).tileHeight()) == 0) {
			visited[(*typeIterator)].location[t.x][t.y] = 2;
			return false;
		}
		return true;
	}

	bool Map::placePiece(Wall& wall, const TilePosition t)
	{
		// If we haven't tried to place one here, set visited
		if (!currentSame)
			visited[(*typeIterator)].location[t.x][t.y] = 1;

		if (typeIterator == wall.getRawBuildings().end() - 1 && requireTight && !isWallTight(*typeIterator, t))
			return false;

		currentWall[t] = *typeIterator, ++typeIterator;

		// If we have placed all pieces
		if (typeIterator == wall.getRawBuildings().end()) {
			if (currentWall.size() == wall.getRawBuildings().size()) {

				// Find current hole, not including overlap
				findCurrentHole(wall, true);
				resetStartEndTiles(wall);

				double dist = 1.0;
				for (auto &piece : currentWall) {

					if (piece.second == UnitTypes::Protoss_Pylon) {
						if (wallBase.isValid()) {
							double test = exp(piece.first.getDistance(wallBase)) / (double)mapBWEM.GetTile(t).MinAltitude();
							dist += test;
						}
						else {
							double test = 1.0 / ((double)mapBWEM.GetTile(t).MinAltitude());
							dist += test;
						}
					}
					else if (wallBase.isValid())
						dist += piece.first.getDistance(static_cast<TilePosition>(wall.getChokePoint()->Center())) + piece.first.getDistance(wallBase);
					else
						dist += piece.first.getDistance(static_cast<TilePosition>(wall.getChokePoint()->Center()));
				}

				// If we need a path, find the current hole including overlap
				if (reservePath) {
					findCurrentHole(wall, false);
				}

				const auto score = currentPathSize / dist;
				if (score > bestWallScore && (!reservePath || currentHole != TilePositions::None)) {
					bestWall = currentWall, bestWallScore = score;
				}
			}
		}

		// Else check for another
		else
			checkPiece(wall, t);

		// Erase current tile and reduce iterator
		currentWall.erase(t);
		--typeIterator;
		return true;
	}

	void Map::findCurrentHole(Wall& wall, bool ignoreOverlap)
	{
		if (overlapsCurrentWall(startTile) != UnitTypes::None || !isWalkable(startTile) || !isWalkable(endTile))
			setStartTile(wall);
		if (overlapsCurrentWall(endTile) != UnitTypes::None || !isWalkable(startTile) || !isWalkable(endTile))
			setEndTile(wall);

		// Reset hole and get a new path
		Path newPath;
		newPath.createWallPath(*this, mapBWEM, startTile, endTile);
		currentHole = TilePositions::None;
		currentPath = newPath.getTiles();

		// Quick check to see if the path contains our end point
		if (find(currentPath.begin(), currentPath.end(), endTile) == currentPath.end()) {
			currentHole = TilePositions::None;
			currentPathSize = DBL_MAX;
			resetStartEndTiles(wall);
			return;
		}

		// Otherwise iterate all tiles and locate the hole
		for (auto &tile : currentPath) {
			double closestGeo = DBL_MAX;
			for (auto &geo : wall.getChokePoint()->Geometry()) {
				if (overlapsCurrentWall(tile) == UnitTypes::None && TilePosition(geo) == tile && tile.getDistance(startTile) < closestGeo)
					currentHole = tile, closestGeo = tile.getDistance(startTile);
			}
		}
		currentPathSize = currentHole.getDistance(startTile) * static_cast<double>(currentPath.size());
		resetStartEndTiles(wall);
	}

	void Map::addDoor(Wall& wall)
	{
		// Sets the walls door if there is a path out
		for (auto &tile : currentPath) {
			if (!wall.getDoor().isValid()) {

				int blocked = 0;
				UnitType touch = UnitTypes::None;
				for (int x = tile.x - 1; x < tile.x + 1; x++) {
					for (int y = tile.y - 1; y < tile.y + 1; y++) {
						TilePosition t(x, y);
						UnitType overlap = overlapsCurrentWall(t);
						if (overlapGrid[x][y] || overlapsCurrentWall(t) != touch || !isWalkable(t))
							blocked++;
						if (overlap != touch)
							touch = overlap;
					}
				}

				if (blocked >= 2) {
					wall.setWallDoor(tile);
				}
			}
		}
	}

	void Map::addReservePath(Wall& wall)
	{
		// Sets the walls reserve path if needed
		if (reservePath) {
			for (auto &tile : currentPath)
				reserveGrid[tile.x][tile.y] = 1;
		}
	}

	void Map::addCentroid(Wall& wall)
	{
		// Set the Walls centroid
		Position centroid;
		int sizeWall = currentWall.size();
		for (auto &piece : currentWall) {
			if (piece.second != UnitTypes::Protoss_Pylon)
				centroid += static_cast<Position>(piece.first) + static_cast<Position>(piece.second.tileSize()) / 2;
			else
				sizeWall--;
		}
		wall.setCentroid(centroid / sizeWall);
	}

	void Map::addDefenses(Wall& wall)
	{
		// Add wall defenses if requested
		for (auto &building : wall.getRawDefenses())
			addToWall(building, wall, tight);
	}

	UnitType Map::overlapsCurrentWall(const TilePosition here, const int width, const int height)
	{
		for (auto x = here.x; x < here.x + width; x++) {
			for (auto y = here.y; y < here.y + height; y++) {
				for (auto &placement : currentWall) {
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
		auto start = TilePosition(wall.getCentroid());
		auto centroidDist = wall.getCentroid().getDistance(Position(endTile));

		// Iterate around wall centroid to find a suitable position
		for (auto x = start.x - 8; x <= start.x + 8; x++) {
			for (auto y = start.y - 8; y <= start.y + 8; y++) {
				const TilePosition t(x, y);
				const auto center = (Position(t) + Position(32, 32));				

				if (!t.isValid()
					|| overlapsAnything(t, building.tileWidth(), building.tileHeight())
					|| !isPlaceable(building, t)
					|| tilesWithinArea(wall.getArea(), t, 2, 2) == 0)
					continue;

				const auto hold = Position(start);
				const auto dist = center.getDistance((Position)endTile);				

				if (dist < distance && dist > 96.0 && dist + 32.0 > centroidDist)
					tileBest = TilePosition(x, y), distance = dist;
			}
		}

		// If tile is valid, add to wall
		if (tileBest.isValid()) {
			currentWall[tileBest] = building;
			wall.insertDefense(tileBest);
			addOverlap(tileBest, 2, 2);
		}
	}

	bool Map::isPoweringWall(const TilePosition here)
	{
		for (auto &piece : currentWall) {
			const auto tile(piece.first);
			auto type(piece.second);
			if (type.tileWidth() == 4) {
				auto powersThis = false;
				if (tile.y - here.y == -5 || tile.y - here.y == 4) {
					if (tile.x - here.x >= -4 && tile.x - here.x <= 1) powersThis = true;
				}
				if (tile.y - here.y == -4 || tile.y - here.y == 3) {
					if (tile.x - here.x >= -7 && tile.x - here.x <= 4) powersThis = true;
				}
				if (tile.y - here.y == -3 || tile.y - here.y == 2) {
					if (tile.x - here.x >= -8 && tile.x - here.x <= 5) powersThis = true;
				}
				if (tile.y - here.y >= -2 && tile.y - here.y <= 1) {
					if (tile.x - here.x >= -8 && tile.x - here.x <= 6) powersThis = true;
				}
				if (!powersThis) return false;
			}
			else {
				auto powersThis = false;
				if (tile.y - here.y == 4) {
					if (tile.x - here.x >= -3 && tile.x - here.x <= 2) powersThis = true;
				}
				if (tile.y - here.y == -4 || tile.y - here.y == 3) {
					if (tile.x - here.x >= -6 && tile.x - here.x <= 5) powersThis = true;
				}
				if (tile.y - here.y >= -3 && tile.y - here.y <= 2) {
					if (tile.x - here.x >= -7 && tile.x - here.x <= 6) powersThis = true;
				}
				if (!powersThis) return false;
			}
		}
		return true;
	}

	void Wall::insertSegment(const TilePosition here, UnitType building)
	{
		if (building.tileWidth() >= 4)
			large.insert(here);
		else if (building.tileWidth() >= 3)
			medium.insert(here);
		else
			small.insert(here);
	}

	const Wall * Map::getClosestWall(TilePosition here) const
	{
		double distBest = DBL_MAX;
		const Wall * bestWall = nullptr;
		for (auto &wall : walls) {
			const auto dist = here.getDistance(static_cast<TilePosition>(wall.getChokePoint()->Center()));

			if (dist < distBest)
				distBest = dist, bestWall = &wall;

		}
		return bestWall;
	}

	Wall* Map::getWall(const BWEM::Area * area, const BWEM::ChokePoint * choke)
	{
		if (!area && !choke)
			return nullptr;

		for (auto &wall : walls) {
			if ((!area || wall.getArea() == area) && (!choke || wall.getChokePoint() == choke))
				return &wall;
		}
		return nullptr;
	}

	bool Map::isWallTight(UnitType building, const TilePosition here)
	{
		const auto height = building.tileHeight() * 4;
		const auto width = building.tileWidth() * 4;
		const auto htSize = building.tileHeight() * 16;
		const auto wtSize = building.tileWidth() * 16;
		const auto tightnessFactor = tight == UnitTypes::None ? 32 : min(tight.width(), tight.height());
		bool B, T, L, R;
		B = T = L = R = false;

		if (tight != UnitTypes::None) {
			if (htSize - building.dimensionDown() - 1 < tightnessFactor)
				B = true;
			if (htSize - building.dimensionUp() < tightnessFactor)
				T = true;
			if (wtSize - building.dimensionLeft() < tightnessFactor)
				L = true;
			if (wtSize - building.dimensionRight() - 1 < tightnessFactor)
				R = true;
		}
		else
			L = R = T = B = true;

		const auto right = WalkPosition(here) + WalkPosition(width, 0);
		const auto left = WalkPosition(here) - WalkPosition(1, 0);
		const auto top = WalkPosition(here) - WalkPosition(0, 1);
		const auto bottom = WalkPosition(here) + WalkPosition(0, height);

		const auto tight = [&](WalkPosition w, bool check) {
			TilePosition t(w);
			if (check && (!w.isValid() || !Broodwar->isWalkable(w)))
				return true;
			if (!requireTight && !isWalkable(t))
				return true;
			return false;
		};

		for (auto y = right.y; y < right.y + height; y++) {
			if (tight(WalkPosition(right.x, y), R))
				return true;
		}

		for (auto y = left.y; y < left.y + height; y++) {
			if (tight(WalkPosition(left.x, y), L))
				return true;
		}

		for (auto x = top.x; x < top.x + width; x++) {
			if (tight(WalkPosition(x, top.y), T))
				return true;
		}

		for (auto x = bottom.x; x < bottom.x + width; x++) {
			if (tight(WalkPosition(x, bottom.y), B))
				return true;
		}
		return false;
	}

	void Map::setStartTile(Wall& newWall)
	{
		auto distBest = DBL_MAX;
		if (!mapBWEM.GetArea(startTile) || !isWalkable(startTile)) {
			for (auto x = startTile.x - 2; x < startTile.x + 2; x++) {
				for (auto y = startTile.y - 2; y < startTile.y + 2; y++) {
					TilePosition t(x, y);
					const auto dist = t.getDistance(endTile);
					if (overlapsCurrentWall(t) != UnitTypes::None)
						continue;

					if (mapBWEM.GetArea(t) == newWall.getArea() && dist < distBest)
						startTile = TilePosition(x, y), distBest = dist;
				}
			}
		}
	}

	void Map::setEndTile(Wall& newWall)
	{
		auto distBest = 0.0;
		if (!mapBWEM.GetArea(endTile) || !isWalkable(endTile)) {
			for (auto x = endTile.x - 4; x < endTile.x + 4; x++) {
				for (auto y = endTile.y - 4; y < endTile.y + 4; y++) {
					TilePosition t(x, y);
					const auto dist = t.getDistance(startTile);
					if (overlapsCurrentWall(t) != UnitTypes::None || !isWalkable(t))
						continue;

					if (mapBWEM.GetArea(t) && dist > distBest)
						endTile = TilePosition(x, y), distBest = dist;
				}
			}
		}
	}

	void Map::resetStartEndTiles(Wall& newWall)
	{
		const auto thirdArea = (newWall.getChokePoint()->GetAreas().first != newWall.getArea()) ? newWall.getChokePoint()->GetAreas().first : newWall.getChokePoint()->GetAreas().second;

		// Finding start and end points of our pathing
		const auto mc = static_cast<Position>(mainChoke->Center());
		const auto nc = static_cast<Position>(naturalChoke->Center());

		if (mainChoke == naturalChoke) {
			startTile = TilePosition((naturalPosition + (Position)naturalChoke->Center()) / 2);
			endTile = TilePosition((mainPosition + (Position)mainChoke->Center()) / 2);
		}
		else if (newWall.getArea() == naturalArea && newWall.getChokePoint() == naturalChoke) {
			double distance = nc.getDistance(naturalPosition) / 32.0;
			Position length = (nc - naturalPosition) / distance;

			startTile = TilePosition(mc);
			endTile = TilePosition(nc) + TilePosition(length * 4);
		}
		else if (newWall.getArea() == mainArea && newWall.getChokePoint() == mainChoke) {
			startTile = (TilePosition(mc) + mainTile) / 2;
			endTile = (TilePosition(mc) + naturalTile) / 2;
		}
		else if (newWall.getArea() == naturalArea && newWall.getChokePoint() == mainChoke) {
			startTile = static_cast<TilePosition>(newWall.getArea()->Top());
			endTile = TilePosition(nc);
		}
		else {
			startTile = static_cast<TilePosition>(newWall.getArea()->Top());
			endTile = static_cast<TilePosition>(thirdArea->Top());
		}
	}
}
