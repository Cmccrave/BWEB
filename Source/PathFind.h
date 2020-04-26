#pragma once
#include <BWAPI.h>

namespace BWEB
{
    class Wall;

    class Path {
        std::vector<BWAPI::TilePosition> tiles;
        double dist;
        bool reachable;
        BWAPI::TilePosition source, target;
    public:
        Path()
        {
            tiles ={};
            dist = 0.0;
            reachable = false;
            source = BWAPI::TilePositions::Invalid;
            target = BWAPI::TilePositions::Invalid;
        }

        /// <summary> Returns the vector of TilePositions associated with this Path. </summary>
        std::vector<BWAPI::TilePosition>& getTiles() { return tiles; }

        /// <summary> Returns the source (start) TilePosition of the Path. </summary>
        BWAPI::TilePosition getSource() { return source; }

        /// <summary> Returns the target (end) TilePosition of the Path. </summary>
        BWAPI::TilePosition getTarget() { return target; }

        /// <summary> Returns the distance from the source to the target in pixels. </summary>
        double getDistance() { return dist; }

        /// <summary> Returns a check if the path was able to reach the target. </summary>
        bool isReachable() { return reachable; }

        /// <summary> Creates a path from the source to the target using JPS and collision provided by BWEB based on walkable tiles and used tiles. </summary>
        void createUnitPath(const BWAPI::Position, const BWAPI::Position);

        /// <summary> Creates a path from the source to the target using JPS, your provided walkable function, and whether diagonals are allowed. </summary>
        void jpsPath(const BWAPI::Position source, const BWAPI::Position target, std::function <bool(const BWAPI::TilePosition&)> collision, bool diagonal = true);

        /// <summary> Creates a path from the source to the target using BFS, your provided walkable function, and whether diagonals are allowed. </summary>
        void bfsPath(const BWAPI::Position source, const BWAPI::Position target, std::function <bool(const BWAPI::TilePosition&)> collision, bool diagonal = true);
    };

    namespace Pathfinding {

        /// <summary> Clears the entire Pathfinding cache. All Paths will be generated as a new Path. </summary>
        void clearCache();

        /// <summary> Clears the Pathfinding cache for a specific walkable function. All Paths will be generated as a new Path. </summary>
        void clearCache(std::function <bool(const BWAPI::TilePosition&)>);

        /// <summary> Returns true if the TilePosition is walkable (does not include any buildings). </summary>
        bool terrainWalkable(const BWAPI::TilePosition&);

        /// <summary> Returns true if the TilePosition is walkable (includes buildings). </summary>
        bool unitWalkable(const BWAPI::TilePosition&);
    }
}
