#include "BWEB.h"

using namespace std;
using namespace BWAPI;

#include <optional>

namespace BWEB {

    namespace {
        const Station *startMainStation = nullptr;
        const Station *startNatStation  = nullptr;
        vector<Station> stations;
        vector<const BWEM::Base *> mainBases;
        vector<const BWEM::Base *> natBases;
        vector<const BWEM::ChokePoint *> mainChokes;
        vector<const BWEM::ChokePoint *> natChokes;
        set<const BWEM::ChokePoint *> nonsenseChokes;
        UnitType defenseType;
    } // namespace

    void Station::addResourceReserves()
    {
        auto closestTiles = [&](Unit resource) {
            const int aw = 4;
            const int ah = 3;

            int ax1 = base->Location().x;
            int ay1 = base->Location().y;
            int ax2 = ax1 + aw - 1;
            int ay2 = ay1 + ah - 1;

            BWAPI::TilePosition bTopLeft = resource->getTilePosition();
            int bw                       = resource->getType().tileWidth();
            int bh                       = resource->getType().tileHeight();

            int bx1 = bTopLeft.x;
            int by1 = bTopLeft.y;
            int bx2 = bx1 + bw - 1;
            int by2 = by1 + bh - 1;

            int tileAx = (ax1 + ax2) / 2;
            int tileAy = (ay1 + ay2) / 2;
            int tileBx = (bx1 + bx2) / 2;
            int tileBy = (by1 + by2) / 2;

            if (ax2 < bx1) {
                tileAx = ax2;
                tileBx = bx1;
            }
            else if (bx2 < ax1) {
                tileAx = ax1;
                tileBx = bx2;
            }

            if (ay2 < by1) {
                tileAy = ay2;
                tileBy = by1;
            }
            else if (by2 < ay1) {
                tileAy = ay1;
                tileBy = by2;
            }

            return std::make_pair(BWAPI::TilePosition(tileAx, tileAy), BWAPI::TilePosition(tileBx, tileBy));
        };

        const auto biggerReserve = [&](TilePosition start, TilePosition end) {
            vector<TilePosition> directions{{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            TilePosition next = start;
            Position pEnd     = Position(end);

            while (next.getDistance(end) > 1) {
                auto distBest = DBL_MAX;
                start         = next;
                int itrcount  = 0;
                for (auto &t : directions) {
                    auto tile = start + t;
                    auto pos  = Position(tile);

                    if (!tile.isValid())
                        continue;

                    auto dist = pos.getDistance(pEnd);
                    if (dist <= distBest) {
                        next     = tile;
                        distBest = dist;
                        itrcount++;
                    }
                }
                auto size = 1;
                Map::addReserve(next, size, size);
            }
        };

        // Add reserved tiles
        for (auto &m : base->Minerals()) {
            Map::addReserve(m->TopLeft(), 2, 1);
            auto tilePair = closestTiles(m->Unit());
            biggerReserve(tilePair.first, tilePair.second);
        }
        for (auto &g : base->Geysers()) {
            Map::addReserve(g->TopLeft(), 4, 2);
            auto tilePair = closestTiles(g->Unit());
            biggerReserve(tilePair.first, tilePair.second);
        }
    }

    void Station::initialize()
    {
        auto cnt = 0;

        // Resource and defense centroids
        for (auto &mineral : base->Minerals()) {
            resourceCentroid += mineral->Pos();
            cnt++;
        }

        if (cnt > 0)
            defenseCentroid = resourceCentroid / cnt;

        for (auto &gas : base->Geysers()) {
            defenseCentroid = (defenseCentroid + gas->Pos()) / 2;
            resourceCentroid += gas->Pos();
            cnt++;
        }

        if (cnt > 0)
            resourceCentroid = resourceCentroid / cnt;
        Map::addUsed(base->Location(), Broodwar->self()->getRace().getResourceDepot());

        if (Broodwar->self()->getRace() == Races::Protoss)
            defenseType = UnitTypes::Protoss_Photon_Cannon;
        if (Broodwar->self()->getRace() == Races::Terran)
            defenseType = UnitTypes::Terran_Missile_Turret;
        if (Broodwar->self()->getRace() == Races::Zerg)
            defenseType = UnitTypes::Zerg_Creep_Colony;
    }

    void Station::findChoke()
    {
        // Only find a Chokepoint for mains or naturals
        if (!main && !natural && base->GetArea()->ChokePoints().size() > 1)
            return;

        // Get closest partner base
        auto distBest = DBL_MAX;
        for (auto &potentialPartner : (main ? natBases : mainBases)) {
            auto dist = Map::getGroundDistance(potentialPartner->Center(), base->Center());
            if (dist < distBest) {
                partnerBase = potentialPartner;
                distBest    = dist;
            }
        }
        if (!partnerBase)
            return;

        // Get partner choke
        const BWEM::ChokePoint *partnerChoke = nullptr;
        if (main && !Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).empty()) {
            partnerChoke = Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).front();

            // Partner only has one chokepoint means we have a shared choke with this path
            if (partnerBase->GetArea()->ChokePoints().size() == 1) {
                partnerChoke = Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).back();
                if (base->GetArea()->ChokePoints().size() <= 2) {
                    for (auto &c : base->GetArea()->ChokePoints()) {
                        if (c != partnerChoke) {
                            choke = c;
                            return;
                        }
                    }
                }
            }
            else {
                choke = partnerChoke;
                return;
            }
        }

        // Only one chokepoint in this area, otherwise find an ideal one
        if (base->GetArea()->ChokePoints().size() == 1) {
            choke = base->GetArea()->ChokePoints().front();
            return;
        }

        // Find chokes between partner base and this base
        set<BWEM::ChokePoint const *> nonChokes;
        for (auto &c : Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()))
            nonChokes.insert(c);
        distBest                 = DBL_MAX;
        const BWEM::Area *second = nullptr;

        // Iterate each neighboring area to get closest to this natural area
        for (auto &area : base->GetArea()->AccessibleNeighbours()) {
            auto center     = area->Top();
            const auto dist = Position(center).getDistance(Map::mapBWEM.Center());

            // Label areas that connect with partner
            bool wrongArea = false;
            for (auto &c : area->ChokePoints()) {
                if (find(nonsenseChokes.begin(), nonsenseChokes.end(), c) == nonsenseChokes.end()) {
                    if ((!c->Blocked() && c->Pos(c->end1).getDistance(c->Pos(c->end2)) <= 2) || nonChokes.find(c) != nonChokes.end())
                        wrongArea = true;
                }
            }
            if (wrongArea)
                continue;

            if (/*center.isValid() &&*/ dist < distBest) {
                second   = area;
                distBest = dist;
            }
        }

        distBest = DBL_MAX;
        for (auto &c : base->GetArea()->ChokePoints()) {
            if (c->Blocked() || find(mainChokes.begin(), mainChokes.end(), c) != mainChokes.end() || c->Geometry().size() <= 3 || (c->GetAreas().first != second && c->GetAreas().second != second))
                continue;

            const auto dist = Position(c->Center()).getDistance(Position(partnerBase->Center()));
            if (dist < distBest) {
                choke    = c;
                distBest = dist;
            }
        }
    }

    void Station::findAngles()
    {
        if (choke)
            main ? mainChokes.push_back(choke) : natChokes.push_back(choke);

        // Create angles that dictate a lot about how the station is formed
        if (choke) {
            anglePosition = Position(choke->Center()) + Position(4, 4);
            auto dist     = getBase()->Center().getDistance(anglePosition);
            baseAngle     = Map::getAngle(make_pair(getBase()->Center(), anglePosition));
            chokeAngle    = Map::getAngle(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))));

            baseAngle  = (round(baseAngle / M_PI_D4)) * M_PI_D4;
            chokeAngle = (round(chokeAngle / M_PI_D4)) * M_PI_D4;

            defenseAngle = baseAngle + M_PI_D2;

            // Narrow chokes don't dictate our angles
            if (choke->Width() < 96.0)
                return;

            // If the chokepoint is against the map edge, we shouldn't treat it as an angular chokepoint
            // Matchpoint fix
            if (TilePosition(choke->Center()).x < 10 || TilePosition(choke->Center()).x > Broodwar->mapWidth() - 10) {
                chokeAngle   = Map::getAngle(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))));
                chokeAngle   = round(chokeAngle / M_PI_D2) * M_PI_D2;
                defenseAngle = chokeAngle;
                return;
            }

            if (base->GetArea()->ChokePoints().size() >= 3) {
                const BWEM::ChokePoint *validSecondChoke = nullptr;
                for (auto &otherChoke : base->GetArea()->ChokePoints()) {
                    if (choke == otherChoke)
                        continue;

                    if ((choke->GetAreas().first == otherChoke->GetAreas().first && choke->GetAreas().second == otherChoke->GetAreas().second) ||
                        (choke->GetAreas().first == otherChoke->GetAreas().second && choke->GetAreas().second == otherChoke->GetAreas().first))
                        validSecondChoke = otherChoke;
                }

                if (validSecondChoke)
                    defenseAngle = (Map::getAngle(make_pair(Position(choke->Center()), Position(validSecondChoke->Center()))) +
                                    Map::getAngle(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))))) /
                                   2.0;
            }

            else if ((dist >= 320.0 && !main && !natural) || (dist >= 368 && natural)) {
                auto baseMod  = fmod(baseAngle + M_PI_D2, M_PI);
                auto chokeMod = fmod(chokeAngle, M_PI);
                defenseAngle  = fmod((baseMod + chokeMod) / 2.0, M_PI);
            }
        }
    }

    void Station::findSecondaryLocations()
    {
        // Add scanner addon for Terran
        if (Broodwar->self()->getRace() == Races::Terran) {
            auto scannerTile = base->Location() + TilePosition(4, 1);
            smallPosition    = scannerTile;
            Map::addUsed(scannerTile, defenseType);
        }

        if (Broodwar->self()->getRace() != Races::Zerg)
            return;

        vector<tuple<TilePosition, TilePosition, TilePosition>> tryOrder;

        // Determine some standard positions to place simcity pool/spire/sunken
        if (main) {
            auto mineralsLeft = resourceCentroid.x < base->Center().x;
            for (auto &geyser : base->Geysers()) {

                // Geyser west of hatchery
                if (geyser->TopLeft() == base->Location() + TilePosition(-7, 1)) {

                    // North
                    if (baseAngle < 2.355 && baseAngle >= 0.785) {
                        tryOrder = {{TilePosition(-3, -2), TilePosition(-5, -1), TilePosition(0, 0)}};
                    }

                    // West
                    else if (baseAngle < 3.925 && baseAngle >= 2.355) {
                        if (mineralsLeft) {
                        }
                        else {
                            tryOrder = {{TilePosition(-3, -4), TilePosition(-5, -2), TilePosition(-3, -2)}};
                        }
                    }
                }

                // Geyser north of hatchery
                if (geyser->TopLeft() == base->Location() + TilePosition(0, -5)) {

                    // North
                    if (baseAngle < 2.355 && baseAngle >= 0.785) {
                        if (mineralsLeft) {
                            tryOrder = {{TilePosition(-3, -4), TilePosition(0, -3), TilePosition(-2, -2)}, {TilePosition(0, -3), TilePosition(-2, -4), TilePosition(-2, -2)}};
                        }
                        else {
                            tryOrder = {{TilePosition(4, -4), TilePosition(0, -3), TilePosition(4, -2)}, {TilePosition(-1, -3), TilePosition(4, -4), TilePosition(4, -2)}};
                        }
                    }

                    // West
                    else if (baseAngle < 3.925 && baseAngle >= 2.355) {
                        if (mineralsLeft) {
                        }
                        else {
                            tryOrder = {{TilePosition(-3, -4), TilePosition(-2, -2), TilePosition(0, -3)}};
                        }
                    }

                    // South
                    else if (baseAngle < 5.495 && baseAngle >= 3.925) {
                        if (mineralsLeft) {
                            tryOrder = {{TilePosition(-3, 5), TilePosition(1, 3), TilePosition(-1, 3)}, {TilePosition(-2, 5), TilePosition(1, 3), TilePosition(-1, 3)}};
                        }
                        else {
                            tryOrder = {{TilePosition(4, 5), TilePosition(1, 3), TilePosition(3, 3)},
                                        {TilePosition(3, 5), TilePosition(1, 3), TilePosition(3, 3)},
                                        {TilePosition(2, 5), TilePosition(1, 3), TilePosition(3, 3)},
                                        {TilePosition(1, 5), TilePosition(1, 3), TilePosition(3, 3)}};
                        }
                    }

                    // East
                    else {
                        if (mineralsLeft) {
                            tryOrder = {{TilePosition(4, -4), TilePosition(4, -2), TilePosition(0, -3)}};
                        }
                    }
                }

                // For each pair, we try to place the best positions first
                for (auto &[medium, small, defense] : tryOrder) {
                    if (Map::isPlaceable(UnitTypes::Zerg_Spawning_Pool, base->Location() + medium) && Map::isPlaceable(UnitTypes::Zerg_Spire, base->Location() + small) &&
                        Map::isPlaceable(UnitTypes::Zerg_Sunken_Colony, base->Location() + defense)) {
                        mediumPosition = base->Location() + medium;
                        smallPosition  = base->Location() + small;
                        Map::addUsed(smallPosition, UnitTypes::Zerg_Spire);
                        Map::addUsed(mediumPosition, UnitTypes::Zerg_Spawning_Pool);
                        break;
                    }
                }
                tryOrder.clear();
            }
        }

        const auto distCalc = [&](const auto &position) {
            if (natural && partnerBase) {
                const auto closestMain = Stations::getClosestMainStation(base->Location());
                if (closestMain && closestMain->getChokepoint())
                    return Position(closestMain->getChokepoint()->Center()).getDistance(position);
            }

            else if (!base->Minerals().empty()) {
                auto distMineralBest = DBL_MAX;
                for (auto &mineral : base->Minerals()) {
                    auto dist = mineral->Pos().getDistance(position);
                    if (dist < distMineralBest)
                        distMineralBest = dist;
                }
                return distMineralBest;
            }
            return 0.0;
        };
    }

    void Station::findDefenses()
    {
        vector<TilePosition> basePlacements = {{-2, -2}, {-2, 1}, {1, -2}};
        auto here                           = base->Location();
        if (isNatural()) {
            basePlacements = {{-2, -2}, {-2, 1}, {1, -2}};
        }

        // Generate defenses
        defenseArrangement = int(round(defenseAngle / 0.785)) % 4;

        // Flip them vertically / horizontally as needed
        if (base->Center().y < defenseCentroid.y) {
            for (auto &placement : basePlacements)
                placement.y = -(placement.y - 1);
        }
        if (base->Center().x < defenseCentroid.x) {
            for (auto &placement : basePlacements)
                placement.x = -(placement.x - 2);
        }

        // If geyser is above, as it should be, add in a defense to left/right
        if (base->Geysers().size() == 1 && !isNatural()) {
            auto geyser = base->Geysers().front();
            auto tile   = geyser->TopLeft();
            if ((tile.y < base->Location().y - 3) || tile.y > base->Location().y + 5) {
                auto placement = tile - base->Location();
                basePlacements.push_back(placement + TilePosition(-2, 1));
                basePlacements.push_back(placement + TilePosition(4, 1));

                // If a placement is where miners would path, shift it over
                for (auto &spot : basePlacements) {
                    if (spot == TilePosition(2, -2))
                        spot = TilePosition(0, -2);
                }
            }
        }

        // Add a defense near each base placement if possible
        for (auto &placement : basePlacements) {
            auto tile = base->Location() + placement;
            if (Map::isPlaceable(defenseType, tile)) {
                defenses.insert(tile);
                Map::addUsed(tile, defenseType);
            }
        }
    }

    const void Station::draw() const
    {
        int color     = Broodwar->self()->getColor();
        int textColor = color == 185 ? textColor = Text::DarkGreen : Broodwar->self()->getTextColor();

        // Draw boxes around each feature
        for (auto &tile : defenses) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
            Broodwar->drawTextMap(Position(tile) + Position(4, 52), "%cS", textColor);
        }

        // Draw corresponding choke
        if (choke && base && (main || natural)) {

            if (base->GetArea()->ChokePoints().size() >= 3) {
                const BWEM::ChokePoint *validSecondChoke = nullptr;
                for (auto &otherChoke : base->GetArea()->ChokePoints()) {
                    if (choke == otherChoke)
                        continue;

                    if ((choke->GetAreas().first == otherChoke->GetAreas().first && choke->GetAreas().second == otherChoke->GetAreas().second) ||
                        (choke->GetAreas().first == otherChoke->GetAreas().second && choke->GetAreas().second == otherChoke->GetAreas().first))
                        validSecondChoke = choke;
                }

                if (validSecondChoke) {
                    Broodwar->drawLineMap(Position(validSecondChoke->Pos(validSecondChoke->end1)), Position(validSecondChoke->Pos(validSecondChoke->end2)), Colors::Grey);
                    Broodwar->drawLineMap(base->Center(), Position(validSecondChoke->Center()), Colors::Grey);
                    Broodwar->drawLineMap(Position(choke->Center()), Position(validSecondChoke->Center()), Colors::Grey);
                }
            }
        }

        Broodwar->drawCircleMap(resourceCentroid, 3, color, true);
        Broodwar->drawBoxMap(Position(base->Location()), Position(base->Location()) + Position(129, 97), color);
        Broodwar->drawTextMap(Position(base->Location()) + Position(4, 84), "%cS", textColor);

        // Draw secondary locations
        for (auto &location : secondaryLocations) {
            Broodwar->drawBoxMap(Position(location), Position(location) + Position(129, 97), color);
            Broodwar->drawTextMap(Position(location) + Position(4, 84), "%cS", textColor);
        }
        Broodwar->drawBoxMap(Position(mediumPosition), Position(mediumPosition) + Position(97, 65), color);
        Broodwar->drawTextMap(Position(mediumPosition) + Position(4, 52), "%cS", textColor);
        Broodwar->drawBoxMap(Position(smallPosition), Position(smallPosition) + Position(65, 65), color);
        Broodwar->drawTextMap(Position(smallPosition) + Position(4, 52), "%cS", textColor);
        Broodwar->drawBoxMap(Position(pocketDefense), Position(pocketDefense) + Position(65, 65), color);
        Broodwar->drawTextMap(Position(pocketDefense) + Position(4, 52), "%cS", textColor);
    }

    void Station::cleanup()
    {
        // Remove used on defenses
        for (auto &tile : defenses) {
            Map::removeUsed(tile, 2, 2);
            Map::addReserve(tile, 2, 2);
        }

        // Remove used on secondary locations
        for (auto &tile : secondaryLocations) {
            Map::removeUsed(tile, 4, 3);
            Map::addReserve(tile, 4, 3);
        }

        // Remove used on base location
        Map::removeUsed(getBase()->Location(), 4, 3);
        Map::addReserve(getBase()->Location(), 4, 3);
        Map::removeUsed(mediumPosition, 3, 2);
        Map::addReserve(mediumPosition, 3, 2);
        Map::removeUsed(smallPosition, 2, 2);
        Map::addReserve(smallPosition, 2, 2);
        Map::removeUsed(pocketDefense, 2, 2);
        Map::addReserve(pocketDefense, 2, 2);
    }
} // namespace BWEB

namespace BWEB::Stations {

    void findStations()
    {
        // Some chokepoints are purely nonsense (Vermeer has an area with ~3 chokepoints on the same tile), let's ignore those
        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &choke : area.ChokePoints()) {
                if (choke->Width() <= 32) {
                    for (auto &areaTwo : Map::mapBWEM.Areas()) {
                        for (auto &chokeTwo : area.ChokePoints()) {
                            if (chokeTwo != choke) {
                                for (auto &geo : chokeTwo->Geometry()) {
                                    if (geo.getDistance(choke->Center()) <= 4)
                                        nonsenseChokes.insert(choke);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Find all main bases
        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &base : area.Bases()) {
                if (base.Starting())
                    mainBases.push_back(&base);
            }
        }

        // Find all natural bases
        for (auto &main : mainBases) {

            const BWEM::Base *baseBest = nullptr;
            auto distBest              = DBL_MAX;

            for (auto &area : Map::mapBWEM.Areas()) {
                for (auto &base : area.Bases()) {

                    // Must have gas, be accesible and at least 5 mineral patches
                    if (base.Starting() || base.Geysers().empty() || base.GetArea()->AccessibleNeighbours().empty() || base.Minerals().size() < 5)
                        continue;

                    const auto dist = Map::getGroundDistance(base.Center(), main->Center());

                    // Sometimes we have a pocket natural, it contains 1 chokepoint
                    if (base.GetArea()->ChokePoints().size() == 1 && dist < 1600.0) {
                        distBest = 0.0;
                        baseBest = &base;
                    }

                    // Otherwise the closest base with a shared choke is the natural
                    if (dist < distBest && dist < 1600.0) {
                        distBest = dist;
                        baseBest = &base;
                    }
                }
            }

            // Store any natural we found
            if (baseBest)
                natBases.push_back(baseBest);

            // Refuse to not have a natural for each main, try again but less strict
            else {
                for (auto &area : Map::mapBWEM.Areas()) {
                    for (auto &base : area.Bases()) {
                        if (base.Starting())
                            continue;

                        const auto dist = Map::getGroundDistance(main->Center(), base.Center());
                        if (dist < distBest) {
                            distBest = dist;
                            baseBest = &base;
                        }
                    }
                }

                // Store any natural we found
                if (baseBest)
                    natBases.push_back(baseBest);
            }
        }

        // Create main stations
        for (auto &main : mainBases) {
            Station newStation(main, true, false);
            stations.push_back(newStation);
        }

        // Create natural stations
        for (auto &nat : natBases) {
            Station newStation(nat, false, true);
            stations.push_back(newStation);
        }

        // Create remaining stations
        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &base : area.Bases()) {
                if (find(natBases.begin(), natBases.end(), &base) != natBases.end() || find(mainBases.begin(), mainBases.end(), &base) != mainBases.end())
                    continue;

                Station newStation(&base, false, false);
                stations.push_back(newStation);
            }
        }

        startMainStation = Stations::getClosestMainStation(Broodwar->self()->getStartLocation());
        if (startMainStation)
            startNatStation = Stations::getClosestNaturalStation(startMainStation->getChokepoint() ? TilePosition(startMainStation->getChokepoint()->Center())
                                                                                                   : TilePosition(startMainStation->getBase()->Center()));
    }

    void draw()
    {
        for (auto &station : Stations::getStations()) {
            station.draw();
        }
    }

    const Station *getStartingMain() { return startMainStation; }

    const Station *getStartingNatural() { return startNatStation; }

    const vector<Station> &getStations() { return stations; }
} // namespace BWEB::Stations