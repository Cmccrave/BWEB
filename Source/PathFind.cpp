#include "PathFind.h"
#include "BWEB.h"
#include "JPS.h"

using namespace std;
using namespace BWAPI;
using namespace std::placeholders;

namespace BWEB::PathFinding
{
	namespace {
		struct JPSGrid {
			inline bool operator()(unsigned x, unsigned y) const
			{
				if (x < width && y < height && !Map::isUsed(TilePosition(x, y)) && Map::isWalkable(TilePosition(x, y)))
					return true;
				return false;
			}
			unsigned width = Broodwar->mapWidth(), height = Broodwar->mapHeight();
			double maxDist;
			TilePosition target;
		};
	}

	void Path::createWallPath(BWEM::Map& mapBWEM, map<TilePosition, UnitType>& currentWall, const Position s, const Position t, bool ignoreOverlap)
	{
		TilePosition target(t);
		TilePosition source(s);
		vector<TilePosition> direction{ { 0, 1 },{ 1, 0 },{ -1, 0 },{ 0, -1 } };

		const auto collision = [&](const TilePosition tile) {
			return !tile.isValid()
				|| tile.getDistance(target) > maxDist * 1.2
				|| (!ignoreOverlap && Map::isOverlapping(tile))
				|| !Map::isWalkable(tile)
				|| Map::isUsed(tile)
				|| Map::overlapsCurrentWall(currentWall, tile) != UnitTypes::None;
		};

		createPath(mapBWEM, s, t, collision, direction);
	}

	void Path::createUnitPath(BWEM::Map& mapBWEM, const Position s, const Position t)
	{
		TilePosition target(t);
		TilePosition source(s);
		vector<TilePosition> newJPSPath;
		Path newPath;
		JPSGrid newGrid;
		newGrid.maxDist = source.getDistance(target);
		newGrid.target = target;

		if (JPS::findPath(newJPSPath, newGrid, source.x, source.y, target.x, target.y)) {
			tiles = newJPSPath;
			Position current = s;
			for (auto &t : tiles) {
				dist += Position(t).getDistance(current);
				current = Position(t);
			}
		}
	}

	void Path::createPath(BWEM::Map& mapBWEM, const Position s, const Position t, function <bool(const TilePosition)> collision, vector<TilePosition> direction)
	{
		TilePosition source(s);
		TilePosition target(t);
		auto maxDist = source.getDistance(target);

		if (source == target || source == TilePosition(0, 0) || target == TilePosition(0, 0))
			return;

		TilePosition parentGrid[256][256];

		// This function requires that parentGrid has been filled in for a path from source to target
		const auto createPath = [&]() {
			tiles.push_back(target);
			TilePosition check = parentGrid[target.x][target.y];
			dist += Position(target).getDistance(Position(check));

			do {
				tiles.push_back(check);
				TilePosition prev = check;
				check = parentGrid[check.x][check.y];
				dist += Position(prev).getDistance(Position(check));
			} while (check != source);

			// HACK: Try to make it more accurate to positions instead of tiles
			auto correctionSource = Position(*(tiles.end() - 1));
			auto correctionTarget = Position(*(tiles.begin() + 1));
			dist += s.getDistance(correctionSource);
			dist += t.getDistance(correctionTarget);
			dist -= 64.0;
		};

		queue<TilePosition> nodeQueue;
		nodeQueue.emplace(source);
		parentGrid[source.x][source.y] = source;

		// While not empty, pop off top the closest TilePosition to target
		while (!nodeQueue.empty()) {
			auto const tile = nodeQueue.front();
			nodeQueue.pop();

			for (auto const &d : direction) {
				auto const next = tile + d;

				if (next.isValid()) {
					// If next has parent or is a collision, continue
					if (parentGrid[next.x][next.y] != TilePosition(0, 0) || collision(next))
						continue;

					// Check diagonal collisions where necessary
					if ((d.x == 1 || d.x == -1) && (d.y == 1 || d.y == -1) && (collision(tile + TilePosition(d.x, 0)) || collision(tile + TilePosition(0, d.y))))
						continue;

					// Set parent here
					parentGrid[next.x][next.y] = tile;

					// If at target, return path
					if (next == target)
						createPath();

					nodeQueue.emplace(next);
				}
			}
		}
	}
}
