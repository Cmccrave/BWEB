#pragma once
#include <set>
#include <BWAPI.h>

namespace BWEB {

    enum class Piece {
        Small, Medium, Large, Addon, Row
    };

    class Block
    {
        int w = 0, h = 0;
        BWAPI::TilePosition tile;
        std::set <BWAPI::TilePosition> smallTiles, mediumTiles, largeTiles;
        bool proxy = false;
        bool defensive = false;
    public:
        Block() : w(0), h(0) {};

        Block(BWAPI::TilePosition _tile, std::vector<Piece> _pieces, bool _proxy = false, bool _defensive = false) {
            tile        = _tile;
            proxy       = _proxy;
            defensive   = _defensive;

            // Arrange pieces
            auto rowHeight = 0;
            auto rowWidth = 0;
            auto here = tile;
            for (auto &p : _pieces) {
                if (p == Piece::Small) {
                    smallTiles.insert(here);
                    here += BWAPI::TilePosition(2, 0);
                    rowWidth += 2;
                    rowHeight = std::max(rowHeight, 2);
                }
                if (p == Piece::Medium) {
                    mediumTiles.insert(here);
                    here += BWAPI::TilePosition(3, 0);
                    rowWidth += 3;
                    rowHeight = std::max(rowHeight, 2);
                }
                if (p == Piece::Large) {
                    if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg && !BWAPI::Broodwar->canBuildHere(here, BWAPI::UnitTypes::Zerg_Hatchery))
                        continue;                    
                    largeTiles.insert(here);
                    here += BWAPI::TilePosition(4, 0);
                    rowWidth += 4;
                    rowHeight = std::max(rowHeight, 3);
                }
                if (p == Piece::Addon) {
                    smallTiles.insert(here + BWAPI::TilePosition(0, 1));
                    here += BWAPI::TilePosition(2, 0);
                    rowWidth += 2;
                    rowHeight = std::max(rowHeight, 2);
                }
                if (p == Piece::Row) {                    
                    w = std::max(w, rowWidth);
                    h += rowHeight;
                    rowWidth = 0;
                    rowHeight = 0;
                    here = tile + BWAPI::TilePosition(0, h);
                }
            }

            // In case there is no row piece
            w = std::max(w, rowWidth);
            h += rowHeight;
        }

        /// <summary> Returns the top left TilePosition of this Block. </summary>
        BWAPI::TilePosition getTilePosition() const { return tile; }

        /// <summary> Returns the set of TilePositions that belong to 2x2 (small) buildings. </summary>
        std::set<BWAPI::TilePosition>& getSmallTiles() { return smallTiles; }

        /// <summary> Returns the set of TilePositions that belong to 3x2 (medium) buildings. </summary>
        std::set<BWAPI::TilePosition>& getMediumTiles() { return mediumTiles; }

        /// <summary> Returns the set of TilePositions that belong to 4x3 (large) buildings. </summary>
        std::set<BWAPI::TilePosition>& getLargeTiles() { return largeTiles; }

        /// <summary> Returns true if this Block was generated for proxy usage. </summary>
        bool isProxy() { return proxy; }

        /// <summary> Returns true if this Block was generated for defensive usage. </summary>
        bool isDefensive() { return defensive; }

        /// <summary> Returns the width of the Block in TilePositions. </summary>
        int width() { return w; }

        /// <summary> Returns the height of the Block in TilePositions. </summary>
        int height() { return h; }

        /// <summary> Inserts a 2x2 (small) building at this location. </summary>
        void insertSmall(const BWAPI::TilePosition here) { smallTiles.insert(here); }

        /// <summary> Inserts a 3x2 (medium) building at this location. </summary>
        void insertMedium(const BWAPI::TilePosition here) { mediumTiles.insert(here); }

        /// <summary> Inserts a 4x3 (large) building at this location. </summary>
        void insertLarge(const BWAPI::TilePosition here) { largeTiles.insert(here); }

        /// <summary> Draws all the features of the Block. </summary>
        void draw();
    };

    namespace Blocks {

        /// <summary> Returns a vector containing every Block </summary>
        std::vector<Block>& getBlocks();

        /// <summary> Returns the closest BWEB::Block to the given TilePosition. </summary>
        Block* getClosestBlock(BWAPI::TilePosition);

        /// <summary> Initializes the building of every BWEB::Block on the map, call it only once per game. </summary>
        void findBlocks();

        /// <summary> Erases any blocks at the specified TilePosition. </summary>
        /// <param name="here"> The TilePosition that you want to delete any BWEB::Block that exists here. </param>
        void eraseBlock(BWAPI::TilePosition here);

        /// <summary> Calls the draw function for each Block that exists. </summary>
        void draw();
    }
}
