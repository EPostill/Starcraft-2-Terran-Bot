#include "Hal9001.h"

#include <iostream>
using namespace std;

void Hal9001::OnGameStart() {
    const ObservationInterface *observation = Observation();
    minerals = observation->GetMinerals();
    supplies = observation->GetFoodUsed();
    // store expansions and start location
    expansions = search::CalculateExpansionLocations(observation, Query());
    startLocation = observation->GetStartLocation();
    rampLocation = startLocation;   // will change later
    mainSCV = nullptr;

}

//This function contains the steps we take at the start to establish ourselves
void Hal9001::BuildOrder(const ObservationInterface *observation) {
    
    // move scv towards supply depot build location
    if (!mainSCV && supplies == 13){
        // set main scv worker to the first one trained
        mainSCV = GetUnitsOfType(UNIT_TYPEID::TERRAN_SCV).front();  // test if this gets the newly trained scv
        const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front();
        Point3D exp = FindNearestExpansion();
        // sets rally point to nearest expansion so scv will walk towards it
        // !!! change to correct location 
        Actions()->UnitCommand(commcenter, ABILITY_ID::SMART, exp);
        
    }
    // set rally point back to minerals
    if (supplies >= 14){
       const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front();
       Actions()->UnitCommand(commcenter, ABILITY_ID::SMART, FindNearestMineralPatch(commcenter->pos)); 
    }

    //first supply depot build
    if (supplies >= 14 && minerals > 100 && CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) == 0) {
        // !!! change to correct location
        BuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, startLocation.x + 3, startLocation.y + 3);

    }

    //first barracks build
    if (supplies >= 16 && minerals >= 150 && CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) == 0) {

        //build barracks next to supply depot
        // !!! need to figure out how to place it correctly
        const Unit *depot = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT).front();
        BuildStructure(ABILITY_ID::BUILD_BARRACKS, depot->pos.x + 5, depot->pos.y);
    }

    //build a refinery on nearest gas
    if (supplies >= 16 && minerals >= 75 && CountUnitType(UNIT_TYPEID::TERRAN_REFINERY) == 0) {
        BuildRefinery();
    }

    Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKS);
    if (!units.empty()) {
        const Unit* barracks = units.front();
        if (doneConstruction(barracks) && CountUnitType(UNIT_TYPEID::TERRAN_MARINE) == 0 && barracks->orders.empty()) {
            // !!! scout with scv

            //train one marine
            Actions()->UnitCommand(barracks, ABILITY_ID::TRAIN_MARINE);
        }
    }

    if (supplies >= 19 && minerals >= 150 && CountUnitType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) == 0) {
        //upgrade command center to orbital command
        const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front();
        Actions()->UnitCommand(commcenter, ABILITY_ID::MORPH_ORBITALCOMMAND);
    }

    if (supplies >= 20 && Observation()->GetMinerals() >= 400 && CountUnitType(UNIT_TYPEID::TERRAN_COMMANDCENTER) == 0) {
        //build command center
        Expand();
    } 


    if (CountUnitType(UNIT_TYPEID::TERRAN_MARINE) == 1 && Observation()->GetMinerals() >= 150) {
        //build reactor on the barracks
        //build a depot next to the reactor
    }

    if (supplies >= 22 && Observation()->GetVespene() > 100 && Observation()->GetMinerals() > 150 && CountUnitType(UNIT_TYPEID::TERRAN_FACTORY) == 0) {
        //build factory in between command centers
    }

    if (supplies >= 23 && Observation()->GetMinerals() >= 100 && CountUnitType(UNIT_TYPEID::TERRAN_FACTORY) == 1 && CountUnitType(UNIT_TYPEID::TERRAN_BUNKER) == 0) {
        //build bunker towards the center from command center 2
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_REACTOR) == 1 && CountUnitType(UNIT_TYPEID::TERRAN_MARINE) < 4 && Observation()->GetMinerals() >= 50) {
        //queue a marine
        //move marines in front of bunker
    }

    if (supplies >= 26 && Observation()->GetMinerals() >= 75 && /*one empty gas next to first command center*/true) {
        //build second refinery next to first command center
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_FACTORY) == 1 && Observation()->GetMinerals() >= 150 && Observation()->GetVespene() > 100) {
        //build a star port next to the factory
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_TECHLAB) == 0 && Observation()->GetMinerals() >= 50) {
        //build a tech lab connected to the factory
    }
}


void Hal9001::ManageSCVTraining(){
    Units commcenters = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
    Units orbital_commcenters = GetUnitsOfType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND);

    for (const auto &cc : commcenters){
        if (cc->assigned_harvesters < cc->ideal_harvesters && cc->orders.empty()){
            Actions()->UnitCommand(cc, ABILITY_ID::TRAIN_SCV);
        }
    }

    for (const auto &cc : orbital_commcenters){
        if (cc->assigned_harvesters < cc->ideal_harvesters && cc->orders.empty()){
            Actions()->UnitCommand(cc, ABILITY_ID::TRAIN_SCV);
        }
    }
}

void Hal9001::OnStep() { 
    // cout << Observation()->GetGameLoop() << endl;
    const ObservationInterface *observation = Observation();
    minerals = observation->GetMinerals();
    supplies = observation->GetFoodUsed();
    ManageSCVTraining();

    BuildOrder(observation);

}

void Hal9001::OnUnitIdle(const Unit *unit) {

}

void Hal9001::Expand(){
    Point3D exp = FindNearestExpansion();
    // build command centre
    // !!! maybe use mainscv as builder?
    BuildStructure(ABILITY_ID::BUILD_COMMANDCENTER, exp.x, exp.y);
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
    for (const auto &u : units) {
        // get closest vespene geyser
        if (u->unit_type == UNIT_TYPEID::NEUTRAL_VESPENEGEYSER) {
            float d = DistanceSquared2D(u->pos, start);
            if (d < distance) {
                distance = d;
                target = u;
            }
        }
    }
    return target;
}

void Hal9001::BuildStructure(ABILITY_ID ability_type_for_structure, float x, float y, const Unit *builder) {
    // if no builder is given, make the builder a random scv
    if (!builder){
        Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_SCV);
        builder = units.back();
    }

    Actions()->UnitCommand(builder, ability_type_for_structure, Point2D(x,y));   
}


void Hal9001::BuildRefinery(const Unit *builder){
    // if no builder is given, make the builder a random scv
    if (!builder){
        Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_SCV);
        builder = units.back();
    }
    
    const Unit *geyser = FindNearestGeyser(builder->pos);
    Actions()->UnitCommand(builder, ABILITY_ID::BUILD_REFINERY, geyser);
}

void Hal9001::moveUnit(const Unit *unit, const Point2D &target){
    Actions()->UnitCommand(unit, ABILITY_ID::SMART, target);
}


size_t Hal9001::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}


Units Hal9001::GetUnitsOfType(UNIT_TYPEID unit_type){
    const ObservationInterface* observation = Observation();
    return observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
}

bool Hal9001::doneConstruction(const Unit *unit){
    return unit->build_progress == 1.0;
}

void Hal9001::step14(){
    // use extra resources for marines (but prioritize buildings)

}

// once factory upgrades finish, build widow mine for air unit defence
void Hal9001::step15(){
    Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_FACTORY);
    const Unit *factory = units.front();
    // tell factory to build widow mine
    Actions()->UnitCommand(factory, ABILITY_ID::TRAIN_WIDOWMINE);

}

// 36-38 supply - build depot behind minerals at the 2nd command center
void Hal9001::step16(){
    Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
    const Unit *commcenter = units.back();    // check if this gets the 2nd command center
    // get a mineral that is near the second command center
    const Unit *mineral = FindNearestMineralPatch(commcenter->pos);
    // build depot behind this mineral
    // figure out how to put it behind the depot

}