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

	void Wall::insertSegment(const TilePosition here, UnitType building)
	{
		if (building.tileWidth() >= 4)
			large.insert(here);
		else if (building.tileWidth() >= 3)
			medium.insert(here);
		else
			small.insert(here);
	}



	void Map::createWall(vector<UnitType>& buildings, const BWEM::Area * area, const BWEM::ChokePoint * choke, const UnitType tight, const vector<UnitType>& defenses, const bool reservePath, const bool requireTight)
	{
		if (!area) {
			Broodwar << "BWEB: Can't initialize a wall without a valid BWEM::Area" << endl;
			return;
		}

		if (!choke) {
			Broodwar << "BWEB: Can't initialize a wall without a valid BWEM::Chokepoint" << endl;
			return;
		}

		if (buildings.empty()) {
			Broodwar << "BWEB: Can't initialize a wall with an empty vector of UnitTypes." << endl;
			return;
		}

		// Create a new wall object
		Wall wall(area, choke, buildings, defenses);

		// I got sick of passing the parameters everywhere, sue me
		this->tight = tight;
		this->reservePath = reservePath;
		this->requireTight = requireTight;

		// Create a line of tiles that make up the geometry of the choke		
		for (auto &walk : wall.getChokePoint()->Geometry()) {
			TilePosition t(walk);
			if (Broodwar->isBuildable(t))
				chokeTiles.push_back(t);
		}

		const auto addDoor = [&] {
			// Check which tile is closest to each part on the path, set as door
			double distBest = DBL_MAX;
			for (auto chokeTile : chokeTiles) {
				for (auto pathTile : currentPath) {
					double dist = chokeTile.getDistance(pathTile);
					if (dist < distBest) {
						distBest = dist;
						wall.setWallDoor(chokeTile);
					}
				}
			}

			// If the line of tiles we made is empty, assign closest path tile to closest to wall centroid
			if (chokeTiles.empty()) {
				for (auto pathTile : currentPath) {
					Position p(pathTile);
					double dist = wall.getCentroid().getDistance(p);
					if (dist < distBest) {
						distBest = dist;
						wall.setWallDoor(pathTile);
					}
				}
			}
		};

		// Set the walls reserve path if needed
		const auto addReservePath = [&] {
			if (reservePath) {
				for (auto &tile : currentPath)
					reserveGrid[tile.x][tile.y] = 1;
			}
		};

		// Set the walls centroid
		const auto addCentroid = [&] {
			Position centroid;
			int sizeWall = currentWall.size();
			for (auto &piece : currentWall) {
				if (piece.second != UnitTypes::Protoss_Pylon)
					centroid += static_cast<Position>(piece.first) + static_cast<Position>(piece.second.tileSize()) / 2;
				else
					sizeWall--;
			}
			wall.setCentroid(centroid / sizeWall);
		};

		// Add wall defenses if requested
		const auto addDefenses = [&] {
			for (auto &building : wall.getRawDefenses())
				addToWall(building, wall, tight);
		};

		// HACK: reset these values so I can make multiple walls easily
		bestWallScore = 0.0;
		currentHole = TilePositions::None;
		startTile = TilePositions::None;
		endTile = TilePositions::None;
		currentPath.clear();
		bestWall.clear();
		currentWall.clear();

		double distBest = DBL_MAX;
		for (auto &base : wall.getArea()->Bases()) {
			double dist = base.Center().getDistance((Position)choke->Center());
			if (dist < distBest)
				distBest = dist, wallBase = Position(base.Location()) + Position(64, 48);
		}

		chokeWidth = int(choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2)) / 4);
		chokeWidth = max(6, chokeWidth);

		// Setup pathing parameters
		initializePathPoints(wall);

		// Iterate pieces, try to find best location
		if (iteratePieces(wall)) {
			for (auto &placement : bestWall) {
				wall.insertSegment(placement.first, placement.second);
				addOverlap(placement.first, placement.second.tileWidth(), placement.second.tileHeight());
			}

			currentWall = bestWall;
			findCurrentHole(wall, false);

			if (requireTight && currentHole.isValid())
				Broodwar << "BWEB: Unable to find a tight wall at the given location. Placed best one possible." << endl;

			addReservePath();
			addCentroid();
			addDoor();
			addDefenses();

			// Push wall into the vector
			walls.push_back(wall);
		}
	}

	bool Map::iteratePieces(Wall& wall)
	{
		TilePosition start(wall.getChokePoint()->Center());
		bool movedStart = false;

		const auto closestChokeTile = [&](Position here) {
			double best = DBL_MAX;
			Position posBest;
			for (auto &tile : chokeTiles) {
				Position p = Position(tile) + Position(16, 16);
				if (p.getDistance(here) < best) {
					posBest = p;
					best = p.getDistance(here);
				}
			}
			return posBest;
		};

		const auto checkStart = [&] {
			while (!Broodwar->isBuildable(start)) {
				double distBest = DBL_MAX;
				TilePosition test = start;
				for (int x = test.x - 1; x <= test.x + 1; x++) {
					for (int y = test.y - 1; y <= test.y + 1; y++) {
						TilePosition t(x, y);
						if (!t.isValid())
							continue;

						double dist = Position(t).getDistance((Position)wall.getArea()->Top());

						if (dist < distBest) {
							distBest = dist;
							start = t;
							movedStart = true;
						}
					}
				}
			}
		};

		const auto sortPieces = [&] {

			// Sort functionality for Pylons by Hannes
			if (find(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), UnitTypes::Protoss_Pylon) != wall.getRawBuildings().end()) {
				sort(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), [](UnitType l, UnitType r) { return (l == UnitTypes::Protoss_Pylon) < (r == UnitTypes::Protoss_Pylon); }); // Moves pylons to end
				sort(wall.getRawBuildings().begin(), find(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), UnitTypes::Protoss_Pylon)); // Sorts everything before pylons
			}
			else
				sort(wall.getRawBuildings().begin(), wall.getRawBuildings().end());
		};

		const auto scoreWall = [&] {

			// Find current hole, not including overlap
			findCurrentHole(wall, !reservePath);
			Position chokeCenter(wall.getChokePoint()->Center());
			double dist = 1.0;
			
			// For walls that require a reserved path, we must have a hole
			if (reservePath && currentHole == TilePositions::None)
				return;

			for (auto &piece : currentWall) {
				auto tile = piece.first;
				auto type = piece.second;
				auto center = Position(tile) + Position(type.tileWidth() * 16.0, type.tileHeight() * 16.0);
				auto chokeDist = Position(closestChokeTile(center)).getDistance(center);

				if (type == UnitTypes::Protoss_Pylon && wallBase.isValid())
					dist += (center.getDistance(wallBase)) / chokeDist;
				else
					dist += chokeDist;
			}

			// Score wall based on path sizes and distances
			const auto score = currentHole.getDistance(startTile) * currentPathSize / (dist);

			if (score > bestWallScore) {
				bestWall = currentWall;
				bestWallScore = score;
			}
		};

		const auto testPiece = [&](Wall& wall, TilePosition t) {
			UnitType currentType = *typeIterator;
			Position c = Position(t) + Position(currentType.tileSize());			

			if ((currentType == UnitTypes::Protoss_Pylon && !isPoweringWall(wall, t))
				|| overlapsCurrentWall(t, currentType.tileWidth(), currentType.tileHeight()) != UnitTypes::None
				|| overlapsAnything(t, currentType.tileWidth(), currentType.tileHeight(), true)
				|| !isPlaceable(currentType, t)
				|| tilesWithinArea(wall.getArea(), t, currentType.tileWidth(), currentType.tileHeight()) <= 2)
				return false;

			// If choke is small, make sure we don't place too close to it - TEMP removed
			//if (chokeWidth < 4 && c.getDistance((Position)wall.getChokePoint()->Center()) < 48)
				//return false;
			return true;
		};

		// Choke angle
		auto p1 = wall.getChokePoint()->Pos(wall.getChokePoint()->end1);
		auto p2 = wall.getChokePoint()->Pos(wall.getChokePoint()->end2);
		double dy = abs(double(p1.y - p2.y));
		double dx = abs(double(p1.x - p2.x));
		auto angle1 = dx > 0.0 ? atan(dy / dx) * 180.0 / 3.14 : 90.0;

		function<void(TilePosition)> recursiveCheck;
		recursiveCheck = [&](TilePosition start) -> void {
			UnitType type = *typeIterator;
			for (auto x = start.x - chokeWidth; x < start.x + chokeWidth; x++) {
				for (auto y = start.y - chokeWidth; y < start.y + chokeWidth; y++) {
					const TilePosition t(x, y);
					auto center = Position(t) + Position(type.tileWidth() * 16.0, type.tileHeight() * 16.0);

					if (!t.isValid())
						continue;

					// We want to ensure the buildings are being placed at the correct angle compared to the chokepoint, within some tolerance
					double angle2 = 0.0;
					if (currentWall.size() == 1 && type.getRace() == Races::Protoss && !movedStart) {
						for (auto piece : currentWall) {							
							auto tileB = piece.first;
							auto typeB = piece.second;
							auto centerB = Position(tileB) + Position(typeB.tileWidth() * 16.0, typeB.tileHeight() * 16.0);
							double dy = abs(double(centerB.y - center.y));
							double dx = abs(double(centerB.x - center.x));
														
							angle2 = dx > 0.0 ? atan(dy / dx) * 180.0 / 3.14 : 90.0;
						}
						
						if (abs(abs(angle1) - abs(angle2)) > 15.0)
							continue;
					}					

					// If the piece is fine to place
					if (testPiece(wall, t) && (isWallTight(wall, type, t) || type == UnitTypes::Protoss_Pylon)) {

						// 1) Store the current type
						// 2) Increase the iterator
						// 3) If at the end, score the wall, else, go another layer deeper
						// 4) Erase this current placement and repeat

						currentWall[t] = type;
						typeIterator++;

						if (typeIterator == wall.getRawBuildings().end())
							scoreWall();
						else
							recursiveCheck(start);

						if (typeIterator != wall.getRawBuildings().begin())
							typeIterator--;

						currentWall.erase(t);
					}
				}
			}
		};

		// Sort all the pieces and iterate over them to find the best wall
		sortPieces();
		checkStart();
		testTile = start;
		do {
			currentWall.clear();
			typeIterator = wall.getRawBuildings().begin();
			recursiveCheck(start);
		} while (next_permutation(wall.getRawBuildings().begin(), find(wall.getRawBuildings().begin(), wall.getRawBuildings().end(), UnitTypes::Protoss_Pylon)));
		return !bestWall.empty();
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

	bool Map::isPoweringWall(Wall& wall, const TilePosition here)
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

	bool Map::isWallTight(Wall& wall, UnitType building, const TilePosition here)
	{
		const auto height = building.tileHeight() * 4;
		const auto width = building.tileWidth() * 4;
		const auto tightnessFactor = (tight == UnitTypes::None) ? 32 : min(tight.width(), tight.height());

		const auto checkLeft = (building.tileWidth() * 16) - building.dimensionLeft() < tightnessFactor;
		const auto checkRight = (building.tileWidth() * 16) - building.dimensionRight() - 1 < tightnessFactor;
		const auto checkUp =  (building.tileHeight() * 16) - building.dimensionUp() < tightnessFactor;
		const auto checkDown =  (building.tileHeight() * 16) - building.dimensionDown() - 1 < tightnessFactor;

		// HACK: I don't have a great method for buildings that can check multiple tiles for Terrain tight, hardcoded a few as shown below
		const auto right = WalkPosition(here) + WalkPosition(width, 0) + WalkPosition(building == UnitTypes::Terran_Barracks, 0);
		const auto left = WalkPosition(here) - WalkPosition(1, 0);
		const auto up = WalkPosition(here) - WalkPosition(0, 1);
		const auto down = WalkPosition(here) + WalkPosition(0, height) + WalkPosition(0, building == UnitTypes::Protoss_Gateway || building == UnitTypes::Terran_Supply_Depot);

		auto count = 0;

		const auto terrainTight = [&](WalkPosition w, bool check) {
			TilePosition t(w);

			// If the walkposition is invalid or unwalkable
			if (tight != UnitTypes::None && check && (!w.isValid() || !Broodwar->isWalkable(w)))
				return true;

			// TEST
			if (overlapsAnything(t))
				return true;			

			// If we don't care about walling tight and the tile isn't walkable
			if (!requireTight && !isWalkable(t))
				return true;
			return false;
		};

		for (auto y = right.y; y < right.y + height; y++) {
			WalkPosition w(right.x, y);
			TilePosition t(w);
			auto parent = overlapsCurrentWall(t);

			// Check if it's tight with the terrain
			if (terrainTight(w, checkRight)) {
				count++;
				break;
			}

			// Check if it's tight with a parent
			if (parent != UnitTypes::None) {
				const auto diff = (parent.tileWidth() * 16 - parent.dimensionLeft()) + (building.tileWidth() * 16 - building.dimensionRight() - 1);
				if (diff < tightnessFactor) {
					count++;
					break;
				}
				//else
				//	return false;
			}
		}

		for (auto y = left.y; y < left.y + height; y++) {
			WalkPosition w(left.x, y);
			TilePosition t(w);
			auto parent = overlapsCurrentWall(t);

			// Check if it's tight with the terrain
			if (terrainTight(w, checkLeft)) {
				count++;
				break;
			}

			// Check if it's tight with a parent
			if (parent != UnitTypes::None) {
				const auto diff = (parent.tileWidth() * 16 - parent.dimensionRight() - 1) + (building.tileWidth() * 16 - building.dimensionLeft());
				if (diff < tightnessFactor) {
					count++;
					break;
				}
				//else
				//	return false;
			}
		}

		for (auto x = up.x; x < up.x + width; x++) {
			WalkPosition w(x, up.y);
			TilePosition t(w);
			auto parent = overlapsCurrentWall(t);

			// Check if it's tight with the terrain
			if (terrainTight(w, checkUp)) {
				count++;
				break;
			}

			// Check if it's tight with a parent
			if (parent != UnitTypes::None) {
				const auto diff = (parent.tileHeight() * 16 - parent.dimensionDown() - 1) + (building.tileHeight() * 16 - building.dimensionUp());
				if (diff < tightnessFactor) {
					count++;
					break;
				}
				//else
				//	return false;
			}
		}

		for (auto x = down.x; x < down.x + width; x++) {
			WalkPosition w(x, down.y);
			TilePosition t(w);
			auto parent = overlapsCurrentWall(t);

			// Check if it's tight with the terrain
			if (terrainTight(w, checkDown)) {
				count++;
				break;
			}

			// Check if it's tight with a parent
			if (parent != UnitTypes::None) {
				const auto diff = (parent.tileHeight() * 16 - parent.dimensionUp()) + (building.tileHeight() * 16 - building.dimensionDown() - 1);
				if (diff < tightnessFactor) {
					count++;
					break;
				}
				//else
				//	return false;
			}
		}
		return count >= 1;
	}

	void Map::checkPathPoints(Wall& wall)
	{
		auto distBest = DBL_MAX;
		startTile = initialStart;
		endTile = initialEnd;

		if (!isWalkable(initialStart) || overlapsCurrentWall(initialStart) != UnitTypes::None || overlapGrid[startTile.x][startTile.y] != 0) {
			for (auto x = initialStart.x - 2; x < initialStart.x + 2; x++) {
				for (auto y = initialStart.y - 2; y < initialStart.y + 2; y++) {
					TilePosition t(x, y);
					const auto dist = t.getDistance(endTile);
					if (overlapsCurrentWall(t) != UnitTypes::None || !isWalkable(t))
						continue;

					if (mapBWEM.GetArea(t) == wall.getArea() && dist < distBest) {
						startTile = t;
						distBest = dist;
					}
				}
			}
		}

		distBest = 0.0;
		if (!isWalkable(initialEnd) || overlapsCurrentWall(initialEnd) != UnitTypes::None || overlapGrid[endTile.x][endTile.y] != 0) {
			for (auto x = initialEnd.x - 4; x < initialEnd.x + 4; x++) {
				for (auto y = initialEnd.y - 4; y < initialEnd.y + 4; y++) {
					TilePosition t(x, y);
					const auto dist = t.getDistance(startTile);
					if (overlapsCurrentWall(t) != UnitTypes::None || !isWalkable(t))
						continue;

					if (mapBWEM.GetArea(t) && dist > distBest) {
						endTile = t;
						distBest = dist;
					}
				}
			}
		}
	}

	void Map::initializePathPoints(Wall& wall)
	{
		auto choke = wall.getChokePoint();
		Position n1 = Position(choke->Pos(choke->end1));
		Position n2 = Position(choke->Pos(choke->end2));
		auto dx1 = n2.x - n1.x;
		auto dy1 = n2.y - n1.y;
		auto dx2 = n1.x - n2.x;
		auto dy2 = n1.y - n2.y;
		Position direction1 = Position(-dy1 / 2, dx1 / 2) + Position(choke->Center());
		Position direction2 = Position(-dy2 / 2, dx2 / 2) + Position(choke->Center());
		Position trueDirection = direction1.getDistance(mapBWEM.Center()) < direction2.getDistance(mapBWEM.Center()) ? direction1 : direction2;

		if (choke == naturalChoke) {
			initialStart = TilePosition(mainChoke->Center());
			initialEnd = (TilePosition)trueDirection;
		}
		else if (choke == mainChoke) {
			initialStart = (mainTile + TilePosition(mainChoke->Center())) / 2;
			initialEnd = (TilePosition)naturalChoke->Center();
		}
		else {
			initialStart = TilePosition(wall.getArea()->Top());
			initialEnd = (TilePosition)trueDirection;
		}

		startTile = initialStart;
		endTile = initialEnd;
	}

	void Map::findCurrentHole(Wall& wall, bool ignoreOverlap)
	{
		// Check that the path points are possible to reach
		checkPathPoints(wall);

		// Get a new path
		Path newPath;
		newPath.createWallPath(*this, mapBWEM, startTile, endTile, ignoreOverlap);
		currentHole = TilePositions::None;
		currentPath = newPath.getTiles();

		// Quick check to see if the path contains our end point, if not then the path never reached the end
		if (find(currentPath.begin(), currentPath.end(), endTile) == currentPath.end()) {
			currentHole = TilePositions::None;
			currentPathSize = DBL_MAX;
		}

		// Otherwise iterate all tiles and locate the hole
		else {
			for (auto &tile : currentPath) {
				double closestGeo = DBL_MAX;
				for (auto &geo : wall.getChokePoint()->Geometry()) {
					if (overlapsCurrentWall(tile) == UnitTypes::None && TilePosition(geo) == tile && tile.getDistance(startTile) < closestGeo)
						currentHole = tile, closestGeo = tile.getDistance(startTile);
				}
			}
			currentPathSize = newPath.getDistance();
		}
	}

	void Map::addToWall(UnitType building, Wall& wall, UnitType tight)
	{
		auto distance = DBL_MAX;
		auto tileBest = TilePositions::Invalid;
		auto start = TilePosition(wall.getCentroid());
		auto centroidDist = wall.getCentroid().getDistance(Position(endTile));
		auto end = Position(endTile);
		auto doorCenter = Position(wall.getDoor()) + Position(16, 16);

		auto furthest = 0.0;
		// Find the furthest non pylon building to the chokepoint
		for (auto &tile : wall.largeTiles()) {
			Position p = Position(tile) + Position(64, 48);
			auto dist = p.getDistance((Position)wall.getChokePoint()->Center());
			if (dist > furthest)
				furthest = dist;
		}
		for (auto &tile : wall.mediumTiles()) {
			Position p = Position(tile) + Position(48, 32);
			auto dist = p.getDistance((Position)wall.getChokePoint()->Center());
			if (dist > furthest)
				furthest = dist;
		}

		// Iterate around wall centroid to find a suitable position
		for (auto x = start.x - 8; x <= start.x + 8; x++) {
			for (auto y = start.y - 8; y <= start.y + 8; y++) {
				const TilePosition t(x, y);
				const auto center = (Position(t) + Position(32, 32));

				if (!t.isValid()
					|| overlapsAnything(t, building.tileWidth(), building.tileHeight())
					|| !isPlaceable(building, t)
					|| tilesWithinArea(wall.getArea(), t, 2, 2) == 0
					|| (building == UnitTypes::Protoss_Photon_Cannon && (center.getDistance((Position)wall.getChokePoint()->Center()) < furthest || center.getDistance(doorCenter) < 96.0)))
					continue;

				const auto dist = center.getDistance(doorCenter);

				if (dist < distance)
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
}
