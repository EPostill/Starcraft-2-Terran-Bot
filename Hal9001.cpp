#include "Hal9001.h"
#include <iostream>
#include <cmath>
#include "cpp-sc2/src/sc2api/sc2_client.cc"
using namespace std;

#define DEBUG false

struct IsArmy {
    IsArmy(const ObservationInterface* obs) : observation_(obs) {}

    bool operator()(const Unit& unit) {
        auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
        for (const auto& attribute : attributes) {
            if (attribute == Attribute::Structure) {
                return false;
            }
        }
        switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::ZERG_OVERLORD: return false;
            case UNIT_TYPEID::PROTOSS_PROBE: return false;
            case UNIT_TYPEID::ZERG_DRONE: return false;
            case UNIT_TYPEID::TERRAN_SCV: return false;
            case UNIT_TYPEID::ZERG_QUEEN: return false;
            case UNIT_TYPEID::ZERG_LARVA: return false;
            case UNIT_TYPEID::ZERG_EGG: return false;
            case UNIT_TYPEID::TERRAN_MULE: return false;
            case UNIT_TYPEID::TERRAN_NUKE: return false;
            default: return true;
        }
    }

    const ObservationInterface* observation_;
};

struct IsFlying {
    bool operator()(const Unit& unit) {
        return unit.is_flying;
    }
};


void Hal9001::OnGameStart() {
    const ObservationInterface *observation = Observation();
    minerals = observation->GetMinerals();
    supplies = observation->GetFoodUsed();
    vespene = observation->GetVespene();
    canRush = false;
    hasStimpack = false;
    attacking = false;
    game_stage = 0;
    steps = 0;

    // store expansions and start location
    expansions = search::CalculateExpansionLocations(observation, Query());
    startLocation = observation->GetStartLocation();

    // get map width and height
    map_width = observation -> GetGameInfo().width;
    map_height = observation -> GetGameInfo().height;

    // get raw map_name
    std::string map_name = observation -> GetGameInfo().map_name;

    cout << map_name << endl;

    // config map_name to enum
    if(map_name == "Cactus Valley LE (Void)"){
        this -> map_name = CACTUS;
    }
    else if (map_name == "Bel'Shir Vestige LE (Void)") {
        this->map_name = BELSHIR;
    }
    else if (map_name == "Proxima Station LE") {
        this->map_name = PROXIMA;
    }
    else{
        cout << "Map Name Cannot be retrieved" << endl;
    }
    // set first depot location
    depotLocation = getFirstDepotLocation(GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front());
    mainSCV = nullptr;

    // set staging area to 0,0
    stagingArea = Point2D(0,0);

    // set corner loc of base
    Corner corner_loc = cornerLoc(GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front());

    cout << "corner loc " << corner_loc << endl;
}

//This function contains the steps we take at the start to establish ourselves
void Hal9001::BuildOrder(const ObservationInterface *observation) {

    // lists to keep track of all of our buildings (so we don't need to recount at every if statement
    Units bases = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
    Units commcenters = getCommCenters();
    Units barracks = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKS);
    Units factories = GetUnitsOfType(UNIT_TYPEID::TERRAN_FACTORY);
    Units starports = GetUnitsOfType(UNIT_TYPEID::TERRAN_STARPORT);
    Units factory_techlabs = GetUnitsOfType(UNIT_TYPEID::TERRAN_FACTORYTECHLAB);
    Units starport_reactors = GetUnitsOfType(UNIT_TYPEID::TERRAN_STARPORTREACTOR);
    Units barrack_techlabs = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
    Units barracks_reactors = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKSREACTOR);
    Units depots = getDepots();
    Units refineries = GetUnitsOfType(UNIT_TYPEID::TERRAN_REFINERY);
    Units engbays = GetUnitsOfType(UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
    Units orbcoms = GetUnitsOfType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND);
    Units bunkers = GetUnitsOfType(UNIT_TYPEID::TERRAN_BUNKER);
    Units armories = GetUnitsOfType(UNIT_TYPEID::TERRAN_ARMORY);
    Units fusioncores = GetUnitsOfType(UNIT_TYPEID::TERRAN_FUSIONCORE);

    // flying buildings
    Units fly_factories = GetUnitsOfType(UNIT_TYPEID::TERRAN_FACTORYFLYING);

    // lists to keep track of all our units
    Units marines = GetUnitsOfType(UNIT_TYPEID::TERRAN_MARINE);
    Units tanks = GetUnitsOfType(UNIT_TYPEID::TERRAN_SIEGETANK);
    Units widowmines = getWidowMines();
    Units vikings = GetUnitsOfType(UNIT_TYPEID::TERRAN_VIKINGFIGHTER);
    //list of upgrades
    auto upgrades = observation->GetUpgrades();


     //NOT PART OF BUILD ORDER - EXTRA FUNCTION FROM EMMETT
    if (supplies >= observation->GetFoodCap() - 2 && depots.size() >= 5) {
        Point3D basePos = commcenters.front()->pos;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D loc = Point2D(basePos.x + rx * 15, basePos.y + ry * 15);
        BuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, loc.x, loc.y, mainSCV);
    }

    // handle mainSCV behaviour for build orders < #2
    initializeMainSCV(bases);

    /**=========================================================================================
    Build Order # 2: Build Depot towards center from command center
    Condition: supply >= 14 and minerals > 100
    Status: DONE
    =========================================================================================**/
    if (supplies >= 14 && minerals > 100 && depots.empty()) {
        // cout << "build 2" << endl;
        // call BuildStructure
        BuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, depotLocation.x, depotLocation.y, mainSCV);
    }

    /**=========================================================================================
    Build Order # 3: Build barracks near depot
    Condition: supply >= 16 and minerals >= 150
    Status: DONE
    ========================================================================================= **/
    if (supplies >= 16 && minerals >= 150 && barracks.empty()) {
        // cout << "build 3" << endl;
        if (!depots.empty()){
            const Unit *depot = depots.front();
            buildNextTo(ABILITY_ID::BUILD_BARRACKS, depot, LEFT, 0, mainSCV);
        }
    }

    /**=========================================================================================
    Build Order # 4: Build a refinery on nearest gas
    Condition: supply >= 16 and minerals >= 75 and No refineries yet
    Status: DONE 
    =========================================================================================**/
    if (supplies >= 16 && minerals >= 75 && refineries.size() == 0) {
        // cout << "build 4" << endl;
        // Call BuildRefinery with no builder aka random scv will be assigned
        // This will seek for nearest gas supply
        BuildRefinery(bases.front());

        // ManageRefineries() will take care of assigning scvs
    }

    /**=========================================================================================
    Build Order # 5: Send a SCV to scount enemy base and train a marine
    Condition: supply >= 17 and Barracks == 1, Supply Depot == 1, Refinery == 1
    Status: NOT DONE
    =========================================================================================**/
    // if (supplies >= 17 && refineries.size() == 1 && depots.size() == 1 && barracks.size() == 1){
        
    //     const Unit *barrack = barracks.front();
    //     // train one marine
    //     if (doneConstruction(barrack) && CountUnitType(UNIT_TYPEID::TERRAN_MARINE) == 0 && barrack->orders.empty()){
    //         // cout << "build 5" << endl;
    //         Actions()->UnitCommand(barrack, ABILITY_ID::TRAIN_MARINE);

    //     }
    //     //if (!builder) {
    //     //    builder = units.back();
    //     //}
    //     // FOR MAX - send scv scout
    // }

    /**=========================================================================================
     * Build Order # 6: Upgrade to orbital command center
     * Condition: supply >= 19, minerals >= 150, Orbital Command == 0
     * Status: DONE
     *=========================================================================================*/
    if (supplies >= 19 && minerals >= 150 && CountUnitType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) == 0 && !bases.empty()) {
        //upgrade command center to orbital command
        const Unit* commcenter = bases.front();
        if (commcenter->orders.empty()){
            // cout << "build 6" << endl;
            Actions()->UnitCommand(commcenter, ABILITY_ID::MORPH_ORBITALCOMMAND, true);
        }
    }

    /***=========================================================================================
     * Build Order # 7: Build second command center (expand)
     * Condition: supply >= 20, minerals >= 400, Command Center == 0
     * Status: DONE
     *=========================================================================================*/
    if (supplies >= 20 && minerals >= 400 && CountUnitType(UNIT_TYPEID::TERRAN_COMMANDCENTER) == 0) {
        // cout << "build 7" << endl;
        //build command center
        Expand();
    } 

    // set rally point of new command center to minerals
    // if (bases.size() == 1){
    //     const Unit* commcenter = bases.front();
    //     // only set it once
    //     if (commcenter->build_progress == 0.5){
    //         Actions()->UnitCommand(commcenter, ABILITY_ID::SMART, FindNearestMineralPatch(commcenter->pos), true);
    //         // tell mainSCV to mine minerals too once its done building the command center
    //         Actions()->UnitCommand(mainSCV, ABILITY_ID::SMART, FindNearestMineralPatch(commcenter->pos), true);
    //     }
    // }

    /***=========================================================================================
     * Build Order # 8: Build reactor on barracks, and build second depot
     * Condition: Marine == 1, we have an orbital and a normal command center and one depot
     * Status: DONE
    *========================================================================================= */
    if (barracks_reactors.empty() && bases.size() == 1 && orbcoms.size() == 1 && depots.size() == 1) {
        // cout << "build 8" << endl;
        // get barrack
        const Unit* barrack = barracks.front();
        // build reactor on barracks
        Actions()->UnitCommand(barrack, ABILITY_ID::BUILD_REACTOR_BARRACKS);
        // build a depot next to the reactor
        buildNextTo(ABILITY_ID::BUILD_SUPPLYDEPOT, barrack, LEFT, 0);
    }

    /***=========================================================================================
     * Build Order # 9: Build factory in between command centers
     * Condition: supply >= 22, vespene > 100, minerals > 150, we have orbital and normal comm center and no factories
     * Status: DONE
     *=========================================================================================*/
    if (factories.empty() && supplies >= 22 && vespene > 100 && minerals > 150 && bases.size() == 1 && orbcoms.size() == 1) {
        // cout << "build 9" << endl;
        // get 1st cc => orbital now
        const Unit* cc1 = orbcoms.front();
        buildNextTo(ABILITY_ID::BUILD_FACTORY, cc1, FRONT, 2); 

    }

    /***=========================================================================================
     * Build Order # 10: build bunker towards the center of the map from the 2nd command center
     * Condition: 23 supply + 100 minerals
     * Status: DONE
     *=========================================================================================*/
    if (supplies >= 23 && minerals >= 100 && factories.size() == 1 && bunkers.size() == 0 && bases.size() == 1) {
            // cout << "build 10" << endl;
            // get command center
            const Unit* cc = bases.back();
            // build bunker towards the center from command center 2
            buildNextTo(ABILITY_ID::BUILD_BUNKER, cc, FRONTRIGHT, 4);
    }

    /***=========================================================================================
     * Build Order # 11: move 3 marines to bunker
     * Condition: once bunker is done
     * Status: DONE
     *=========================================================================================*/
    // load marines in bunker
    if (marines.size() >= 3 && bunkers.size() == 1){
        const Unit *bunker = bunkers.front();
        if (doneConstruction(bunker) && bunker->cargo_space_taken < bunker->cargo_space_max){
            Units chosen_marines = GetRandomUnits(UNIT_TYPEID::TERRAN_MARINE);
            Actions()->UnitCommand(chosen_marines, ABILITY_ID::SMART, bunker);
        }
    }

    /***=========================================================================================
     * Build Order # 12: build second refinery
     * Condition: 26 supply + ??? minerals
     * Status: DONE
     *=========================================================================================*/
    if (supplies >= 26 && minerals >= 75 && refineries.size() == 1 && orbcoms.size() == 1) {
        // cout << "build 12" << endl;
        // build second refinery next to first command center
        BuildRefinery(orbcoms.front());
    }

    /***=========================================================================================
     * Build Order # 13: unlock star port + tech lab
     * Condition: once factory from (9) finishes
     * Status: DONE
     *=========================================================================================*/
    if (factories.size() == 1 && minerals >= 150 && vespene > 100 && factory_techlabs.empty()) {
        // get factory
        const Unit* factory = factories.front();
        if (doneConstruction(factory)){
            // cout << "build 13" << endl;
            // build a star port next to the factory
            buildNextTo(ABILITY_ID::BUILD_STARPORT, factory, RIGHT, 2);
            // build tech lab on factory
            #ifdef DEBUG
            cout << "trying to build tech lab on factory" << endl;
            #endif
            //if the factory has orders, get rid of them to prioritize techlab
            Actions()->UnitCommand(factory, ABILITY_ID::BUILD_TECHLAB_FACTORY);
        }
    }
    // build another starport (modification of build order)
    if (minerals >= 150 && starports.size() == 1){
        buildNextTo(ABILITY_ID::BUILD_STARPORT, starports.front(), BEHINDRIGHT, 3);
    }

    /***=========================================================================================
     * Build Order # 16: build a 3rd supply depot in front of 2nd comm center
     * Condition : we have 2 supply depots and a 2nd command center
     * Status: DONE
    *========================================================================================= */
    if (supplies > 36 && minerals >= 100 && depots.size() == 2 && orbcoms.size() == 1 && bunkers.size() == 1) {
        // build depot
        buildNextTo(ABILITY_ID::BUILD_SUPPLYDEPOT, bunkers.front(), LEFT, 1, mainSCV);
    }

    /***=========================================================================================
     * Build Order # 19: build 3 more depots in succession behind minerals at 2nd comm center
     * Condition : we already have 3 depots and have less than 6 depots
     * Status: DONE
    *========================================================================================= */    
    if (minerals >= 100 && depots.size() >= 3 && depots.size() < 6 && !bases.empty()) {
        // get depot thats close to 2nd commcenter
        const Unit *depot = FindNearestDepot(bases.front()->pos);
        if (mainSCV->orders.empty()){
            // cout << "build 19" << endl;
            buildNextTo(ABILITY_ID::BUILD_SUPPLYDEPOT, depot, RIGHT, 0, mainSCV);
        }
    }


    /***=========================================================================================
     * Build Order # 20: build 2 more barracks next to the star port and factory
     * Condition : we have a starport, factory techlab and only one barracks so far
     * Status: DONE
    *========================================================================================= */
    if (supplies >= 46 && minerals >= 300 && barracks.size() >= 1 && barracks.size() < 3 && factories.size() == 1 && starports.size() >= 1) {
        const Unit* fa = factories.back();
        const Unit* sp = starports.back();
        // get two builders (so buildNextTo doesn't assign the same builder for both barracks)
        Units builders = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV, fa->pos, 2);
        if (builders.size() == 2){
            // cout << "build 20" << endl;
            // build barrack next to factory
            buildNextTo(ABILITY_ID::BUILD_BARRACKS, fa, FRONT, 1, builders.front());
            // build another barrack next to starport
            buildNextTo(ABILITY_ID::BUILD_BARRACKS, sp, FRONT, 1, builders.back());
        }
    }
    // build tech labs on the 2 barracks (modification of build order)
    if (minerals >= 100 && vespene >= 50 && barracks.size() == 3 && barrack_techlabs.size() < 3){
        for (const auto &b : barracks){
            if (!doneConstruction(b)){
                continue;
            }
            // don't build on the one that has a reactor
            if (b->unit_type != UNIT_TYPEID::TERRAN_BARRACKSREACTOR && b->orders.empty()){
                // TODO: it only builds a techlab on one of the barracks?
                Actions()->UnitCommand(b, ABILITY_ID::BUILD_TECHLAB_BARRACKS);
            }
        }
    }

    /***=========================================================================================
     * Build Order # 21: build reactor on both starports (modification of build order)
     * Condition : we have 2 starports
     * Status: DONE
    *========================================================================================= */
    if (supplies >= 46 && minerals >= 300 && !starports.empty() && starport_reactors.size() < 2) {
        // cout << "build 21" << endl;
        // build reactor on starport
        Actions()->UnitCommand(starports, ABILITY_ID::BUILD_REACTOR_STARPORT, true);
    }

    /***=========================================================================================
     * Build Order # 23: build eng bay and gas near 2nd comm center
     * Condition : we have 4 barracks
     * Status: DONE
    *========================================================================================= */
    if (refineries.size() == 2 && engbays.empty() && barracks.size() == 3 && bases.size() == 1 && supplies >= 48 && minerals >= 200) {
        const Unit *cc = bases.front();
        Units builders = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV, cc->pos, 2);
        if (builders.size() == 2){
            // cout << "build 23" << endl;
            buildNextTo(ABILITY_ID::BUILD_ENGINEERINGBAY, cc, FRONT, 3, builders.front());
            BuildRefinery(cc, builders.back());
        }
    }

    /***=========================================================================================
     * Build Order # 26: make another refinery near 2nd comm center
     * Condition : 2nd command center's mineral line is full
     * Status: DONE
    *========================================================================================= */    
    if (refineries.size() == 3 && bases.size() == 1 && orbcoms.size() == 1){
        const Unit *cc = bases.front();
        if (cc->assigned_harvesters >= cc->ideal_harvesters){
            // cout << "build 26" << endl;
            BuildRefinery(cc);
            buildOrderComplete = true;
            cout << "Build order complete" << endl;
        }
    }   


    // lower supply depots
    for (const auto &depot: depots){
        if (depot->unit_type != UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED && depot->orders.empty()){
            Actions()->UnitCommand(depot, ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER);
        }
    }

    // spam depots at a random location
    if (depots.size() >= 3 || buildOrderComplete){
        if (supplies >= observation->GetFoodCap() - 2){
            if (commcenters.empty()){
                return;
            }
            Point3D basePos = commcenters.front()->pos;
            float rx = GetRandomScalar();
            float ry = GetRandomScalar();
            Point2D loc = Point2D(basePos.x + rx * 15, basePos.y + ry * 15);
            BuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, loc.x, loc.y, mainSCV);
        }
    }

}

void Hal9001::setStagingArea(const ObservationInterface *observation){
    Units bunkers = GetUnitsOfType(UNIT_TYPEID::TERRAN_BUNKER);
    if (bunkers.empty()){
        return;
    }
    const Unit *bunker = bunkers.front();
    std::pair<int, int> relCor = getRelativeDir(bunker, FRONT);
    // config coordinates for building barracks
    float x = (bunker->pos.x) + 4 * relCor.first;
    float y = (bunker->pos.y) + 4 * relCor.second;
    stagingArea = Point2D(x,y);

}

void Hal9001::setCanRush(const ObservationInterface *observation){
    bool hasStimpack = false;
    bool hasCombatShield = false;
    bool hasInfantry1 = false;
    Units marines = GetUnitsOfType(UNIT_TYPEID::TERRAN_MARINE);

    // check marine hp == 55 for combat shielf
    if (marines.empty()){
        return;
    }
    const Unit *marine = marines.front();
    if (marine->health_max >= 55){
        hasCombatShield = true;
    }

    // need stimpack, combat shield and terran infantry weapons lvl 1
    auto upgrades = observation->GetUpgrades();
    for (const auto &upgrade : upgrades){
        if (upgrade == UPGRADE_ID::STIMPACK){
            // cout << "has stimpack" << endl;
            hasStimpack = true;
        } else if (upgrade == UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL1){
            // cout << "has infantry1" << endl;
            hasInfantry1 = true;
        }
    }
    canRush = hasStimpack && hasCombatShield && hasInfantry1;
}

void Hal9001::CanAttack(const ObservationInterface *observation) {
    int numMarines = CountUnitType(UNIT_TYPEID::TERRAN_MARINE);
    int numMarauders = CountUnitType(UNIT_TYPEID::TERRAN_MARAUDER);
    int numMedivacs = CountUnitType(UNIT_TYPEID::TERRAN_MEDIVAC);
    int numVikings = CountUnitType(UNIT_TYPEID::TERRAN_VIKINGFIGHTER) + CountUnitType(UNIT_TYPEID::TERRAN_VIKINGASSAULT);
    int numTanks = CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) + CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANKSIEGED);

    if (numMarines >= unit_ratios[game_stage][MARINE] &&
    numMarauders >= unit_ratios[game_stage][MARAUDER] &&
    numVikings >= unit_ratios[game_stage][VIKING] &&
    numTanks >= unit_ratios[game_stage][TANK] &&
    numMedivacs >= unit_ratios[game_stage][MEDIVAC]) {
        #ifdef DEBUG
        cout << "attack conditions met, attacking flag set" << endl;
        #endif
        attacking_army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
        attacking = true;
        return;
    }
    return;
}

void Hal9001::ManageUpgrades(const ObservationInterface* observation){
    Units engbays = GetUnitsOfType(UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
    Units barrack_techlabs = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
    auto upgrades = observation->GetUpgrades();
    // we have all our 3 upgrades
    if (upgrades.size() >= 3){
        return;
    }
    // TryBuildUnit(ABILITY_ID::RESEARCH_STIMPACK, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
    // TryBuildUnit(ABILITY_ID::RESEARCH_COMBATSHIELD, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
    TryBuildUnit(ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONS, UNIT_TYPEID::TERRAN_ENGINEERINGBAY);

    // combat shield
    if (barrack_techlabs.size() >= 2){
        const Unit *bt = barrack_techlabs.front();
        if (bt->orders.empty()){
            Actions()->UnitCommand(bt, ABILITY_ID::RESEARCH_STIMPACK);
        }
        bt = barrack_techlabs.back();
        if (bt->orders.empty()){
            Actions()->UnitCommand(bt, ABILITY_ID::RESEARCH_COMBATSHIELD);
        }        
    }

}

void Hal9001::ShouldRetreat(const ObservationInterface* observation) {
    //TODO determine retreat condition
    Units enemies = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));
    int enemyCount = enemies.size();
    int aliveCount = 0;
    int threshold = attacking_army.size() / 2;
    for (const auto &unit : attacking_army){
        if (unit->is_alive){
            ++aliveCount;
        }
    }
    // retreat if we lost more than half of our units and
    // the enemy still has more units than us
    if (aliveCount < threshold && enemyCount > aliveCount) {
        if (game_stage < 2){
            game_stage++;
        }
        attacking = false;
        attacking_army.clear();
    }

}

void Hal9001::ManageArmyProduction(const ObservationInterface* observation){
    Units barracks = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKS);
    Units factories = GetUnitsOfType(UNIT_TYPEID::TERRAN_FACTORY);
    Units starports = GetUnitsOfType(UNIT_TYPEID::TERRAN_STARPORT);

    int numMarines = CountUnitType(UNIT_TYPEID::TERRAN_MARINE);
    int numMarauders = CountUnitType(UNIT_TYPEID::TERRAN_MARAUDER);
    int numMedivacs = CountUnitType(UNIT_TYPEID::TERRAN_MEDIVAC);
    int numVikings = CountUnitType(UNIT_TYPEID::TERRAN_VIKINGFIGHTER) + CountUnitType(UNIT_TYPEID::TERRAN_VIKINGASSAULT);
    int numTanks = CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) + CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANKSIEGED);
    int numWidowMines = CountUnitType(UNIT_TYPEID::TERRAN_WIDOWMINE) + CountUnitType(UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED);

    if (observation->GetMinerals() < 300) {
        return;
    }

    if (!barracks.empty()) {
        for (auto const &barrack : barracks){
            if (barrack->orders.size() < 2 && numMarines < unit_ratios[game_stage][MARINE] * 2){
                Actions()->UnitCommand(barrack, ABILITY_ID::TRAIN_MARINE);
            }
        }
        for (auto const &barrack : barracks){
            if (barrack->orders.empty() && numMarauders < unit_ratios[game_stage][MARAUDER] * 2){
                Actions()->UnitCommand(barrack, ABILITY_ID::TRAIN_MARAUDER);
            }
        }
    }
    
    if (!starports.empty()) {
        //Medivacs
        for (auto const &starport : starports) {
            if (starport->orders.empty() && numMedivacs < unit_ratios[game_stage][MEDIVAC] * 2) {
                Actions()->UnitCommand(starport, ABILITY_ID::TRAIN_MEDIVAC);
            }
        }
        //Vikings
        for (auto const &starport : starports) {
            if (starport->orders.size() < 2 && numVikings < unit_ratios[game_stage][VIKING]) {
                Actions()->UnitCommand(starport, ABILITY_ID::TRAIN_VIKINGFIGHTER);
            }
        }
    }
    
    if (!factories.empty()) {
        // widow mines
        for (auto const &factory : factories) {
            if (factory->orders.empty() && numWidowMines < 1) {
                Actions()->UnitCommand(factory, ABILITY_ID::TRAIN_WIDOWMINE);
            }
        }        
        //tanks
        for (auto const &factory : factories) {
            if (factory->orders.size() < 2 && numTanks < unit_ratios[game_stage][TANK] * 2) {
                Actions()->UnitCommand(factory, ABILITY_ID::TRAIN_SIEGETANK);
            }
        }
    }
}

void Hal9001::ManageArmy() {
    const ObservationInterface *observation = Observation();

    Units enemies = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));
    Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    Units enemybases = observation->GetUnits(Unit::Alliance::Enemy, IsTownHall());
    Units allies = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
    Units marines = GetUnitsOfType(UNIT_TYPEID::TERRAN_MARINE);
    Units bunkers = GetUnitsOfType(UNIT_TYPEID::TERRAN_BUNKER);

    const Unit *closestEnemy;

    const Unit *homebase = bases.front();
    const Unit *squadleader;
    Point2D base_to_rush = Point2D(0,0);
    float distance = std::numeric_limits<float>::max();

    if (!marines.empty()) {
        squadleader = marines.front();
    }


    if (attacking) {

        if (!endgame) {
            if (Distance2D(allies.front()->pos, enemyBase) < 6 && enemybases.empty()) {
                endgame = true;
            }
        }

        if (enemybases.empty()){
            //we may have never seen the enemy base, try moving to the supposed location
            //if the enemy really doesn't have any bases, this won't matter anyway
            base_to_rush = enemyBase;
        }
        else {
            //if they have multiple, find which base is the closest
            for (const auto &base : enemybases) {
                float d = DistanceSquared3D(base->pos, startLocation);
                if (d < distance) {
                    distance = d;
                    base_to_rush = base->pos;
                }
            }
        }
        for (const auto &unit : allies) {
            #ifdef DEBUG
            cout << "Enemy location is" << base_to_rush.x << "," << base_to_rush.y << endl;
            #endif
            if (base_to_rush == Point2D(0,0)){
                cout << "base_to_rush is null" << endl;
                continue;
            }
            //squared distance > 25 = distance > 5
            else if (unit->is_flying && DistanceSquared2D(unit->pos, squadleader->pos) > 25) {
                Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, squadleader->pos);
            }
            else if (DistanceSquared2D(base_to_rush, unit->pos) > 25) {
                Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, base_to_rush);
            }
        }
    }
    else {

        for (const auto& unit : allies) {
            //Movement handler if we are not attacking
            if (stagingArea != Point2D(0,0) && unit->orders.empty()) {

                //if we can't rush chill in staging area
                if (DistanceSquared2D(unit->pos, stagingArea) > 16) {
                    // #ifdef DEBUG
                    // cout << "Unit pos, Staging area pos" << endl;
                    // cout << unit->pos.x << "," << unit->pos.y << " " << stagingArea.x << "," << stagingArea.y << endl;
                    // #endif
                    Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, stagingArea);
                }

                // bury widow mine at staging area
                if (unit->unit_type == UNIT_TYPEID::TERRAN_WIDOWMINE && DistanceSquared2D(unit->pos, stagingArea) < 9){
                    Actions()->UnitCommand(unit, ABILITY_ID::BURROWDOWN_WIDOWMINE);
                }
            }
        }
    }



    for (const auto &unit : allies) {
        //INDIVIDUAL UNIT BEHAVIOUR
        switch (unit->unit_type.ToType()) {
            //MARINES
            case UNIT_TYPEID::TERRAN_MARINE: {
                if (hasStimpack && !unit->orders.empty()) {
                        if (unit->orders.front().ability_id == ABILITY_ID::ATTACK) {
                            distance = std::numeric_limits<float>::max();
                            for (const auto& enemy : enemies) {
                                float d = DistanceSquared2D(enemy->pos, unit->pos);
                                if (d < distance) {
                                    closestEnemy = enemy;
                                    distance = d;
                                }
                            }
                            bool has_stimmed = false;
                            for (const auto& buff : unit->buffs) {
                                if (buff == BUFF_ID::STIMPACK) {
                                    has_stimmed = true;
                                }
                            }
                            //if we are within combat range and are not stimmed, use it
                            if (distance < 36 && !has_stimmed) {
                                Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_STIM);
                                break;
                            }
                            //EXPERIMENTAL KITING
                            if (distance < 25) {
                                float enemy_facing = closestEnemy->facing;
                                Actions()->UnitCommand(unit, ABILITY_ID::MOVE_MOVE, Point2D(unit->pos.x + sin(enemy_facing), unit->pos.y + cos(enemy_facing)));
                            }
                        }

                    }
                break;
            }
            //MARAUDERS
            case UNIT_TYPEID::TERRAN_MARAUDER: {
                if (hasStimpack && !unit->orders.empty()) {
                    if (unit->orders.front().ability_id == ABILITY_ID::ATTACK) {
                        distance = numeric_limits<float>::max();
                        for (const auto& enemy : enemies) {
                            float d = DistanceSquared2D(enemy->pos, unit->pos);
                            if (d < distance) {
                                distance = d;
                            }
                        }
                        bool has_stimmed = false;
                        for (const auto& buff : unit->buffs) {
                            if (buff == BUFF_ID::STIMPACK) {
                                has_stimmed = true;
                            }
                        }
                        //same condition as marines
                        if (distance < 49 && !has_stimmed) {
                            Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_STIM);
                            break;
                        }
                    }
                }
            }

            //WIDOWMINES
            case UNIT_TYPEID::TERRAN_WIDOWMINE: {
                float distance = numeric_limits<float>::max();
                for (const auto& enemy : enemies) {
                    float d = DistanceSquared2D(enemy->pos, unit->pos);
                    if (d < distance) {
                        distance = d;
                    }
                }
                //burrow if within range
                if (distance < 6) {
                    Actions()->UnitCommand(unit, ABILITY_ID::BURROWDOWN);
                }
                break;
            }
            
            //TANKS
            case UNIT_TYPEID::TERRAN_SIEGETANK: {
                distance = numeric_limits<float>::max();
                for (const auto& enemy : enemies) {
                    if (enemy->is_flying) {
                        continue;
                    }
                    float d = DistanceSquared2D(enemy->pos, unit->pos);
                    if (d < distance) {
                        distance = d;
                    }
                }
                //seige if within range
                if (distance < 100) {
                    Actions()->UnitCommand(unit, ABILITY_ID::MORPH_SIEGEMODE);
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED: {
                distance = numeric_limits<float>::max();
                for (const auto& enemy : enemies) {
                    float d = DistanceSquared2D(enemy->pos, unit->pos);
                    if (d < distance) {
                        distance = d;
                    }
                }
                //if there are no enemies nearby 
                if (distance > 169 || enemies.empty()) {
                    Actions()->UnitCommand(unit, ABILITY_ID::MORPH_UNSIEGE);
                }
                break;
            }

            //MEDIVACS
            case UNIT_TYPEID::TERRAN_MEDIVAC: {
                Units bio_units = observation->GetUnits(Unit::Self, IsUnits(bio_types));
                if (unit->orders.empty()) {
                    for (const auto& bio_unit : bio_units) {
                        //heal low health army units
                        if (bio_unit->health < bio_unit->health_max) {
                            Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_HEAL, bio_unit);
                            break;
                        }
                    }
                    //heal whoever is closest
                    if (!bio_units.empty()) {
                        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, bio_units.front());
                    }
                }
                break;
            }

            //VIKINGS
            case UNIT_TYPEID::TERRAN_VIKINGFIGHTER: {
                Units flying_units = observation->GetUnits(Unit::Enemy, IsFlying());

                //no flying units, should go into ground mode
                if (flying_units.empty() && !enemies.empty()) {
                    Actions()->UnitCommand(unit, ABILITY_ID::MORPH_VIKINGASSAULTMODE);
                }
                else if (!flying_units.empty()){
                    Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, flying_units.front());
                }
                break;
            }

            case UNIT_TYPEID::TERRAN_VIKINGASSAULT: {
                Units flying_units = observation->GetUnits(Unit::Enemy, IsFlying());
                
                //enemy has flying units, go into air mode
                if (!flying_units.empty() || enemies.empty()) {
                    Actions()->UnitCommand(unit, ABILITY_ID::MORPH_VIKINGFIGHTERMODE);
                }
                break;
            }

            default: {
                if (!attacking) {
                    Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, stagingArea);
                }
                break;
            }
        }
    }
}

//look through all possible enemy locations and finish them off
void Hal9001::FinalSweep(const ObservationInterface* observation) {

}



void Hal9001::initializeMainSCV(Units &bases){
    if (bases.empty()){
        return;
    }
    const Unit *commcenter = bases.front();
    // tell the first scv in training to move towards supply depot build location
    // by setting command center rally point
    if (!mainSCV && supplies == 13){
        Actions()->UnitCommand(commcenter, ABILITY_ID::SMART, depotLocation);   
    }

    if (!mainSCV && supplies == 14){
        // set the first trained scv to mainSCV
        Units scvs = GetUnitsOfType(UNIT_TYPEID::TERRAN_SCV);
        for (const auto &scv: scvs){
            const UnitOrder order = scv->orders.front();
            // this is the first trained scv
            if (order.target_pos.x != 0 && order.target_pos.y != 0 && scv != scout){
                mainSCV = scv;
            }
        }
        // set commcenter rally point back to minerals
        Actions()->UnitCommand(commcenter, ABILITY_ID::SMART, FindNearestMineralPatch(commcenter->pos)); 
    }        

}

void Hal9001::MineIdleWorkers() {
    const ObservationInterface* observation = Observation();
    Units bases = getCommCenters();
    Units workers = GetUnitsOfType(UNIT_TYPEID::TERRAN_SCV);

    const Unit* valid_mineral_patch = nullptr;

    if (bases.empty()) {
        return;
    }

    for (const auto& base : bases) {
        //If we have already mined out here skip the base.
        if (base->ideal_harvesters == 0 || base->build_progress != 1) {
            continue;
        }
        //find a base that needs workers and send scvs there
        valid_mineral_patch = FindNearestMineralPatch(base->pos);
        for (const auto&worker : workers) {
            if (worker->orders.empty()) {
                if (base->assigned_harvesters < base->ideal_harvesters) {
                    Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, valid_mineral_patch);
                }
            }
        }
        //see if bases have too many workers
        if (base->assigned_harvesters > base->ideal_harvesters) {
            Units assigned_workers = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV, base->pos, base->assigned_harvesters - base->ideal_harvesters);
            for (const auto& base2 : bases) {
                if (base2->assigned_harvesters < base2->ideal_harvesters) {
                    valid_mineral_patch = FindNearestMineralPatch(base2->pos);
                    Actions()->UnitCommand(assigned_workers, ABILITY_ID::HARVEST_GATHER, valid_mineral_patch);
                    return;
                }
            }
        }
    }
    
}

const Point2D Hal9001::getFirstDepotLocation(const Unit *commcenter){
    // get the radius of the initial command center
    float radiusCC = commcenter->radius;

    // get the radius of to-be-built structure
    float radiusSD = radiusOfToBeBuilt(ABILITY_ID::BUILD_SUPPLYDEPOT);

    // distance from CC
    // TODO: is this still too hard coded? maybe look into playable_max of game_info
    int distance = 10;

    // get relative direction
    // I want to place supply depot in relative front left of Command Center
    std::pair<int, int> relDir = getRelativeDir(commcenter, FRONTLEFT);

    // config coordinates 
    // startLocation is the location of command center
    float x = startLocation.x + (radiusCC + radiusSD + distance) * relDir.first;
    float y = startLocation.y + (radiusCC + radiusSD + distance) * relDir.second;   
    return Point2D(x, y); 
}

void Hal9001::ManageSCVTraining(){
    Units commcenters = getCommCenters();
    Units refineries = GetUnitsOfType(UNIT_TYPEID::TERRAN_REFINERY);
    int scv_count = CountUnitType(UNIT_TYPEID::TERRAN_SCV);
    int ideal_count = 0;
    // count how many scvs we need in total
    for (const auto &cc : commcenters){
        ideal_count += cc->ideal_harvesters;
    }
    for (const auto &rf : refineries){
        ideal_count += rf->ideal_harvesters;
    }

    if (scv_count < ideal_count)
    for (const auto &cc : commcenters){
        // if any comm center is not full tell all comm centers to train an scv
        // mineidleworkers will send the scvs to the not full comm centers
        if (cc->orders.empty()){
            Actions()->UnitCommand(cc, ABILITY_ID::TRAIN_SCV);
        }
    }
}


/***=========================================================================================
 * NEW FUNCTION FOR SCOUTING
 *  
 *=========================================================================================*/
void Hal9001::ReconBase(const ObservationInterface* observation) {
    
    if (enemyBaseFound == false) {
        // Get a random scv
        if (!scout || !scout->is_alive){
            Units units = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV);
            // no units
            if (units.empty()){
                return;
            }
            scout = units.front();
            scouting = false;
        }

        Point2D L1 = observation->GetGameInfo().enemy_start_locations[0];

        // Placeholder values in case number of map starting points is less than 4
        Point2D L2 = L1;
        Point2D L3 = L1;


        if (observation->GetGameInfo().enemy_start_locations.size() > 1) {
            L2 = observation->GetGameInfo().enemy_start_locations[1];
            L3 = observation->GetGameInfo().enemy_start_locations[2];
        }

        Units my_units = observation->GetUnits(Unit::Alliance::Enemy);
        for (const auto unit : my_units) {
            if (unit->unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER ||
                unit->unit_type == UNIT_TYPEID::TERRAN_SUPPLYDEPOT ||
                unit->unit_type == UNIT_TYPEID::ZERG_HATCHERY ||
                unit->unit_type == UNIT_TYPEID::PROTOSS_PYLON ||
                unit->unit_type == UNIT_TYPEID::PROTOSS_NEXUS) {
                    enemyBase = Point2D(unit->pos.x, unit->pos.y);
                    enemyBaseFound = true;
                    moveUnit(scout, mainSCV->pos);
                    scouting = false;
                    return;
            }
        }

        if (scouting == false) {
            moveUnit(scout, L1);
            scouting = true;
            cout << "going to L1\n";
        }
        if (scouting == true) {
            if (observation->GetGameInfo().enemy_start_locations.size() == 3) {
                if (scout->pos.x == L1.x && scout->pos.y == L1.y) {
                    scouting = true;
                    moveUnit(scout, L2);
                    cout << "going to L2\n";
                }
                if (scout->pos.x == L2.x && scout->pos.y == L2.y) {
                    cout << "setting base by process of elimination" << endl;
                    enemyBase = L3;
                    enemyBaseFound = true;
                    moveUnit(scout, mainSCV->pos);
                    scouting = false;
                }
            }
        }
    }
}

void Hal9001::OnStep() { 
    ++steps;
    // cout << Observation()->GetGameLoop() << endl;
    const ObservationInterface *observation = Observation();

    minerals = observation->GetMinerals();
    supplies = observation->GetFoodUsed();
    vespene = observation->GetVespene();
    ManageSCVTraining();
    MineIdleWorkers();
    ManageRefineries();


    if (!canRush && steps % 2 == 0) {
        setCanRush(observation);
    }
    if (!attacking && steps % 2 == 0) {
        CanAttack(observation);
    }
    if (enemyBase == Point2D(0,0)) {
        ReconBase(observation);
    }
    if (steps % 3 == 0) {
        BuildOrder(observation);
        ManageUpgrades(observation);
    }
    if (stagingArea == Point2D(0,0)) {
        setStagingArea(observation);
    }
    ManageArmyProduction(observation);

    if (!endgame) {
        ManageArmy();
    }
    else {
        FinalSweep(observation);
    }

    if (attacking && steps % 2 == 0) {
        ShouldRetreat(observation);
    }

}

void Hal9001::OnUnitIdle(const Unit *unit) {

}

void Hal9001::Expand(){
    Point3D exp = FindNearestExpansion();
    // build command centre
    BuildStructure(ABILITY_ID::BUILD_COMMANDCENTER, exp.x, exp.y, mainSCV);
}

const Point3D Hal9001::FindNearestExpansion(){
    bool occupied;
    float distance = std::numeric_limits<float>::max();
    Point3D closest;    // very unlikely that closest will remain uninitialized
    for (const auto &exp : expansions){
        occupied = false;
        float d = DistanceSquared3D(exp, startLocation);
        if (d < distance){
            // check if there's a command centre already on it
            Units commcenters = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
            for (const auto &cc : commcenters){
                if (cc->pos == exp){
                    occupied = true;
                    break;
                }
            }
            if (!occupied){
                distance = d;
                closest = exp;
            }
        }
    }
    return closest;
}

// returns nearest mineral patch or nullptr if none found
const Unit* Hal9001::FindNearestMineralPatch(const Point2D &start) {
    // gets all neutral units
    Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
    float distance = std::numeric_limits<float>::max();
    const Unit *target = nullptr;
    for (const auto &u : units) {
        // get closest mineral field
        if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
            float d = DistanceSquared2D(u->pos, start);
            if (d < distance) {
                distance = d;
                target = u;
            }
        }
    }
    return target;
}

// returns nearest vespene geyser or nullptr if none found
const Unit* Hal9001::FindNearestGeyser(const Point2D &start) {
    // gets all neutral units
    Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
    float distance = std::numeric_limits<float>::max();
    const Unit *target = nullptr;
    Units refineries = GetUnitsOfType(UNIT_TYPEID::TERRAN_REFINERY);
    bool valid = true;
    for (const auto &u : units) {
        if (u->unit_type == UNIT_TYPEID::NEUTRAL_VESPENEGEYSER){
            valid = true;
            // check if geyser already has a refinery on it
            for (const auto &r : refineries){
                if (u->pos == r->pos){
                    valid = false;
                    break;
                }
            }
            // get closest vespene geyser
            if (valid){
                float d = DistanceSquared2D(u->pos, start);
                if (d < distance) {
                    distance = d;
                    target = u;
                }
            }
        }
    }
    return target;
}

const Unit* Hal9001::FindNearestDepot(const Point2D &start) {
    Units units = getDepots();
    float distance = std::numeric_limits<float>::max();
    const Unit *target = nullptr;
    for (const auto &u : units) {
        float d = DistanceSquared2D(u->pos, start);
        if (d < distance && d > 0.1) {
            distance = d;
            target = u;
        }
    }
    return target;
}

const Unit* Hal9001::FindNearestSCV(const Point2D &start) {
    Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_SCV);
    float distance = std::numeric_limits<float>::max();
    const Unit *target = nullptr;
    for (const auto &u : units) {
        float d = DistanceSquared2D(u->pos, start);
        if (d < distance && d > 0.1) {
            distance = d;
            target = u;
        }
    }
    return target;
}

bool Hal9001::alreadyOrdered(ABILITY_ID ability_id){
    Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_SCV);
    for (const auto &unit : units){
        for (const auto &order : unit->orders){
            // don't need to build if another builder is building
            if (order.ability_id == ability_id){
                return true;
            }
        }
    }
    return false;
}

void Hal9001::BuildStructure(ABILITY_ID ability_type_for_structure, float x, float y, const Unit *builder) {
    // don't build if already being built
    if (alreadyOrdered(ability_type_for_structure)){
        return;
    }
    // if no builder is given, make the builder a random scv that's close to the build location
    if (!builder){
        Units units = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV, Point3D(x,y,0.0));
        // if none close make builder any random scv
        if (units.empty()){
            units = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV);
        }
        // if no scvs available don't build
        if (units.empty()){
            return;
        }
        builder = units.back();
    }

    Actions()->UnitCommand(builder, ability_type_for_structure, Point2D(x,y), true);   
}

bool Hal9001::TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
    const ObservationInterface* observation = Observation();

    const Unit* unit = nullptr;
    if (!GetRandomUnit(unit, observation, unit_type)) {
        return false;
    }
    if (!unit->orders.empty()) {
        return false;
    }

    if (unit->build_progress != 1) {
        return false;
    }

    Actions()->UnitCommand(unit, ability_type_for_unit);
    return true;
}

void Hal9001::BuildRefinery(const Unit *commcenter, const Unit *builder){
    // don't build if already being built
    if (alreadyOrdered(ABILITY_ID::BUILD_REFINERY)){
        return;
    }
    const Unit *geyser = FindNearestGeyser(commcenter->pos);   
    // if no builder is given, make the builder a random scv
    if (!builder){
        Units units = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV, geyser->pos);
        // can't find a builder
        if (units.empty()){
            return;
        }
        builder = units.back();
    }  
    
    Actions()->UnitCommand(builder, ABILITY_ID::BUILD_REFINERY, geyser);
}

void Hal9001::moveUnit(const Unit *unit, const Point2D &target){
    Actions()->UnitCommand(unit, ABILITY_ID::SMART, target);
}

bool Hal9001::GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type) {
    Units my_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
    if (!my_units.empty()) {
        unit_out = GetRandomEntry(my_units);
        return true;
    }
    return false;
}


size_t Hal9001::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

Units Hal9001::getDepots(){
    Units raised = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
    Units lowered = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED);
    raised.insert(raised.begin(), lowered.begin(), lowered.end());
    return raised;
}

Units Hal9001::getWidowMines(){
    Units raised = GetUnitsOfType(UNIT_TYPEID::TERRAN_WIDOWMINE);
    Units lowered = GetUnitsOfType(UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED);
    raised.insert(raised.begin(), lowered.begin(), lowered.end());
    return raised;
}

Units Hal9001::getCommCenters(){
    Units commcenters = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
    Units orbital_commcenters = GetUnitsOfType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND);
    commcenters.insert(commcenters.begin(), orbital_commcenters.begin(), orbital_commcenters.end());
    return commcenters;
}

Units Hal9001::GetUnitsOfType(UNIT_TYPEID unit_type){
    const ObservationInterface* observation = Observation();
    return observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
    
}

Units Hal9001::GetRandomUnits(UNIT_TYPEID unit_type, Point3D location, int num){
    // !!! maybe worry about if count < num after going thru all units list
    int count = 0;
    Units units = GetUnitsOfType(unit_type);
    Units bases = getCommCenters();
    Units to_return;
    bool in_range = true;
    for (const auto &u : units){
        if (count == num){
            break;
        }
        if (unit_type == UNIT_TYPEID::TERRAN_SCV){
            // don't return mainScv
            if (u == mainSCV){
                continue;
            }
            // don't return scout
            if (u == scout && !enemyBaseFound){
                continue;
            }
            // only choose from scvs that are mining minerals or idle
            for (const auto &base : bases){
                if (u->orders.empty() || u->orders.front().target_unit_tag == base->tag){
                    if (location != Point3D(0,0,0) && DistanceSquared2D(u->pos, location) > 900){
                        in_range = false;
                    }
                    if (in_range){
                        to_return.push_back(u);
                        ++count;
                    }
                }
            }
        // only choose from idle units
        } else if (u->orders.empty()){
            if (location != Point3D(0,0,0) && DistanceSquared2D(u->pos, location) > 900){
                in_range = false;
            }
            if (in_range){
                to_return.push_back(u);
                ++count;
            }
        }
        in_range = true;
    }
    return to_return;
}

bool Hal9001::doneConstruction(const Unit *unit){
    return unit->build_progress == 1.0;
}

// have 3 workers on each refinery
void Hal9001::ManageRefineries(){
    Units refineries = GetUnitsOfType(UNIT_TYPEID::TERRAN_REFINERY);
    for (const auto &refinery : refineries){
        if (refinery->build_progress != 1) {
            continue;
        }
        // refinery isn't full
        if (refinery->assigned_harvesters < refinery->ideal_harvesters){
            // get missing amount of scvs
            int num = refinery->ideal_harvesters - refinery->assigned_harvesters;
            Units scvs = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV, refinery->pos, num);
            if (scvs.empty()){
                return;
            }

            Actions()->UnitCommand(scvs, ABILITY_ID::HARVEST_GATHER, refinery);
        }
        //refinery is too full
        if (refinery->assigned_harvesters > refinery->ideal_harvesters){
            const Unit *scv = FindNearestSCV(refinery->pos);
            const Unit *mpatch = FindNearestMineralPatch(scv->pos);
            Actions()->UnitCommand(scv, ABILITY_ID::HARVEST_GATHER, mpatch);
        }
    }
}


/*
@desc 	This will return the radius of the structure to be built
*/
void Hal9001::buildNextTo(ABILITY_ID ability_id, const Unit* ref, RelDir relDir, int dist, const Unit *builder) {
    // don't build if already being built
    if (alreadyOrdered(ability_id)){
        return;
    }
    #ifdef DEBUG
    // cout << "placement query called" << endl;
    #endif
    // get radius of ref
    float radiusRE = ref -> radius;

    // get radius of to be built
    float radiusTB = radiusOfToBeBuilt(ability_id); 
    vector<QueryInterface::PlacementQuery> queries;
    // check placement in all directions
    for (int i = 0; i <= RelDir::BEHIND; ++i){
        RelDir rd = static_cast<RelDir>(i);
        std::pair<int, int> relCor = getRelativeDir(ref, rd);
        // config coordinates for building barracks
        float x = (ref -> pos.x) + (radiusRE + radiusTB + dist) * relCor.first;
        float y = (ref -> pos.y) + (radiusRE + radiusTB + dist) * relCor.second;
        queries.push_back(QueryInterface::PlacementQuery(ability_id, Point2D(x,y)));

    }
    vector<bool> placeable = Query()->Placement(queries);
    // try to place in given direction
    if (placeable[relDir]){
        float x = queries[relDir].target_pos.x;
        float y = queries[relDir].target_pos.y;
        // call BuildStructure
        BuildStructure(ability_id, x, y, builder); 
        return;
    }
    // place in a possible placement
    for (int i = 0; i < placeable.size(); ++i){
        if (placeable[i]){
            float x = queries[i].target_pos.x;
            float y = queries[i].target_pos.y;
            // call BuildStructure
            BuildStructure(ability_id, x, y, builder);
            break;
        }
    }

}

/*
@desc 	This will return the radius of the structure to be built
@param	abilityId - BUILD_SUPPLYDEPOT, etc
*/
float Hal9001::radiusOfToBeBuilt(ABILITY_ID abilityId){
    // TODO:: add assertion that only "build-type" abilities will be accepted
    // get observation
    const ObservationInterface *observation = Observation();

    // get vector of abilities
    const vector<AbilityData> abilities = observation ->GetAbilityData();

    // This will be the index of the searched ability
    int index;

    // loop through vector to search for ability
    for(int i = 0; i < abilities.size(); ++i){
        if(abilities[i].ability_id == abilityId){
            index = i;
        }
    }

    // cout << abilities[index].button_name << " " << abilities[index].is_building << endl;

    // return the footprint radius of ability
    return abilities[index].footprint_radius;
}


/*
@desc   This will return a Point towards the center from the building location
@param  buildingloc - location of reference building
        ratio - distance towards center eg. 2 is halfway to center
*/
Point2D Hal9001::PointTowardCenter(GameInfo game_info_, Point3D buildingloc, float ratio) {
    Point2D mapcenter = Point2D(game_info_.playable_max.x / 2, game_info_.playable_max.y / 2);
    float build_to_center_x = (buildingloc.x - mapcenter.x) / ratio;
    float build_to_center_y = (buildingloc.y - mapcenter.y) / ratio;
    #ifdef DEBUG
        cout << "Map center is " << mapcenter.x << ", " << mapcenter.y << endl;
        cout << "cc location is " << buildingloc.x << ", " << buildingloc.y << endl;
        cout << "PointToCenter returns: " << buildingloc.x - build_to_center_x << ", " << buildingloc.y - build_to_center_y << endl;
    #endif
    return Point2D(buildingloc.x - build_to_center_x, buildingloc.y - build_to_center_y);
}

/*
@desc   This will return a tuple (1,0), (1, -1), etc. 
@param  anchor - a unit, will build relative to this
        dir    - RelDir enum
@note   Refer to Orientation pngs in /docs
*/
std::pair<int, int> Hal9001::getRelativeDir(const Unit *anchor, const RelDir dir){
    // get which corner in the map base is
    Corner loc = cornerLoc(anchor);

    // anchor is located in the SW part of Map
    if( loc == SW ){
    
        switch(dir){
            case(FRONT):
                return std::make_pair(1, 1);
            case(LEFT):
                return std::make_pair(-1, 1);
            case(RIGHT):
                return std::make_pair(1, -1);
            case(FRONTLEFT):
                return std::make_pair(0, 1);
            case(FRONTRIGHT):
                return std::make_pair(1, 0);
            case(BEHIND):
                return std::make_pair(-1, -1);
            case(BEHINDLEFT):
                return std::make_pair(-1, 0);
            case(BEHINDRIGHT):
                return std::make_pair(0, -1);
            default:
                return std::make_pair(0,0);
        }
        
    // anchor is located in NW part of Map
    } else if( loc == NW ){

        switch(dir){
            case(FRONT):
                return std::make_pair(1, -1);
            case(LEFT):
                return std::make_pair(1, 1);
            case(RIGHT):
                return std::make_pair(-1, -1);
            case(FRONTLEFT):
                return std::make_pair(1, 0);
            case(FRONTRIGHT):
                return std::make_pair(0, -1);
            case(BEHIND):
                return std::make_pair(-1, 1);
            case(BEHINDLEFT):
                return std::make_pair(0, 1);
            case(BEHINDRIGHT):
                return std::make_pair(-1, 0);
            default:
                return std::make_pair(0,0);
        }
        
    // anchor is located in SE part of Map
    } else if( loc == SE ){

        switch(dir){
            case(FRONT):
                return std::make_pair(-1, 1);
            case(LEFT):
                return std::make_pair(-1, -1);
            case(RIGHT):
                return std::make_pair(1, 1);
            case(FRONTLEFT):
                return std::make_pair(-1, 0);
            case(FRONTRIGHT):
                return std::make_pair(0, 1);
            case(BEHIND):
                return std::make_pair(1, -1);
            case(BEHINDLEFT):
                return std::make_pair(0, -1);
            case(BEHINDRIGHT):
                return std::make_pair(1, 0);
            default:
                return std::make_pair(0,0);
        }

    // anchor is located in NE part of Map
    } else if( loc == NE ){

        switch(dir){
            case(FRONT):
                return std::make_pair(-1, -1);
            case(LEFT):
                return std::make_pair(1, -1);
            case(RIGHT):
                return std::make_pair(-1, 1);
            case(FRONTLEFT):
                return std::make_pair(0, -1);
            case(FRONTRIGHT):
                return std::make_pair(-1, 0);
            case(BEHIND):
                return std::make_pair(1, 1);
            case(BEHINDLEFT):
                return std::make_pair(1, 0);
            case(BEHINDRIGHT):
                return std::make_pair(0, 1);
            default:
                return std::make_pair(0,0);
        }
    }

    

    return std::make_pair(0, 0);
}


/*
@desc   This will return where another structure is relative to another structure anchor
         Layman: "Is target on the left or right of anchor"

@param  anchor - a unit, from this
        target - a unit, to this

@return LEFT or RIGHT (enum)
*/
RelDir Hal9001::getHandSide(const Unit* anchor, const Unit *target){
    
    // the displacements
    int x_dis = abs( (anchor -> pos.x) - (target -> pos.y) );
    int y_dis = abs( (anchor -> pos.y) - (target -> pos.y) );

    // diff of x_dis - y_dis, if > 0 then x_dis is greater
    int diff = x_dis - y_dis;


    Corner loc = cornerLoc(anchor);
    if(loc == SW || loc == NE){
        if(diff > 0){
            return RIGHT;
        } else{
            return LEFT;
        }

    } else if(loc == SE || loc == NW){
        if(diff > 0){
            return LEFT;
        } else{
            return RIGHT;
        }
    }
    
    return FRONT;
}

/*
@desc This will return where a structure is in the map
@param unit
@return Corner enum
*/
Corner Hal9001::cornerLoc(const Unit* unit){
    if( ((map_width / 2) > (unit -> pos.x)) && ((map_height /2) > (unit -> pos.y)) ){
        return SW;
    
    } else if( ((map_width / 2) > (unit -> pos.x)) && ((map_height /2) < (unit -> pos.y)) ){
        return NW;
    
    } else if( ((map_width / 2) < (unit -> pos.x)) && ((map_height /2) > (unit -> pos.y)) ) {
        return SE;

    } else if( ((map_width / 2) < (unit -> pos.x)) && ((map_height /2) < (unit -> pos.y))){
        return NE;
    }
    
    return M;
}

/*
@desc This will return true or false if a build ability is placeable in the given position
@param unit
@return bool
*/
bool Hal9001::isPlaceable(ABILITY_ID abilityId, Point2D points){
    // placement query provision
    vector<QueryInterface::PlacementQuery> queries;

    // slate to query
    queries.push_back(QueryInterface::PlacementQuery(abilityId, points));

    // get query bool
    vector<bool> placeble = Query()-> Placement(queries);

    return placeble[0];
}

/*
@desc This will return true or false if a order is already given to an scv and is in progress
@param abilityid
@return bool
*/
bool Hal9001::isOrdered(ABILITY_ID abilityId, UNIT_TYPEID unitTypeId){
    Units units = GetUnitsOfType(unitTypeId);

    for( const auto &u: units ){
        if(u -> orders.size()){
            
            for( const auto &o: u -> orders){
                if(o.ability_id == abilityId){
                    return true;
                }
            }
        }
    }
    return false;
}

/*
@desc This will land a build structure in the vicinity
@param unit for flying building, rel dir (try to land here first), ability id for landing the unit
@return void
*/
void Hal9001::landFlyer(const Unit* flyer, RelDir relDir, ABILITY_ID aid_to_land){
    // derived from mich's code

    // config coordinates
    float x = (flyer -> pos.x) + (flyer -> radius) * 2 + 2;
    float y = (flyer -> pos.y) + (flyer -> radius) * 2 + 2;

    cout << flyer -> radius << endl;

    vector<QueryInterface::PlacementQuery> queries;
    // check placement in all 8 directions
    for(int i = 0; i <= RelDir::BEHIND; ++i){
        RelDir rd = static_cast<RelDir>(i);
        std::pair<int, int> relCor = getRelativeDir(flyer, rd);
        x *= relCor.first; y *= relCor.second;
        queries.push_back(QueryInterface::PlacementQuery(aid_to_land, Point2D(x, y)));
    }

    vector<bool> landable = Query() -> Placement(queries);
    // try to land in given dir first
    if( landable[relDir]){
        Actions() -> UnitCommand(flyer, aid_to_land, queries[relDir].target_pos);
        return;
    }

    // or land in any possible placement
    for (int i = 0; i < landable.size(); ++i){
        if (landable[i]){
            Actions() -> UnitCommand(flyer, aid_to_land, queries[i].target_pos);
            break;
        }
    }
}
