#pragma once
#include <BWAPI.h>
#include <bwem.h>
#include "Block.h"
#include "PathFind.h"
#include "Station.h"
#include "Wall.h"

namespace BWEB::Map
{
    /// <summary> Global access of BWEM for BWEB. </summary>
    inline BWEM::Map& mapBWEM = BWEM::Map::Instance();

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
    bool isReserved(BWAPI::TilePosition here, int width = 1, int height = 1, bool ignoreBlocks = false);

    /// <summary> Adds a section of BWAPI::TilePositions to the BWEB used grid. </summary>
    void addUsed(BWAPI::TilePosition tile, BWAPI::UnitType);

    /// <summary> Removes a section of BWAPI::TilePositions from the BWEB used grid. </summary>
    void removeUsed(BWAPI::TilePosition tile, int width, int height);

    /// <summary> Returns the first UnitType found in a section of BWAPI::TilePositions, if it is within BWEBs used grid. </summary>
    BWAPI::UnitType isUsed(BWAPI::TilePosition here, int width = 1, int height = 1);

    /// <summary> Returns true if a BWAPI::TilePosition is fully walkable. </summary>
    /// <param name="tile"> The BWAPI::TilePosition you want to check. </param>
    bool isWalkable(BWAPI::TilePosition tile);

    /// <summary> Returns true if the given BWAPI::UnitType is placeable at the given BWAPI::TilePosition. </summary>
    /// <param name="type"> The BWAPI::UnitType of the structure you want to build. </param>
    /// <param name="tile"> The BWAPI::TilePosition you want to build on. </param>
    bool isPlaceable(BWAPI::UnitType type, BWAPI::TilePosition tile);

    /// <summary> Returns how many BWAPI::TilePosition are within a BWEM::Area. </summary>
    /// <param name="area"> The BWEM::Area to check. </param>
    /// <param name="tile"> The BWAPI::TilePosition to check. </param>
    /// <param name="width"> Optional: the width of BWAPI::TilePositions to check. </param>
    /// <param name="height"> Optional: the height of BWAPI::TilePositions to check. </param>
    int tilesWithinArea(const BWEM::Area * area, BWAPI::TilePosition tile, int width = 1, int height = 1);

    /// <summary> Returns the closest buildable BWAPI::TilePosition for any type of structure. </summary>
    /// <param name="type"> The BWAPI::UnitType of the structure you want to build. </param>
    /// <param name="tile"> The BWAPI::TilePosition you want to build closest to. </param>
    BWAPI::TilePosition getBuildPosition(BWAPI::UnitType type, BWAPI::TilePosition tile = BWAPI::Broodwar->self()->getStartLocation());

    /// <summary> Returns the closest buildable BWAPI::TilePosition for a defensive structure. </summary>
    /// <param name="type"> The BWAPI::UnitType of the structure you want to build. </param>
    /// <param name="tile"> The BWAPI::TilePosition you want to build closest to. </param>
    BWAPI::TilePosition getDefBuildPosition(BWAPI::UnitType type, BWAPI::TilePosition tile = BWAPI::Broodwar->self()->getStartLocation());

    template <class T>
    /// <summary> Returns the estimated ground distance from one Position type to another Position type. </summary>
    /// <param name="start"> The first Position. </param>
    /// <param name="end"> The second Position. </param>
    double getGroundDistance(T start, T end);

    /// <summary> Returns the BWEM::Area of the natural expansion. </summary>
    const BWEM::Area * getNaturalArea();

    /// <summary> Returns the BWEM::Area of the main. </summary>
    const BWEM::Area * getMainArea();

    /// <summary> Returns the BWEM::Chokepoint of the natural. </summary>
    const BWEM::ChokePoint * getNaturalChoke();

    /// <summary> Returns the BWEM::Chokepoint of the main. </summary>
    const BWEM::ChokePoint * getMainChoke();

    /// Returns the BWAPI::TilePosition of the natural expansion.
    BWAPI::TilePosition getNaturalTile();

    /// Returns the BWAPI::Position of the natural expansion.
    BWAPI::Position getNaturalPosition();

    /// Returns the BWAPI::TilePosition of the main.
    BWAPI::TilePosition getMainTile();

    /// Returns the BWAPI::Position of the main.
    BWAPI::Position getMainPosition();

    /// Returns the closest BWAPI::Position that makes up the geometry of a BWEM::ChokePoint to another BWAPI::Position.
    BWAPI::Position getClosestChokeTile(const BWEM::ChokePoint *, BWAPI::Position);

    /// Returns two BWAPI::Positions representing a line of best fit for a given BWEM::ChokePoint.
    std::pair<BWAPI::Position, BWAPI::Position> lineOfBestFit(BWEM::ChokePoint const *);

    /// Returns two BWAPI::Positions perpendicular to a line at a given distance away in pixels.
    std::pair<BWAPI::Position, BWAPI::Position> perpendicularLine(std::pair<BWAPI::Position, BWAPI::Position>, double);

    /// Returns a set of BWAPI::TilePositions that make up the geometry of a BWEM::ChokePoint.
    std::set<BWAPI::TilePosition> getChokeTiles(const BWEM::ChokePoint *);

    /// Returns the angle of a pair of BWAPI::Point in degrees.
    template <class T>
    double getAngle(std::pair<T, T> p) {
        auto left = p.first.x < p.second.x ? p.first : p.second;
        auto right = left == p.first ? p.second : p.first;
        auto dy = (double(left.y - right.y));
        auto dx = (double(left.x - right.x));
        return (dx != 0.0 ? atan(dy / dx) * 180.0 / 3.14 : 90.0);
    }
}
