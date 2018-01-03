#pragma once
#include "BWEB.h"

namespace BWEB
{
	using namespace BWAPI;
	using namespace std;

	class Block
	{
		int w, h;
		TilePosition t;
		set <TilePosition> small, medium, large;
	public:
		Block() { };
		Block(int width, int height, TilePosition tile) { w = width, h = height, t = tile; }
		int width() { return w; }
		int height() { return h; }

		// Returns the top left tile position of this block
		const TilePosition Location() { return t; }

		// Returns the const set of tilepositions that belong to 2x2 (small) buildings
		const set<TilePosition>& SmallTiles() { return small; }

		// Returns the const set of tilepositions that belong to 3x2 (medium) buildings
		const set<TilePosition>& MediumTiles() { return medium; }

		// Returns the const set of tilepositions that belong to 4x3 (large) buildings
		const set<TilePosition>& LargeTiles() { return large; }
		
		void insertSmall(TilePosition here) { small.insert(here); }
		void insertMedium(TilePosition here) { medium.insert(here); }
		void insertLarge(TilePosition here) { large.insert(here); }		
	};
}
