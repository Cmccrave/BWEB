#include "BWEB.h"

using namespace std::placeholders;

namespace BWEB
{
	vector<TilePosition> Map::findPath(BWEM::Map& bwem, BWEB::Map& bweb, const TilePosition source, const TilePosition target, bool ignoreOverlap, bool ignoreWalls, bool diagonal)
	{
		if(source == target) return { target };
		
		const auto collision = [&]const TilePosition tile) {
			return !tile.isValid()
				|| (!ignoreOverlap && bweb.overlapGrid[tile.x][tile.y] > 0)
				|| !bweb.isWalkable(tile)
				|| (!ignoreWalls && bweb.overlapsCurrentWall(tile) != UnitTypes::None);
		};
		
		TilePosition parentGrid[256][256];
		
		for (int i = 0; i < 256; ++i) {
			for (int j = 0; j < 256; ++j) {
				parentGrid[i][j] = BWAPI::TilePositions::None;
			}
		}

		// This function requires that parentGrid has been filled in for a path from source to target
		const auto createPath = [&]() {
			vector<TilePosition> path;
			path.push_back(target);
			TilePosition check = target.parent;

			do {
				path.push_back(check);
				check = parentGrid[check.x][check.y];
			} while (check != source);
			return path;
		};

		auto const direction = [diagonal]() {
			vector<TilePosition> vec{ { 0, 1 },{ 1, 0 },{ -1, 0 },{ 0, -1 } };
			vector<TilePosition> diag{ { -1,-1 }, { -1, 1 }, { 1, -1 }, { 1, 1 } };
			if (diagonal) {
				vec.insert(vec.end(), diag.begin(), diag.end());
			}
			return vec;
		}();

		std::queue<BWAPI::TilePosition> nodeQueue;
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
					if (parentGrid[next.x][next.y] != BWAPI::TilePositions::None || collision(next))
						continue;
					
					// Set parent here, BFS optimization
					parentGrid[next.x][next.y] = tile;
					
					// If at target, return path
					if (next == target)
						return createPath();
					
					nodeQueue.emplace(next);
				}
			}
		}

		return {};
	}
}
