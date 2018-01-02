#pragma once
#include "BWEB.h"

using namespace BWAPI;
using namespace BWEM;
using namespace std;

namespace BWEB
{
	class Block
	{
		int w, h;
		TilePosition t;
		set <TilePosition> pylon, small, medium, large;
	public:
		Block() { };
		Block(int width, int height, TilePosition tile) { w = width, h = height, t = tile; }
		int width() { return w; }
		int height() { return h; }
		TilePosition tile() { return t; }
		set<TilePosition>& getPylon() { return pylon; }
		set<TilePosition>& getSmall() { return small; }
		set<TilePosition>& getMedium() { return medium; }
		set<TilePosition>& getLarge() { return large; }

		void insertPylon(TilePosition here) { pylon.insert(here); }
		void insertSmall(TilePosition here) { small.insert(here); }
		void insertMedium(TilePosition here) { medium.insert(here); }
		void insertLarge(TilePosition here) { large.insert(here); }		
	};
}
