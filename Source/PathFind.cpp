#include "PathFind.h"
#include "BWEB.h"
#include "JPS.h"

using namespace std;
using namespace BWAPI;

namespace BWEB
{
    namespace {

        list<Path> pathCache;
        map<pair<TilePosition, TilePosition>, list<Path>::iterator> pathCacheRefs;
        int maxCacheSize = 10000;
        map<const BWEM::Area *, int> notReachableThisFrame;
    }

    void Path::createWallPath(const Position s, const Position t, bool allowLifted, double maxDist)
    {
        target = TilePosition(t);
        source = TilePosition(s);
        vector<TilePosition> direction{ { 0, 1 },{ 1, 0 },{ -1, 0 },{ 0, -1 } };

        const auto isWalkable = [&](const TilePosition tile) {
            if (!tile.isValid()
                || tile.getDistance(target) > maxDist + 64.0
                || !Map::isWalkable(tile)
                || Map::isUsed(tile) != UnitTypes::None
                || Map::isReserved(tile)
                || (allowLifted && Map::isUsed(tile) != UnitTypes::Terran_Barracks && Map::isUsed(tile) != UnitTypes::None)
                || (!allowLifted && Map::isUsed(tile) != UnitTypes::None))
                return false;
            return true;
        };

        //jpsPath(s, t, isWalkable, direction);
        bfsPath(s, t, isWalkable, direction);

        if (this->getDistance() > maxDist * 2)
            reachable = false;
    }

    void Path::createUnitPath(const Position s, const Position t)
    {
        target = TilePosition(t);
        source = TilePosition(s);

        // If this path does not exist in cache, remove last reference and erase reference
        auto pathPoints = make_pair(source, target);
        if (pathCacheRefs.find(pathPoints) == pathCacheRefs.end()) {
            if (pathCache.size() == maxCacheSize) {
                auto last = pathCache.back();
                pathCache.pop_back();
                pathCacheRefs.erase(make_pair(last.getSource(), last.getTarget()));
            }
        }

        // If it does exist, set this path as cached version, update reference and push cached path to the front
        else {
            auto &oldPath = pathCacheRefs[pathPoints];
            dist = oldPath->getDistance();
            tiles = oldPath->getTiles();
            reachable = oldPath->isReachable();

            pathCache.erase(pathCacheRefs[pathPoints]);
            pathCache.push_front(*this);
            pathCacheRefs[pathPoints] = pathCache.begin();
            return;
        }

        if (target.isValid() && Map::mapBWEM.GetArea(target)) {
            auto checkReachable = notReachableThisFrame[Map::mapBWEM.GetArea(target)];
            if (checkReachable >= Broodwar->getFrameCount() && Broodwar->getFrameCount() > 0) {
                reachable = false;
                dist = DBL_MAX;
                return;
            }
        }

        vector<TilePosition> newJPSPath;
        const auto width = Broodwar->mapWidth();
        const auto height = Broodwar->mapHeight();

        const auto isWalkable = [&](const int x, const int y) {
            const TilePosition tile(x, y);
            if (tile == source)
                return true;
            if (x < width && y < height && Map::isWalkable(tile) && Map::isUsed(tile) == UnitTypes::None)
                return true;
            return false;
        };

        // If we found a path, store what was found
        if (JPS::findPath(newJPSPath, isWalkable, source.x, source.y, target.x, target.y)) {
            Position current = s;
            for (auto &t : newJPSPath) {
                dist += Position(t).getDistance(current);
                current = Position(t);
                tiles.push_back(t);
            }
            reachable = true;
            tiles.push_back(target);
            tiles.push_back(source);

            // Update cache 
            pathCache.push_front(*this);
            pathCacheRefs[pathPoints] = pathCache.begin();
        }

        // If not found, set destination area as unreachable for this frame
        else if (target.isValid() && Map::mapBWEM.GetArea(target)) {
            dist = DBL_MAX;
            notReachableThisFrame[Map::mapBWEM.GetArea(target)] = Broodwar->getFrameCount();
            reachable = false;
        }
    }

    void Path::bfsPath(const Position s, const Position t, function <bool(const TilePosition)> isWalkable, vector<TilePosition> direction)
    {
        TilePosition source = TilePosition(s);
        TilePosition target = TilePosition(t);
        auto maxDist = source.getDistance(target);
        const auto width = Broodwar->mapWidth();
        const auto height = Broodwar->mapHeight();

        if (source == target
            || source == TilePosition(0, 0)
            || target == TilePosition(0, 0))
            return;

        TilePosition parentGrid[256][256];

        // This function requires that parentGrid has been filled in for a path from source to target
        const auto createPath = [&]() {
            tiles.push_back(target);
            reachable = true;
            TilePosition check = parentGrid[target.x][target.y];
            dist += Position(target).getDistance(Position(check));

            do {
                tiles.push_back(check);
                TilePosition prev = check;
                check = parentGrid[check.x][check.y];
                dist += Position(prev).getDistance(Position(check));
            } while (check != source);

            // HACK: Try to make it more accurate to positions instead of tiles
            auto correctionSource = Position(*(tiles.end() - 1));
            auto correctionTarget = Position(*(tiles.begin() + 1));
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
                    if (parentGrid[next.x][next.y] != TilePosition(0, 0) || !isWalkable(next))
                        continue;

                    // Check diagonal collisions where necessary
                    if ((d.x == 1 || d.x == -1) && (d.y == 1 || d.y == -1) && (!isWalkable(tile + TilePosition(d.x, 0)) || !isWalkable(tile + TilePosition(0, d.y))))
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

    void Path::jpsPath(const Position s, const Position t, function <bool(const TilePosition)> passedWalkable, vector<TilePosition> direction)
    {
        target = TilePosition(t);
        source = TilePosition(s);

        if (target.isValid() && Map::mapBWEM.GetArea(target)) {
            auto checkReachable = notReachableThisFrame[Map::mapBWEM.GetArea(target)];
            if (checkReachable >= Broodwar->getFrameCount() && Broodwar->getFrameCount() > 0) {
                reachable = false;
                dist = DBL_MAX;
                return;
            }
        }

        vector<TilePosition> newJPSPath;
        const auto width = Broodwar->mapWidth();
        const auto height = Broodwar->mapHeight();

        const auto isWalkable = [&](const int x, const int y) {
            const TilePosition tile(x, y);
            if (x > width || y > height || x < 0 || y < 0)
                return false;
            if (passedWalkable(tile))
                return true;
            return false;
        };

        // If we found a path, store what was found
        if (JPS::findPath(newJPSPath, isWalkable, source.x, source.y, target.x, target.y)) {
            Position current = s;
            for (auto &t : newJPSPath) {
                dist += Position(t).getDistance(current);
                current = Position(t);
                tiles.push_back(t);
            }
            reachable = true;
            tiles.push_back(target);
            tiles.push_back(source);
        }

        // If not found, set destination area as unreachable for this frame
        else if (target.isValid() && Map::mapBWEM.GetArea(target)) {
            dist = DBL_MAX;
            notReachableThisFrame[Map::mapBWEM.GetArea(target)] = Broodwar->getFrameCount();
            reachable = false;
        }
    }

    namespace Pathfinding {
        void clearCache()
        {
            pathCache.clear();
            pathCacheRefs.clear();
        }
    }
}