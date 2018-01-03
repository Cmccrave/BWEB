#pragma once
#include "BWEB.h"

namespace BWEB
{
	class Base
	{		
		const BWEM::Base * base;
		set<TilePosition> defenses;
		TilePosition resourceCenter;
		
	public:
		Base(TilePosition, set<TilePosition>, const BWEM::Base*);

		// Returns the central position of the resources associated with this base including geysers
		const TilePosition ResourceCenter() { return resourceCenter; }

		// Returns the set of defense locations associated with this base
		const set<TilePosition>& DefenseLocations() { return defenses; }

		// Returns the BWEM base associated with this BWEB base
		const BWEM::Base * BWEMBase() { return base; }
	};
}