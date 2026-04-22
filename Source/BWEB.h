#pragma once
#include <BWAPI.h>

#include <bwem.h>

#include "Block.h"
#include "PathFind.h"
#include "Station.h"
#include "Wall.h"

#define M_PI_D4 0.78539816339744830961
#define M_PI_D2 1.57079632679489661923
#define M_PI 3.14159265358979323846
#define M_PI_T2 6.28318530717958647692

namespace BWEB::Map {
    /// <summary> Global access of BWEM for BWEB. </summary>
    inline BWEM::Map &mapBWEM = BWEM::Map::Instance();

    /// <summary> Draws all BWEB::Walls, BWEB::Stations, and BWEB::Blocks when called. Call this every frame if you need debugging information. </summary>
    void draw();

    /// <summary> Called on game start to initialize the BWEB::Map. </summary>
    void onStart();

    /// <summary> Stores used tiles if it is a building. Increments defense counters for any stations where the placed building is a static defense unit. </summary>
    void onUnitDiscover(BWAPI::Unit);

    /// <summary> Removes used tiles if it is a building. Decrements defense counters for any stations where the destroyed building is a static defense unit. </summary>
    void onUnitDestroy(BWAPI::Unit);

    /// <summary> Calls BWEB::onUnitDiscover. </summary>
    void onUnitMorph(BWAPI::Unit);

    /// <summary> Adds a section of BWAPI::TilePositions to the BWEB overlap grid. </summary>
    void addReserve(BWAPI::TilePosition, int width, int height);

    /// <summary> Removes a section of BWAPI::TilePositions from the BWEB overlap grid. </summary>
    void removeReserve(BWAPI::TilePosition tile, int width, int height);

    /// <summary> Returns true if a section of BWAPI::TilePositions are within BWEBs overlap grid. </summary>
    bool isReserved(BWAPI::TilePosition here, int width = 1, int height = 1);

    /// <summary> Adds a section of BWAPI::TilePositions to the BWEB used grid. </summary>
    void addUsed(BWAPI::TilePosition tile, BWAPI::UnitType);

    /// <summary> Removes a section of BWAPI::TilePositions from the BWEB used grid. </summary>
    void removeUsed(BWAPI::TilePosition tile, int width, int height);

    /// <summary> Returns the first UnitType found in a section of BWAPI::TilePositions, if it is within BWEBs used grid. </summary>
    BWAPI::UnitType isUsed(const BWAPI::TilePosition &here, const int &width = 1, const int &height = 1);

    /// <summary> Returns true if a BWAPI::TilePosition is fully walkable. </summary>
    /// <param name="tile"> The BWAPI::TilePosition you want to check. </param>
    /// <param name="type"> The BWAPI::UnitType you want to path with. </param>
    bool isWalkable(const BWAPI::TilePosition tile, BWAPI::UnitType type = BWAPI::UnitTypes::None);

    /// <summary> Returns true if the given BWAPI::UnitType is placeable at the given BWAPI::TilePosition. </summary>
    /// <param name="type"> The BWAPI::UnitType of the structure you want to build. </param>
    /// <param name="tile"> The BWAPI::TilePosition you want to build on. </param>
    bool isPlaceable(BWAPI::UnitType type, BWAPI::TilePosition tile);

    /// <summary> Returns the estimated ground distance from one Position type to another Position type. </summary>
    /// <param name="start"> The first Position. </param>
    /// <param name="end"> The second Position. </param>
    double getGroundDistance(BWAPI::Position start, BWAPI::Position end);

    /// Returns the closest BWAPI::Position that makes up the geometry of a BWEM::ChokePoint to another BWAPI::Position.
    BWAPI::Position getClosestChokeTile(const BWEM::ChokePoint *const, BWAPI::Position);

    /// Returns two BWAPI::Positions perpendicular to a line at a given distance away in pixels.
    std::pair<BWAPI::Position, BWAPI::Position> perpendicularLine(std::pair<BWAPI::Position, BWAPI::Position>, double);

    /// Returns the angle of a pair of BWAPI::Point in degrees.
    template <class T> //
    double getAngle(std::pair<T, T> p)
    {
        return getAngle(p.first, p.second);
    }

    template <class T> //
    double getAngle(T p1, T p2)
    {
        double dy    = double(p2.y - p1.y);
        double dx    = double(p2.x - p1.x);
        double angle = std::atan2(-dy, dx);
        if (angle < 0)
            angle += M_PI_T2;
        return angle;
    }
} // namespace BWEB::Map
