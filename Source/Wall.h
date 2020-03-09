#pragma once
#include <set>
#include <BWAPI.h>
#include <bwem.h>

namespace BWEB {

    class Wall
    {
        BWAPI::UnitType tightType;
        BWAPI::TilePosition opening;
        BWAPI::Position centroid;

        std::set<BWAPI::TilePosition> defenses, smallTiles, mediumTiles, largeTiles;
        std::vector<BWAPI::UnitType> rawBuildings, rawDefenses;
        std::map<BWAPI::TilePosition, BWAPI::UnitType> currentLayout, bestLayout;

        BWAPI::TilePosition node1Tight = BWAPI::TilePositions::Invalid, node2Tight = BWAPI::TilePositions::Invalid;
        const BWEM::Area * area;
        const BWEM::ChokePoint * choke;
        const BWEM::Base * base;
        bool pylonWall, openWall, requireTight, movedStart;
        double chokeAngle;

        std::vector<BWAPI::UnitType>::iterator typeIterator;
        BWAPI::TilePosition initialPathStart, initialPathEnd, pathStart, pathEnd;
        double bestWallScore = 0.0;
        double jpsDist = 0.0;

        bool powerCheck(const BWAPI::UnitType, const BWAPI::TilePosition);
        bool angleCheck(const BWAPI::UnitType, const BWAPI::TilePosition);
        bool placeCheck(const BWAPI::UnitType, const BWAPI::TilePosition);
        bool tightCheck(const BWAPI::UnitType, const BWAPI::TilePosition);
        bool spawnCheck(const BWAPI::UnitType, const BWAPI::TilePosition);

        void cleanup();
        void initializePathPoints();
        void checkPathPoints();
        Path findOpeningInWall();
        void initialize();
        void addCentroid();
        void addOpening();
        void addDefenses();

    public:
        Wall(const BWEM::Area * _area, const BWEM::ChokePoint * _choke, std::vector<BWAPI::UnitType> _buildings, std::vector<BWAPI::UnitType> _defenses, BWAPI::UnitType _tightType, bool _requireTight, bool _openWall) {
            area = _area;
            choke = _choke;
            opening = BWAPI::TilePositions::Invalid;
            rawBuildings = _buildings;
            rawDefenses = _defenses;
            openWall = _openWall;
            requireTight = _requireTight;
            tightType = _tightType;

            bestWallScore = 0.0;
            pathStart = BWAPI::TilePositions::None;
            pathEnd = BWAPI::TilePositions::None;
            initialPathStart = BWAPI::TilePositions::None;
            initialPathEnd = BWAPI::TilePositions::None;

            initialize();
            addCentroid();
            addOpening();
            addDefenses();
            cleanup();
        }

        void addToWall(BWAPI::TilePosition here, BWAPI::UnitType building) {
            if (building.tileWidth() >= 4)
                largeTiles.insert(here);
            else if (building.tileWidth() >= 3)
                mediumTiles.insert(here);
            else if (building == BWAPI::UnitTypes::Protoss_Photon_Cannon
                || building == BWAPI::UnitTypes::Zerg_Creep_Colony
                || building == BWAPI::UnitTypes::Zerg_Sunken_Colony
                || building == BWAPI::UnitTypes::Zerg_Spore_Colony)
                defenses.insert(here);
            else
                smallTiles.insert(here);
        }

        void draw();

        const BWEM::ChokePoint * getChokePoint() const { return choke; }
        const BWEM::Area * getArea() const { return area; }

        /// <summary> Returns the defense locations associated with this Wall. </summary>
        std::set<BWAPI::TilePosition> getDefenses() const { return defenses; }

        /// <summary> Returns the TilePosition belonging to the opening of the wall. </summary>
        BWAPI::TilePosition getOpening() const { return opening; }

        /// <summary> Returns the TilePosition belonging to the centroid of the wall pieces. </summary>
        BWAPI::Position getCentroid() const { return centroid; }

        /// <summary> Returns the TilePosition belonging to large UnitType buildings. </summary>
        std::set<BWAPI::TilePosition>& getLargeTiles() { return largeTiles; }

        /// <summary> Returns the TilePosition belonging to medium UnitType buildings. </summary>
        std::set<BWAPI::TilePosition>& getMediumTiles() { return mediumTiles; }

        /// <summary> Returns the TilePosition belonging to small UnitType buildings. </summary>
        std::set<BWAPI::TilePosition>& getSmallTiles() { return smallTiles; }

        /// <summary> Returns the raw vector of the buildings the wall was initialzied with. </summary>
        std::vector<BWAPI::UnitType>& getRawBuildings() { return rawBuildings; }

        /// <summary> Returns the raw vector of the defenses the wall was initialzied with. </summary>
        std::vector<BWAPI::UnitType>& getRawDefenses() { return rawDefenses; }

        bool isPylonWall() { return pylonWall; }

        /// <summary> Returns the number of ground defenses associated with this Wall. </summary>
        int getGroundDefenseCount();

        /// <summary> Returns the number of air defenses associated with this Wall. </summary>
        int getAirDefenseCount();
    };

    namespace Walls {

        /// <summary> Returns a map containing every BWEB::Wall keyed by BWEM::Chokepoint. </summary>
        std::map<const BWEM::ChokePoint *, Wall>& getWalls();

        /// <summary> <para> Returns a pointer to a BWEB::Wall if it has been created in the given BWEM::Area and BWEM::ChokePoint. </para>
        /// <para> Note: If you only pass a BWEM::Area or a BWEM::ChokePoint (not both), it will imply and pick a BWEB::Wall that exists within that Area or blocks that BWEM::ChokePoint. </para></summary>
        /// <param name="area"> The BWEM::Area that the BWEB::Wall resides in </param>
        /// <param name="choke"> The BWEM::Chokepoint that the BWEB::Wall blocks </param>
        Wall * getWall(const BWEM::ChokePoint *);

        /// <summary> Returns the closest BWEB::Wall to the given TilePosition. </summary>
        Wall * getClosestWall(BWAPI::TilePosition);

        /// <summary> <para> Given a vector of UnitTypes, an Area and a Chokepoint, finds an optimal wall placement, returns a valid pointer if a BWEB::Wall was created. </para>
        /// <para> Note: Highly recommend that only Terran walls attempt to be walled tight, as most Protoss and Zerg wallins have gaps to allow your units through.</para>
        /// <para> BWEB makes tight walls very differently from non tight walls and will only create a tight wall if it is completely tight. </para></summary>
        /// <param name="buildings"> A Vector of UnitTypes that you want the BWEB::Wall to consist of. </param>
        /// <param name="area"> The BWEM::Area that you want the BWEB::Wall to be contained within. </param>
        /// <param name="choke"> The BWEM::Chokepoint that you want the BWEB::Wall to block. </param>
        /// <param name="tight"> (Optional) Decides whether this BWEB::Wall intends to be walled around a specific UnitType. </param>
        /// <param name="defenses"> (Optional) A Vector of UnitTypes that you want the BWEB::Wall to have defenses consisting of. </param>
        /// <param name="openWall"> (Optional) Set as true if you want an opening in the wall for unit movement. </param>
        /// <param name="requireTight"> (Optional) Set as true if you want pixel perfect placement. </param>
        Wall * createWall(std::vector<BWAPI::UnitType>& buildings, const BWEM::Area * area, const BWEM::ChokePoint * choke, BWAPI::UnitType tight = BWAPI::UnitTypes::None, const std::vector<BWAPI::UnitType>& defenses ={}, bool openWall = false, bool requireTight = false);

        /// <summary><para> Creates a Forge Fast Expand BWEB::Wall at the natural. </para>
        /// <para> Places 1 Forge, 1 Gateway, 1 Pylon and 6 Cannons. </para></summary>
        Wall * createFFE();

        /// <summary><para> Creates a "Sim City" of Zerg buildings at the natural. </para>
        /// <para> Places 6 Sunkens, 2 Evolution Chambers and 1 Hatchery. </para>
        Wall * createZSimCity();

        /// <summary><para> Creates a full wall of Terran buildings at the main choke. </para>
        /// <para> Places 2 Depots and 1 Barracks. </para>
        Wall * createTWall();

        /// <summary> Adds a UnitType to a currently existing BWEB::Wall. </summary>
        /// <param name="type"> The UnitType you want to place at the BWEB::Wall. </param>
        /// <param name="area"> The BWEB::Wall you want to add to. </param>
        /// <param name="tight"> (Optional) Decides whether this addition to the BWEB::Wall intends to be walled around a specific UnitType. Defaults to none. </param>
        void addToWall(BWAPI::UnitType type, Wall& wall, BWAPI::UnitType tight = BWAPI::UnitTypes::None);

        /// <summary> Draws all BWEB Walls. </summary>
        void draw();
    }
}
