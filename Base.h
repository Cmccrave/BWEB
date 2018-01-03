#pragma once
#include "BWEB.h"

using namespace BWAPI;
using namespace std;

namespace BWEB
{
	class Base
	{
		TilePosition location, resourceCenter;
		const BWEM::Base * base;
		set<TilePosition> defenses;
		
	public:
		Base(TilePosition, TilePosition, set<TilePosition>, const BWEM::Base*);

		// Returns the top left tile of this base
		TilePosition Location() { return location; }

		// Returns the central position of the resources associated with this base including geysers
		TilePosition ResourceCenter() { return resourceCenter; }

		// Returns the set of defense locations associated with this base
		set<TilePosition>& DefenseLocations() { return defenses; }

		// Returns the BWEM base associated with this BWEB base
		const BWEM::Base * Base() { return base; }
	};
}