#include "PathFind.h"
#include "BWEB.h"
#include "JPS.h"

using namespace std;
using namespace BWAPI;

namespace BWEB
{
    namespace {
        struct UnitCollision {
            inline bool operator()(unsigned x, unsigned y) const
            {
                TilePosition t(x, y);
                if (x < width && y < height && Map::isUsed(t) == UnitTypes::None && Map::isWalkable(t))
                    return true;
                return false;
            }
            unsigned width = Broodwar->mapWidth(), height = Broodwar->mapHeight();
        };

        struct WallCollision {
            inline bool operator()(unsigned x, unsigned y) const
            {
                TilePosition t(x, y);
                if (x < width && y < height && Walls::overlapsCurrentWall(t) == UnitTypes::None && Map::isUsed(t) == UnitTypes::None && Map::isWalkable(t))
                    return true;
                return false;
            }
            map<TilePosition, UnitType> currentWall;
            unsigned width = Broodwar->mapWidth(), height = Broodwar->mapHeight();
            bool ignoreOverlap;
        };

        map<const BWEM::Area *, int> notReachableThisFrame;
    }

    void Path::createWallPath(map<TilePosition, UnitType>& currentWall, const Position s, const Position t, bool ignoreOverlap, bool allowLifted)
    {
        TilePosition target = Map::tConvert(t);
        TilePosition source = Map::tConvert(s);
        auto maxDist = source.getDistance(target);
        vector<TilePosition> direction{ { 0, 1 },{ 1, 0 },{ -1, 0 },{ 0, -1 } };

        const auto collision = [&](const TilePosition tile) {
            return !tile.isValid()
                || tile.getDistance(target) > maxDist * 1.2
                || (!ignoreOverlap && Map::isOverlapping(tile))
                || !Map::isWalkable(tile)
                || Map::isUsed(tile) != UnitTypes::None
                || (Walls::overlapsCurrentWall(tile) != UnitTypes::None && (!allowLifted || Walls::overlapsCurrentWall(tile) != UnitTypes::Terran_Barracks));
        };

        createPath(s, t, collision, direction);
    }

    void Path::createUnitPath(const Position s, const Position t)
    {
        TilePosition target = Map::tConvert(t);
        TilePosition source = Map::tConvert(s);

        auto checkReachable = notReachableThisFrame[Map::mapBWEM.GetArea(target)];
        if (checkReachable >= Broodwar->getFrameCount()) {
            reachable = false;
            dist = DBL_MAX;
            return;
        }

        vector<TilePosition> newJPSPath;
        UnitCollision collision;

        if (JPS::findPath(newJPSPath, collision, source.x, source.y, target.x, target.y)) {
            Position current = s;
            for (auto &t : newJPSPath) {
                dist += Map::pConvert(t).getDistance(current);
                current = Map::pConvert(t);
                tiles.push_back(t);
            }
            reachable = true;
        }
        else {
            dist = DBL_MAX;
            notReachableThisFrame[Map::mapBWEM.GetArea(target)] = Broodwar->getFrameCount();
            reachable = false;
        }
    }

    void Path::createPath(const Position s, const Position t, function <bool(const TilePosition)> collision, vector<TilePosition> direction)
    {
        TilePosition source = Map::tConvert(s);
        TilePosition target = Map::tConvert(t);
        auto maxDist = source.getDistance(target);

        if (source == target || source == TilePosition(0, 0) || target == TilePosition(0, 0))
            return;

        TilePosition parentGrid[256][256];

        // This function requires that parentGrid has been filled in for a path from source to target
        const auto createPath = [&]() {
            tiles.push_back(target);
            reachable = true;
            TilePosition check = parentGrid[target.x][target.y];
            dist += Map::pConvert(target).getDistance(Map::pConvert(check));

            do {
                tiles.push_back(check);
                TilePosition prev = check;
                check = parentGrid[check.x][check.y];
                dist += Map::pConvert(prev).getDistance(Map::pConvert(check));
            } while (check != source);

            // HACK: Try to make it more accurate to positions instead of tiles
            auto correctionSource = Map::pConvert(*(tiles.end() - 1));
            auto correctionTarget = Map::pConvert(*(tiles.begin() + 1));
            dist += s.getDistance(correctionSource);
            dist += t.getDistance(correctionTarget);
            dist -= 64.0;
        };

        queue<TilePosition> nodeQueue;
        nodeQueue.emplace(source);
        parentGrid[source.x][source.y] = source;

        // While not empty, pop off top the closest TilePosition to target
        while (!nodeQueue.empty()) {
            auto const tile = nodeQueue.front();
            nodeQueue.pop();

            for (auto const &d : direction) {
                auto const next = tile + d;

                if (next.isValid()) {
                    // If next has parent or is a collision, continue
                    if (parentGrid[next.x][next.y] != TilePosition(0, 0) || collision(next))
                        continue;

                    // Check diagonal collisions where necessary
                    if ((d.x == 1 || d.x == -1) && (d.y == 1 || d.y == -1) && (collision(tile + TilePosition(d.x, 0)) || collision(tile + TilePosition(0, d.y))))
                        continue;

                    // Set parent here
                    parentGrid[next.x][next.y] = tile;

                    // If at target, return path
                    if (next == target)
                        createPath();

                    nodeQueue.emplace(next);
                }
            }
        }
    }
}