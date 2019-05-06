#pragma once
#include <BWAPI.h>

namespace BWEB
{
    class Path {
        std::vector<BWAPI::TilePosition> tiles;
        double dist;
        bool reachable;
    public:
        Path()
        {
            tiles ={};
            dist = 0.0;
            reachable = false;
        }

        /// <summary> Returns the vector of TilePositions associated with this Path. </summary>
        std::vector<BWAPI::TilePosition>& getTiles() { return tiles; }

        /// <summary> Returns the distance from the source to the target in pixels. </summary>
        double getDistance() { return dist; }

        /// <summary> Returns a check if the path was able to reach the target. </summary>
        bool isReachable() { return reachable; }

        /// <summary> Creates a path from the source to the target using JPS and collision provided by BWEB based on walkable tiles and used tiles. </summary>
        void createUnitPath(const BWAPI::Position, const BWAPI::Position);

        /// <summary> Creates a path from the source to the target using BFS and some odd collision functionality. BWEB use mostly. </summary>
        void createWallPath(std::map<BWAPI::TilePosition, BWAPI::UnitType>&, const BWAPI::Position, const BWAPI::Position, bool, bool);

        /// <summary> Creates a path from the source to the target using JPS, your provided collision function, and directions. </summary>
        void createPath(const BWAPI::Position source, const BWAPI::Position target, std::function <bool(const BWAPI::TilePosition)> collision, std::vector<BWAPI::TilePosition> directions);
    };
}
