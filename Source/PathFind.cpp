#include "PathFind.h"

#include "BWEB.h"
#include "JPS.h"

using namespace std;
using namespace BWAPI;

namespace BWEB {
    vector<TilePosition> fourDir{{0, 1}, {1, 0}, {-1, 0}, {0, -1}};
    vector<TilePosition> eightDir{{0, 1}, {1, 0}, {-1, 0}, {0, -1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

    namespace {

        struct Node {
            TilePosition tile{-1, -1};
            double f = 0;
            bool operator>(const Node &rhs) const { return f > rhs.f; }
        };

        struct TileData {
            TilePosition parent;
            double g;
            int lastVisitedId = 0;
            bool isClosed     = false;
        };

        std::set<TilePosition> notReachable;
        bool expectReachable(TilePosition source, TilePosition target) { return (notReachable.find(source) == notReachable.end()) || (notReachable.find(target) == notReachable.end()); }

        int oneDim(TilePosition tile) { return (tile.y * Broodwar->mapWidth()) + tile.x; }
        int currentId = 0;
        TileData tileData[65536];
    } // namespace

    void Path::generateJPS(function<bool(const TilePosition &)> passedWalkable)
    {
        static const auto width  = Broodwar->mapWidth();
        static const auto height = Broodwar->mapHeight();

        dist      = 0.0;
        reachable = false;
        tiles.clear();
        if (!expectReachable(source, target)) {
            return;
        }

        const auto isWalkable = [&](const int x, const int y) {
            const TilePosition tile(x, y);
            if (x > width || y > height || x < 0 || y < 0)
                return false;
            if (tile == source || tile == target)
                return true;
            if (passedWalkable(tile))
                return true;
            return false;
        };

        vector<TilePosition> newJPSPath;
        if (JPS::findPath(newJPSPath, isWalkable, source.x, source.y, target.x, target.y)) {
            Position current = Position(source);
            tiles.push_back(source);
            for (auto &t : newJPSPath) {
                dist += Position(t).getDistance(current);
                current = Position(t);
                tiles.push_back(t);
            }
            reachable = true;
            if (reverse)
                std::reverse(tiles.begin(), tiles.end());
        }
        else {
            notReachable.insert(source);
            notReachable.insert(target);
        }
    }

    void Path::generateAS(function<double(const TilePosition &)> passedHeuristic, function<bool(const TilePosition &)> isWalkable)
    {
        if (!source.isValid() || !target.isValid() || !expectReachable(source, target))
            return;

        // Reset IDs
        currentId++;
        if (currentId == INT_MAX) {
            currentId = 1;
            memset(tileData, 0, sizeof(tileData));
        }

        priority_queue<Node, vector<Node>, greater<Node>> openQueue;

        // Clear in case path was re-used for some reason
        dist      = 0.0;
        reachable = false;
        tiles.clear();

        // Start at the target
        int startIdx                     = oneDim(target);
        tileData[startIdx].g             = 0;
        tileData[startIdx].lastVisitedId = currentId;
        tileData[startIdx].isClosed      = false;
        tileData[startIdx].parent        = target;

        auto h = double(source.getApproxDistance(target));
        openQueue.push({target, h});

        while (!openQueue.empty()) {
            Node current = openQueue.top();
            openQueue.pop();

            int currIdx = oneDim(current.tile);

            // Check if visited already
            if (tileData[currIdx].isClosed && tileData[currIdx].lastVisitedId == currentId)
                continue;

            // Create a path if at source
            if (current.tile == source) {
                reachable         = true;
                TilePosition step = current.tile;
                while (step != target) {
                    tiles.push_back(step);
                    step = tileData[oneDim(step)].parent;
                }
                tiles.push_back(target);
                if (reverse)
                    std::reverse(tiles.begin(), tiles.end());
                return;
            }

            tileData[currIdx].isClosed = true;

            for (const auto &d : eightDir) {
                TilePosition next = current.tile + d;
                if (!next.isValid())
                    continue;

                // Costs calcs
                double moveCost = ((d.x != 0 && d.y != 0) ? 45.248 : 32.0) + passedHeuristic(next);
                double currG    = tileData[currIdx].g + moveCost;
                auto &nextData = tileData[oneDim(next)];

                // Walkability checks
                if (!isWalkable(next))
                    continue;
                if (d.x != 0 && d.y != 0) {
                    if (!isWalkable({current.tile.x + d.x, current.tile.y}) || !isWalkable({current.tile.x, current.tile.y + d.y}))
                        continue;
                }

                // Check if visited or lower score
                if (nextData.lastVisitedId != currentId || currG < nextData.g) {
                    nextData  = {current.tile, currG, currentId, false};
                    auto nextH = double(source.getApproxDistance(next));
                    openQueue.push({next, currG + nextH});
                }
            }
        }

        // Must not be reachable if we are here
        notReachable.insert(source);
        notReachable.insert(target);
    }

    bool Path::terrainWalkable(const TilePosition &tile)
    {
        if (Map::isWalkable(tile, type))
            return true;
        return false;
    }

    bool Path::unitWalkable(const TilePosition &tile)
    {
        if (tile == source || (Map::isWalkable(tile, type) && Map::isUsed(tile) == UnitTypes::None))
            return true;
        return false;
    }

    namespace Pathfinding {
        void clearCacheFully() { notReachable.clear(); }

        void clearCache(function<bool(const TilePosition &)> passedWalkable) {}

        void testCache()
        {
            for (auto &tile : notReachable) {
                Broodwar->drawCircleMap(Position(tile), 4, Colors::Blue);
            }
        }
    } // namespace Pathfinding
} // namespace BWEB