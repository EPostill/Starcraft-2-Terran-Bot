#include "Hal9001.h"

#include <iostream>
#include "cpp-sc2/src/sc2api/sc2_client.cc"
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

    if (build_complete) {
        return;
    }

    //Counts to keep track of all of our buildings (so we don't need to recount at every if statement
    Units bases = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
    Units barracks = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKS);
    Units factories = GetUnitsOfType(UNIT_TYPEID::TERRAN_FACTORY);
    Units starports = GetUnitsOfType(UNIT_TYPEID::TERRAN_STARPORT);
    Units depots = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
    
    // tell the first scv in training to move towards supply depot build location
    if (!mainSCV && supplies == 13){
        const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front();
        Point3D exp = FindNearestExpansion();
        // sets rally point to nearest expansion so scv will walk towards it
        // !!! change to correct location 
        Actions()->UnitCommand(commcenter, ABILITY_ID::SMART, exp);
        
    }
    // set mainSCV to the first trained scv
    if (!mainSCV && supplies == 14){
        Units scvs = GetUnitsOfType(UNIT_TYPEID::TERRAN_SCV);
        for (const auto &scv: scvs){
            const UnitOrder order = scv->orders.front();
            // this is the first trained scv
            if (order.target_pos.x != 0 && order.target_pos.y != 0){
                mainSCV = scv;
            }
        }
        // set rally point back to minerals
        const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front();
        Actions()->UnitCommand(commcenter, ABILITY_ID::SMART, FindNearestMineralPatch(commcenter->pos)); 
    }

    //first supply depot build
    if (supplies >= 14 && minerals > 100 && CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) == 0) {
        // !!! change to correct location
        BuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, startLocation.x + 3, startLocation.y + 3, mainSCV);

    }

    //first barracks build
    if (supplies >= 16 && minerals >= 150 && CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) == 0) {

        //build barracks next to supply depot
        // !!! need to figure out how to place it correctly
        const Unit *depot = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT).front();
        BuildStructure(ABILITY_ID::BUILD_BARRACKS, depot->pos.x + 5, depot->pos.y, mainSCV);
    }

    //build a refinery on nearest gas
    if (supplies >= 16 && minerals >= 75 && CountUnitType(UNIT_TYPEID::TERRAN_REFINERY) == 0) {
        BuildRefinery(GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front());
    }

    Units barracks_list = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKS);
    if (!barracks_list.empty()) {
        const Unit* barracks = barracks_list.front();
        if (doneConstruction(barracks) && CountUnitType(UNIT_TYPEID::TERRAN_MARINE) == 0 && barracks->orders.empty()) {
            // !!! scout with scv

            //train one marine
            Actions()->UnitCommand(barracks, ABILITY_ID::TRAIN_MARINE);
        }
    }

    if (supplies >= 19 && minerals >= 150 && CountUnitType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) == 0) {
        //upgrade command center to orbital command
        const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front();
        Actions()->UnitCommand(commcenter, ABILITY_ID::MORPH_ORBITALCOMMAND, true);
    }

    if (supplies >= 20 && Observation()->GetMinerals() >= 400 && CountUnitType(UNIT_TYPEID::TERRAN_COMMANDCENTER) == 0) {
        //build command center
        Expand();
    } 
    // // set rally point of new command center to minerals
    // if (CountUnitType(UNIT_TYPEID::TERRAN_COMMANDCENTER) == 1){
    //     const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front();
    //     if (commcenter->build_progress == 0.5){
    //         Actions()->UnitCommand(commcenter, ABILITY_ID::SMART, FindNearestMineralPatch(commcenter->pos), true);
    //     }
    // }


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

    if (supplies >= 26 && Observation()->GetMinerals() >= 75) {
        //build second refinery next to first command center
        const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND).front();
        BuildRefinery(commcenter);  // !!! doesn't seem to be working
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_FACTORY) == 1 && CountUnitType(UNIT_TYPEID::TERRAN_STARPORT) == 0 && Observation()->GetMinerals() >= 150 && Observation()->GetVespene() > 100) {
        //build a star port next to the factory
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_TECHLAB) == 0 && CountUnitType(UNIT_TYPEID::TERRAN_STARPORT) == 1 && Observation()->GetMinerals() >= 50) {
        //build a tech lab connected to the factory
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_REACTOR) == 1 && CountUnitType(UNIT_TYPEID::TERRAN_WIDOWMINE) == 0 && Observation()->GetMinerals() >= 75) {
        //build widow mine for defence
    }

    if (supplies > 36 && Observation()->GetMinerals() >= 100 && CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 2) {
        //build supply depot behind the minerals next to the 2nd comm center
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_STARPORT) == 1 && CountUnitType(UNIT_TYPEID::TERRAN_VIKINGASSAULT) == 0) {
        //build a viking
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) == 0 && CountUnitType(UNIT_TYPEID::TERRAN_VIKINGASSAULT) == 1 && Observation()->GetMinerals() >= 150 && Observation()->GetVespene() >= 125) {
        //build a tank
    }

    //this one is tricky, we basically want to chain depots next to each other behind the second comm center
    const Unit* current_depot;
    if (depots.size() != 0) {
        current_depot = depots.front();
    }
    if (doneConstruction(current_depot) && Observation()->GetMinerals() >= 100) {
        //build another depot behind the current depot
    }

    if (supplies >= 46 && Observation()->GetMinerals() >= 300) {
        //build 2 more barracks next to the star port and factory
    }

    //this is another tricky one, when the tank is FINISHED we want to move the factory on to a tech lab and the star port on to a reactor
    if (CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) == 1) {
        //move buildings
    }

    if (Observation()->GetGameLoop() > 4320 && Observation()->GetMinerals() >= 200 && CountUnitType(UNIT_TYPEID::TERRAN_ENGINEERINGBAY) == 0) { //4320 ticks ~ 4mins30sec
        //build engineering bay and a gas refinery at the 2nd comm center
    }

    //something about checking building positions here
    if (/*factory and star port have been moved*/true) {
        //move the 2 newest barracks to the tech labs that are now open
    }

    Units engbays = GetUnitsOfType(UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
    const Unit* engbay;
    if (engbays.size() != 0) {
        engbay = engbays.front();
    }
    if (doneConstruction(engbay)) {
        //upgrade infantry weapons to level 2 and research stim in the tech labs
    }

    if (/*mineral line is fully saturated*/true && Observation()->GetMinerals() >= 75 && CountUnitType(UNIT_TYPEID::TERRAN_REFINERY) < 3) {
        //build second refinery for the gas
    }

    //At this point we have a few goals before we attack
    // we want to:
    //research combat shields
    //build another command center
    //research some amount of techs

    //more notes:
    //we should only build medivacs once combat shields has been researched
    //Star ports -> medivacs
    //Factories -> tanks
    //barracks -> marines (later on maurauders, although I doubt the game will go that far)

    if (false /*every condition is met*/) {
        build_complete = true;
    }
}


void Hal9001::ManageSCVTraining(){
    Units commcenters = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
    Units orbital_commcenters = GetUnitsOfType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND);

    for (const auto &cc : commcenters){
        if (cc->assigned_harvesters < cc->ideal_harvesters && cc->orders.empty()){
            Actions()->UnitCommand(cc, ABILITY_ID::TRAIN_SCV);
        }
        // set rally point of new command centers to minerals
        if (cc->build_progress == 0.5){
            Actions()->UnitCommand(cc, ABILITY_ID::SMART, FindNearestMineralPatch(cc->pos), true);
        }
    }

    for (const auto &cc : orbital_commcenters){
        if (cc->assigned_harvesters < cc->ideal_harvesters && cc->orders.empty()){
            Actions()->UnitCommand(cc, ABILITY_ID::TRAIN_SCV);
        }
        // set rally point of new command centers to minerals
        if (cc->build_progress == 0.5){
            Actions()->UnitCommand(cc, ABILITY_ID::SMART, FindNearestMineralPatch(cc->pos), true);
        }
    }
}

// have 3 workers on each refinery
void Hal9001::ManageRefineries(){
    Units refineries = GetUnitsOfType(UNIT_TYPEID::TERRAN_REFINERY);
    for (const auto &refinery : refineries){
        // almost done building
        if (refinery->build_progress == 0.75){
            // get two random scvs
            Units scvs = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV, refinery->pos, 2);

            Actions()->UnitCommand(scvs, ABILITY_ID::SMART, refinery, true);
        }
    }
}

void Hal9001::OnStep() { 
    const ObservationInterface *observation = Observation();
    minerals = observation->GetMinerals();
    supplies = observation->GetFoodUsed();
    ManageSCVTraining();
    ManageRefineries();

    BuildOrder(observation);


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
        builder = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV).back();
    }

    Actions()->UnitCommand(builder, ability_type_for_structure, Point2D(x,y));   
}

void Hal9001::BuildNextTo(ABILITY_ID ability_type_for_structure, UNIT_TYPEID new_building, const Unit* reference, const Unit* builder) {
    //find a way to add the radius of the new building to this
    float offset = reference->radius;
    float reference_x = reference->pos.x;
    float reference_y = reference->pos.y;
    

    BuildStructure(ability_type_for_structure, reference_x + offset, reference_y, builder);
    if (true) { //if build order is successful
        return;
    }
    BuildStructure(ability_type_for_structure, reference_x - offset, reference_y, builder);
    if (true) { //if build order is successful
        return;
    }
    BuildStructure(ability_type_for_structure, reference_x, reference_y + offset, builder);
    if (true) { //if build order is successful
        return;
    }
    BuildStructure(ability_type_for_structure, reference_x, reference_y - offset, builder);
    if (true) { //if build order is successful
        return;
    }

}


void Hal9001::BuildRefinery(const Unit *commcenter, const Unit *builder){
    const Unit *geyser = FindNearestGeyser(commcenter->pos);
    // if no builder is given, make the builder a random scv
    if (!builder){
        builder = GetRandomUnits(UNIT_TYPEID::TERRAN_SCV, geyser->pos).back();
    }  
    
    Actions()->UnitCommand(builder, ABILITY_ID::BUILD_REFINERY, geyser, true);
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

Units Hal9001::GetRandomUnits(UNIT_TYPEID unit_type, Point3D location, int num){
    // !!! maybe worry about if count < num after going thru all units list
    int count = 0;
    Units units = GetUnitsOfType(unit_type);
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
            // only choose from scvs that are mining minerals or idle
            AbilityID aid = u->orders.front().ability_id;
            if (u->orders.empty() || aid == ABILITY_ID::HARVEST_GATHER || aid == ABILITY_ID::HARVEST_RETURN){
                if (location != Point3D(0,0,0) && DistanceSquared2D(u->pos, location) > 225.0){
                    in_range = false;
                }
                if (in_range){
                    to_return.push_back(u);
                    ++count;
                }
            }
        // only choose from idle units
        } else if (u->orders.empty()){
            if (location != Point3D(0,0,0) && DistanceSquared3D(u->pos, location) > 225.0){
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
    if (unit == nullptr) {
        return false;
    }
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