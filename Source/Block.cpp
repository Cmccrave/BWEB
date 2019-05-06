#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB::Blocks
{
    namespace {
        vector<Block> allBlocks;
        map<const BWEM::Area *, int> typePerArea;

        vector<Piece> whichPieces(int width, int height)
        {
            vector<Piece> pieces;
            if (height == 2) {
                if (width == 5)
                    pieces ={ Piece::Small, Piece::Medium };
            }
            else if (height == 4) {
                if (width == 5)
                    pieces ={ Piece::Small, Piece::Medium, Piece::Row, Piece::Small, Piece::Medium };
            }
            else if (height == 5) {
                if (width == 4)
                    pieces ={ Piece::Large, Piece::Row, Piece::Small, Piece::Small };
            }
            else if (height == 6) {
                if (width == 10)
                    pieces ={ Piece::Large, Piece::Addon, Piece::Large, Piece::Row, Piece::Large, Piece::Small, Piece::Large };
                if (width == 18)
                    pieces ={ Piece::Large, Piece::Large, Piece::Addon, Piece::Large, Piece::Large, Piece::Row, Piece::Large, Piece::Large, Piece::Small, Piece::Large, Piece::Large };
            }
            else if (height == 8) {
                if (width == 8)
                    pieces ={ Piece::Large, Piece::Large, Piece::Row, Piece::Small, Piece::Small, Piece::Small, Piece::Small, Piece::Row, Piece::Large, Piece::Large };
                if (width == 5)
                    pieces ={ Piece::Large, Piece::Row, Piece::Small, Piece::Medium, Piece::Row, Piece::Large };
            }
            return pieces;
        }

        bool canAddBlock(const TilePosition here, const int width, const int height, bool onlyBlock = false)
        {
            const auto offset = onlyBlock ? 0 : 1;

            if (!TilePosition(here.x + width + 1, here.y + height + 1).isValid())
                return false;

            // Check if a block of specified size would overlap any bases, resources or other blocks
            for (auto x = here.x - offset; x < here.x + width + offset; x++) {
                for (auto y = here.y - offset; y < here.y + height + offset; y++) {
                    const TilePosition t(x, y);
                    if (!t.isValid() || !Map::mapBWEM.GetTile(t).Buildable() || Map::isOverlapping(t) || Map::isReserved(t))
                        return false;
                }
            }
            return true;
        }

        void insertBlock(TilePosition here, vector<Piece> pieces)
        {
            Block newBlock(here, pieces);
            allBlocks.push_back(newBlock);
            Map::addOverlap(here, newBlock.width(), newBlock.height());
        }

        void findMainStartBlock()
        {
            const auto  race = Broodwar->self()->getRace();
            vector<Piece> pieces;
            bool blockFacesLeft, blockFacesUp;

            const auto creepOnCorners = [&](TilePosition here, int width, int height) {
                return Broodwar->hasCreep(here) && Broodwar->hasCreep(here + TilePosition(width, 0)) && Broodwar->hasCreep(here + TilePosition(0, height)) && Broodwar->hasCreep(here + TilePosition(width, height));
            };

            auto tileBest = TilePositions::Invalid;
            auto distBest = DBL_MAX;
            auto start = (Map::getMainTile() + (Map::getMainChoke() ? TilePosition(Map::getMainChoke()->Center()) : Map::getMainTile())) / 2;

            if (race == Races::Zerg)
                start = Map::getMainTile();

            // Try to find a block near our starting location
            for (auto x = start.x - 10; x <= start.x + 6; x++) {
                for (auto y = start.y - 10; y <= start.y + 6; y++) {
                    const TilePosition tile(x, y);

                    if (!tile.isValid()
                        || Map::mapBWEM.GetArea(tile) != Map::getMainArea())
                        continue;

                    const auto blockCenter = Position(tile) + Position(128, 80);
                    const auto dist = blockCenter.getDistance(Map::getMainPosition()) + log(blockCenter.getDistance((Position)Map::getMainChoke()->Center()));

                    if (dist < distBest && ((race == Races::Protoss && canAddBlock(tile, 8, 5, true))
                        || (race == Races::Terran && canAddBlock(tile, 6, 5, true))
                        || (race == Races::Zerg && creepOnCorners(tile, 5, 4) && canAddBlock(tile, 5, 4, true)))) {
                        tileBest = tile;
                        distBest = dist;

                        blockFacesLeft = (blockCenter.x < Map::getMainPosition().x);
                        blockFacesUp = (blockCenter.y < Map::getMainPosition().y);
                    }
                }
            }

            // If our main and natural choke are shared, try to find one in the natural area too
            if (Map::getMainChoke() == Map::getNaturalChoke()) {
                for (auto x = Map::getNaturalTile().x - 9; x <= Map::getNaturalTile().x + 6; x++) {
                    for (auto y = Map::getNaturalTile().y - 6; y <= Map::getNaturalTile().y + 5; y++) {
                        const TilePosition tile(x, y);

                        if (!tile.isValid())
                            continue;

                        const auto blockCenter = Position(tile) + Position(128, 80);
                        const auto dist = blockCenter.getDistance(Map::getMainPosition()) + blockCenter.getDistance(Position(Map::getMainChoke()->Center()));
                        if (dist < distBest && ((race == Races::Protoss && canAddBlock(tile, 8, 5)) || (race == Races::Terran && canAddBlock(tile, 6, 5)))) {
                            tileBest = tile;
                            distBest = dist;

                            blockFacesLeft = (blockCenter.x < Map::getMainPosition().x);
                            blockFacesUp = (blockCenter.y < Map::getMainPosition().y);
                        }
                    }
                }
            }

            // If we don't have a valid one, rotate and try a less efficient vertical one
            if (!tileBest.isValid()) {
                for (auto x = Map::getMainTile().x - 16; x <= Map::getMainTile().x + 20; x++) {
                    for (auto y = Map::getMainTile().y - 16; y <= Map::getMainTile().y + 20; y++) {
                        const TilePosition tile(x, y);

                        if (!tile.isValid() || Map::mapBWEM.GetArea(tile) != Map::getMainArea())
                            continue;

                        const auto blockCenter = Position(tile) + Position(80, 128);
                        const auto dist = blockCenter.getDistance(Map::getMainPosition());
                        if (dist < distBest && race == Races::Protoss && canAddBlock(tile, 5, 8)) {
                            tileBest = tile;
                            distBest = dist;
                        }
                    }
                }
                if (tileBest.isValid())
                    insertBlock(tileBest, { Piece::Large, Piece::Row, Piece::Small, Piece::Medium, Piece::Row, Piece::Large });

            }

            // Mirror the block vertically/horizontally for better placement
            else if (tileBest.isValid()) {

                // TODO: Add T/Z mirroring
                if (race == Races::Zerg)
                    pieces ={ Piece::Small, Piece::Medium, Piece::Row, Piece::Medium, Piece::Small };
                else if (race == Races::Terran)
                    pieces ={ Piece::Large, Piece::Addon, Piece::Row, Piece::Medium, Piece::Medium };
                else if (race == Races::Protoss) {
                    if (blockFacesLeft) {
                        if (blockFacesUp)
                            pieces ={ Piece::Large, Piece::Large, Piece::Row, Piece::Medium, Piece::Medium, Piece::Small };                            
                        else
                            pieces ={ Piece::Medium, Piece::Medium, Piece::Small, Piece::Row, Piece::Large, Piece::Large };
                    }
                    else {
                        if (blockFacesUp)
                            pieces ={ Piece::Large, Piece::Large, Piece::Row, Piece::Small, Piece::Medium, Piece::Medium };
                        else
                            pieces ={ Piece::Small, Piece::Medium, Piece::Medium, Piece::Row, Piece::Large, Piece::Large };
                    }
                }

                insertBlock(tileBest, pieces);
            }
        }

        void findMainDefenseBlock()
        {
            // Added a block that allows a good shield battery placement or bunker placement
            auto tileBest = TilePositions::Invalid;
            auto start = TilePosition(Map::getMainChoke()->Center());
            auto distBest = DBL_MAX;
            for (auto x = start.x - 12; x <= start.x + 16; x++) {
                for (auto y = start.y - 12; y <= start.y + 16; y++) {
                    TilePosition tile(x, y);

                    if (!tile.isValid() || Map::mapBWEM.GetArea(tile) != Map::getMainArea())
                        continue;

                    const auto blockCenter = Position(tile) + Position(80, 32);
                    const auto dist = (blockCenter.getDistance((Position)Map::getMainChoke()->Center()));
                    if (dist < distBest && canAddBlock(tile, 5, 2)) {
                        tileBest = tile;
                        distBest = dist;
                    }
                }
            }

            if (tileBest.isValid())
                insertBlock(tileBest, { Piece::Small, Piece::Medium });
        }

        void findProductionBlocks()
        {
            multimap<double, TilePosition> tilesByPathDist;
            map<Piece, int> mainPieces;

            // Calculate distance for each tile to our natural choke, we want to place bigger blocks closer to the chokes
            for (int y = 0; y < Broodwar->mapHeight(); y++) {
                for (int x = 0; x < Broodwar->mapWidth(); x++) {
                    const TilePosition t(x, y);
                    if (t.isValid() && Broodwar->isBuildable(t)) {
                        const auto p = Position(x * 32, y * 32);
                        const auto dist = Map::getNaturalChoke() ? p.getDistance(Position(Map::getNaturalChoke()->Center())) : p.getDistance(Map::getMainPosition());
                        tilesByPathDist.insert(make_pair(dist, t));
                    }
                }
            }

            // Iterate every tile
            for (int i = 20; i > 0; i--) {
                for (int j = 20; j > 0; j--) {

                    // Check if we have pieces to use
                    const auto pieces = whichPieces(i, j);
                    if (pieces.empty())
                        continue;

                    const auto hasMedium = find(pieces.begin(), pieces.end(), Piece::Medium) != pieces.end();
                    const auto hasLarge = find(pieces.begin(), pieces.end(), Piece::Large) != pieces.end();

                    for (auto &[_, tile] : tilesByPathDist) {

                        if (hasMedium && Map::mapBWEM.GetArea(tile) != Map::getMainArea() && Broodwar->self()->getRace() == Races::Protoss)
                            continue;
                        if (hasLarge && Map::mapBWEM.GetArea(tile) == Map::getMainArea() && mainPieces[Piece::Large] >= 8 && mainPieces[Piece::Medium] < 10)
                            continue;

                        if (canAddBlock(tile, i, j)) {
                            insertBlock(tile, pieces);
                            if (Map::mapBWEM.GetArea(tile) == Map::getMainArea()) {
                                for (auto &piece : pieces)
                                    mainPieces[piece]++;
                            }
                        }
                    }
                }
            }
        }        
    }

    void eraseBlock(const TilePosition here)
    {
        for (auto &it = allBlocks.begin(); it != allBlocks.end(); ++it) {
            auto &block = *it;
            if (here.x >= block.getTilePosition().x && here.x < block.getTilePosition().x + block.width() && here.y >= block.getTilePosition().y && here.y < block.getTilePosition().y + block.height()) {
                allBlocks.erase(it);
                return;
            }
        }
    }

    void findBlocks()
    {
        findMainStartBlock();
        findMainDefenseBlock();
        findProductionBlocks();
    }

    void draw()
    {
        for (auto &block : allBlocks) {
            for (auto &tile : block.getSmallTiles())
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
            for (auto &tile : block.getMediumTiles())
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), Broodwar->self()->getColor());
            for (auto &tile : block.getLargeTiles())
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), Broodwar->self()->getColor());
        }
    }

    vector<Block>& getBlocks() {
        return allBlocks;
    }

    Block * getClosestBlock(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Block * bestBlock = nullptr;
        for (auto &block : allBlocks) {
            const auto tile = block.getTilePosition() + TilePosition(block.width() / 2, block.height() / 2);
            const auto dist = here.getDistance(tile);

            if (dist < distBest) {
                distBest = dist;
                bestBlock = &block;
            }
        }
        return bestBlock;
    }
}