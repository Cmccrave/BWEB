#include "BWEBUtil.h"

namespace BWEB
{
	bool BWEBUtil::overlapsBases(TilePosition here)
	{
		for (auto &base : BWEB::Map::Instance().Bases())
		{
			TilePosition tile = base.BWEMBase()->Location();
			if (here.x >= tile.x && here.x < tile.x + 4 && here.y >= tile.y && here.y < tile.y + 3) return true;
			for (auto defense : base.DefenseLocations())
				if (here.x >= defense.x && here.x < defense.x + 2 && here.y >= defense.y && here.y < defense.y + 2) return true;
		}
		return false;
	}

	bool BWEBUtil::overlapsBlocks(TilePosition here)
	{
		for (auto &b : BWEB::Map::Instance().Production())
		{
			Block& block = b.second;
			if (here.x >= block.tile().x && here.x < block.tile().x + block.width() && here.y >= block.tile().y && here.y < block.tile().y + block.height()) return true;
		}
		return false;
	}

	bool BWEBUtil::overlapsMining(TilePosition here)
	{
		for (auto &base : BWEB::Map::Instance().Bases())
			if (here.getDistance(base.ResourceCenter()) < 5) return true;
		return false;
	}

	bool BWEBUtil::overlapsNeutrals(TilePosition here)
	{
		for (auto &m : BWEM::Map::Instance().Minerals())
		{
			TilePosition tile = m->TopLeft();
			if (here.x >= tile.x && here.x < tile.x + 2 && here.y >= tile.y && here.y < tile.y + 1) return true;
		}

		for (auto &g : BWEM::Map::Instance().Geysers())
		{
			TilePosition tile = g->TopLeft();
			if (here.x >= tile.x && here.x < tile.x + 4 && here.y >= tile.y && here.y < tile.y + 2) return true;
		}

		for (auto &n : BWEM::Map::Instance().StaticBuildings())
		{
			TilePosition tile = n->TopLeft();
			if (here.x >= tile.x && here.x < tile.x + n->Type().tileWidth() && here.y >= tile.y && here.y < tile.y + n->Type().tileHeight()) return true;
		}
		return false;
	}

	bool BWEBUtil::overlapsWalls(TilePosition here)
	{
		int x = here.x;
		int y = here.y;

		for (auto &w : BWEB::Map::Instance().getWalls())
		{
			Wall wall = w.second;
			if (x >= wall.getSmallWall().x && x < wall.getSmallWall().x + 2 && y >= wall.getSmallWall().y && y < wall.getSmallWall().y + 2) return true;
			if (x >= wall.getMediumWall().x && x < wall.getMediumWall().x + 3 && y >= wall.getMediumWall().y && y < wall.getMediumWall().y + 2) return true;
			if (x >= wall.getLargeWall().x && x < wall.getLargeWall().x + 4 && y >= wall.getLargeWall().y && y < wall.getLargeWall().y + 3) return true;
		}
		return false;
	}

	bool BWEBUtil::isWalkable(TilePosition here)
	{
		WalkPosition start = WalkPosition(here);
		for (int x = start.x; x < start.x + 4; x++)
		{
			for (int y = start.y; y < start.y + 4; y++)
			{
				if (!WalkPosition(x, y).isValid()) return false;
				if (!BWEM::Map::Instance().GetMiniTile(WalkPosition(x, y)).Walkable()) return false;
			}
		}
		return true;
	}
}