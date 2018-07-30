#include "BWEB.h"

using namespace std::placeholders;

namespace BWEB
{
	void Path::createWallPath(BWEB::Map& mapBWEB, BWEM::Map& mapBWEM, const TilePosition source, const TilePosition target)
	{
		try {
			if (source == target) return;

			vector<const BWEM::Area *> bwemAreas;

			const auto validArea = [&](const TilePosition tile) {
				if (!mapBWEM.GetArea(tile) || find(bwemAreas.begin(), bwemAreas.end(), mapBWEM.GetArea(tile)) != bwemAreas.end())
					return true;
				return false;
			};

			const auto collision = [&](const TilePosition tile) {
				return !tile.isValid()
					|| mapBWEB.overlapGrid[tile.x][tile.y] > 0
					|| !mapBWEB.isWalkable(tile)
					|| mapBWEB.usedGrid[tile.x][tile.y] > 0
					|| mapBWEB.overlapsCurrentWall(tile) != UnitTypes::None;
			};

			TilePosition parentGrid[256][256];

			// This function requires that parentGrid has been filled in for a path from source to target
			const auto createPath = [&]() {
				tiles.push_back(target);
				TilePosition check = parentGrid[target.x][target.y];

				do {
					tiles.push_back(check);
					check = parentGrid[check.x][check.y];
				} while (check != source);
				return;
			};

			auto const direction = []() {
				vector<TilePosition> vec{ { 0, 1 },{ 1, 0 },{ -1, 0 },{ 0, -1 } };
				return vec;
			}();

			for (auto &choke : mapBWEM.GetPath((Position)source, (Position)target)) {
				bwemAreas.push_back(choke->GetAreas().first);
				bwemAreas.push_back(choke->GetAreas().second);
			}
			bwemAreas.push_back(mapBWEM.GetArea(source));
			bwemAreas.push_back(mapBWEM.GetArea(target));

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
						if (parentGrid[next.x][next.y] != TilePosition(0, 0) || collision(next))
							continue;

						// Set parent here, BFS optimization
						parentGrid[next.x][next.y] = tile;

						// If at target, return path
						if (next == target) {
							createPath();
							return;
						}

						nodeQueue.emplace(next);
					}
				}
			}
		}
		catch (exception ex) {
			return;
		}
	}

	

	Path::Path() 
	{
		tiles ={};
		dist = 0.0;
	}
}