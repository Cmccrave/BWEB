#include "BWEB.h"

namespace BWEB
{
	bool Map::overlapsStations(const TilePosition here)
	{
		for (auto& station : BWEB::Map::Instance().Stations())
		{
			const auto tile = station.BWEMBase()->Location();
			if (here.x >= tile.x && here.x < tile.x + 4 && here.y >= tile.y && here.y < tile.y + 3) return true;
			for (auto& defense : station.DefenseLocations())
				if (here.x >= defense.x && here.x < defense.x + 2 && here.y >= defense.y && here.y < defense.y + 2) return true;
		}
		return false;
	}

	bool Map::overlapsBlocks(const TilePosition here)
	{
		for (auto& block : BWEB::Map::Instance().Blocks())
		{
			if (here.x >= block.Location().x && here.x < block.Location().x + block.width() && here.y >= block.Location().y && here.y < block.Location().y + block.height()) return true;
		}
		return false;
	}

	bool Map::overlapsMining(TilePosition here)
	{
		for (auto& station : BWEB::Map::Instance().Stations())
			if (here.getDistance(TilePosition(station.ResourceCentroid())) < 5) return true;
		return false;
	}

	bool Map::overlapsNeutrals(const TilePosition here)
	{
		for (auto& m : BWEM::Map::Instance().Minerals())
		{
			const auto tile = m->TopLeft();
			if (here.x >= tile.x && here.x < tile.x + 2 && here.y >= tile.y && here.y < tile.y + 1) return true;
		}

		for (auto& g : BWEM::Map::Instance().Geysers())
		{
			const auto tile = g->TopLeft();
			if (here.x >= tile.x && here.x < tile.x + 4 && here.y >= tile.y && here.y < tile.y + 2) return true;
		}

		for (auto& n : BWEM::Map::Instance().StaticBuildings())
		{
			const auto tile = n->TopLeft();
			if (here.x >= tile.x && here.x < tile.x + n->Type().tileWidth() && here.y >= tile.y && here.y < tile.y + n->Type().tileHeight()) return true;
		}

		for (auto& n : Broodwar->neutral()->getUnits())
		{
			const auto tile = n->getInitialTilePosition();
			if (here.x >= tile.x && here.x < tile.x + n->getType().tileWidth() && here.y >= tile.y && here.y < tile.y + n->getType().tileHeight()) return true;
		}
		return false;
	}

	bool Map::overlapsWalls(const TilePosition here)
	{
		const auto x = here.x;
		const auto y = here.y;

		for (auto& wall : Instance().getWalls())
		{
			for (const auto tile : wall.smallTiles())
				if (x >= tile.x && x < tile.x + 2 && y >= tile.y && y < tile.y + 2) return true;
			for (const auto tile : wall.mediumTiles())
				if (x >= tile.x && x < tile.x + 3 && y >= tile.y && y < tile.y + 2) return true;
			for (const auto tile : wall.largeTiles())
				if (x >= tile.x && x < tile.x + 4 && y >= tile.y && y < tile.y + 3) return true;

			for (auto& defense : wall.getDefenses())
				if (here.x >= defense.x && here.x < defense.x + 2 && here.y >= defense.y && here.y < defense.y + 2) return true;
		}
		return false;
	}

	bool Map::overlapsAnything(const TilePosition here, const int width, const int height, bool ignoreBlocks)
	{
		for (auto i = here.x; i < here.x + width; i++)
		{
			for (auto j = here.y; j < here.y + height; j++)
			{
				if (!TilePosition(i, j).isValid()) continue;
				if (overlapGrid[i][j] > 0) return true;
			}
		}
		return false;
	}

	bool Map::isWalkable(const TilePosition here)
	{
		const auto start = WalkPosition(here);
		for (auto x = start.x; x < start.x + 4; x++)
		{
			for (auto y = start.y; y < start.y + 4; y++)
			{
				if (!WalkPosition(x, y).isValid()) return false;
				if (!Broodwar->isWalkable(WalkPosition(x, y))) return false;
			}
		}
		return true;
	}

	int Map::tilesWithinArea(BWEM::Area const * area, const TilePosition here, const int width, const int height)
	{
		auto cnt = 0;
		for (auto x = here.x; x < here.x + width; x++)
		{
			for (auto y = here.y; y < here.y + height; y++)
			{
				TilePosition t(x, y);
				if (!t.isValid()) return false;
				if (BWEM::Map::Instance().GetArea(t) == area || !BWEM::Map::Instance().GetArea(t))
					cnt++;
			}
		}
		return cnt;
	}
}