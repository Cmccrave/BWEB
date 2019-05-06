#pragma once
#include <set>
#include <BWAPI.h>
#include <bwem.h>

namespace BWEB {

    class Station
    {
        const BWEM::Base * base;
        std::set<BWAPI::TilePosition> defenses;
        BWAPI::Position resourceCentroid;
        int defenseCount = 0;

    public:
        Station(const BWAPI::Position newResourceCenter, const std::set<BWAPI::TilePosition>& newDefenses, const BWEM::Base* newBase)
        {
            resourceCentroid = newResourceCenter;
            defenses = newDefenses;
            base = newBase;
        }

        /// <summary> Returns the central position of the resources associated with this base including geysers. </summary>
        BWAPI::Position getResourceCentroid() const { return resourceCentroid; }

        /// <summary> Returns the set of defense locations associated with this base. </summary>
        const std::set<BWAPI::TilePosition>& getDefenseLocations() const { return defenses; }

        /// <summary> Returns the BWEM base associated with this BWEB base. </summary>
        const BWEM::Base * getBWEMBase() const { return base; }

        /// <summary> Returns the number of defenses associated with this station. </summary>
        const int getDefenseCount() const { return defenseCount; }

        /// <summary> Sets the number of defenses associated with this station. </summary>
        /// <param name="newValue"> The new defense count. </param>
        void setDefenseCount(int newValue) { defenseCount = newValue; }
    };

    namespace Stations {

        /// <summary> Initializes the building of every BWEB::Station on the map, call it only once per game. </summary>
        void findStations();

        /// <summary> Draws all BWEB Stations. </summary>
        void draw();

        /// <summary> Returns a vector containing every BWEB::Station </summary>
        std::vector<Station>& getStations();

        /// <summary> Returns the closest BWEB::Station to the given TilePosition. </summary>
        const Station * getClosestStation(BWAPI::TilePosition);

    }
}