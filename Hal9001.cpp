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

    //lists to keep track of all of our buildings (so we don't need to recount at every if statement
    Units bases = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
    Units barracks = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKS);
    Units factories = GetUnitsOfType(UNIT_TYPEID::TERRAN_FACTORY);
    Units starports = GetUnitsOfType(UNIT_TYPEID::TERRAN_STARPORT);
    Units techlabs = GetUnitsOfType(UNIT_TYPEID::TERRAN_TECHLAB);
    Units reactors = GetUnitsOfType(UNIT_TYPEID::TERRAN_REACTOR);
    Units depots = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
    Units refineries = GetUnitsOfType(UNIT_TYPEID::TERRAN_REFINERY);
    Units engbays = GetUnitsOfType(UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
    Units orbcoms = GetUnitsOfType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND);
    Units bunkers = GetUnitsOfType(UNIT_TYPEID::TERRAN_BUNKER);

    //lists to keep track of all our units
    Units marines = GetUnitsOfType(UNIT_TYPEID::TERRAN_MARINE);
    Units tanks = GetUnitsOfType(UNIT_TYPEID::TERRAN_SIEGETANK);
    Units widowmines = GetUnitsOfType(UNIT_TYPEID::TERRAN_WIDOWMINE);
    Units vikings = GetUnitsOfType(UNIT_TYPEID::TERRAN_VIKINGASSAULT);
    
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
    if (supplies >= 14 && minerals > 100 && depots.size() == 0) {
        // !!! change to correct location
        BuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, startLocation.x + 3, startLocation.y + 3, mainSCV);

    }

    //first barracks build
    if (supplies >= 16 && minerals >= 150 && barracks.size() == 0) {

        //build barracks next to supply depot
        // !!! need to figure out how to place it correctly
        const Unit *depot = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT).front();
        BuildStructure(ABILITY_ID::BUILD_BARRACKS, depot->pos.x + 5, depot->pos.y, mainSCV);
    }

    //build a refinery on nearest gas
    if (supplies >= 16 && minerals >= 75 && refineries.size() == 0) {
        BuildRefinery(GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front());
    }

    //first marine
    if (!barracks.empty()) {
        const Unit* barrack = barracks.front();
        if (doneConstruction(barrack) && marines.size() == 0 && barrack->orders.empty()) {
            // !!! scout with scv

            //train one marine
            Actions()->UnitCommand(barrack, ABILITY_ID::TRAIN_MARINE);
        }
    }

    //upgrade command center
    if (supplies >= 19 && minerals >= 150 && orbcoms.size() == 0) {
        //upgrade command center to orbital command
        const Unit* commcenter = bases.front();
        Actions()->UnitCommand(commcenter, ABILITY_ID::MORPH_ORBITALCOMMAND, true);
    }

    //build second command center
    if (supplies >= 20 && Observation()->GetMinerals() >= 400 && bases.size() < 2) {
        //build command center
        Expand();
    } 
    // // set rally point of new command center to minerals
    // if (bases.size() == 1){
    //     const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front();
    //     if (commcenter->build_progress == 0.5){
    //         Actions()->UnitCommand(commcenter, ABILITY_ID::SMART, FindNearestMineralPatch(commcenter->pos), true);
    //     }
    // }

    //build reactor and another depot
    if (marines.size() == 1 && depots.size() < 2 && Observation()->GetMinerals() >= 150) {
        //build reactor on the barracks
        //build a depot next to the reactor
    }

    //build first factory
    if (supplies >= 22 && Observation()->GetVespene() > 100 && Observation()->GetMinerals() > 150 && factories.size() == 0) {
        //build factory in between command centers
    }

    //build first bunker
    if (supplies >= 23 && Observation()->GetMinerals() >= 100 && factories.size() == 1 && bunkers.size() == 0) {
        //build bunker towards the center from command center 2
    }

    //build army to size 3 and station at bunker
    if (reactors.size() == 1 && marines.size() < 4 && Observation()->GetMinerals() >= 50) {
        //queue a marine
        //move marines in front of bunker
    }

    //build our second refinery
    if (supplies >= 26 && refineries.size() < 2 && Observation()->GetMinerals() >= 75) {
        //build second refinery next to first command center
        const Unit* commcenter = orbcoms.front();
        BuildRefinery(commcenter);  // !!! doesn't seem to be working
    }

    //build our first star port
    if (factories.size() == 1 && starports.size() == 0 && Observation()->GetMinerals() >= 150 && Observation()->GetVespene() > 100) {
        //build a star port next to the factory
    }

    //build our first techlab
    if (techlabs.size() == 0 && starports.size() == 1 && Observation()->GetMinerals() >= 50) {
        //build a tech lab connected to the factory
    }

    //build a widow mine for air defence
    if (reactors.size() == 1 && widowmines.size() == 0 && Observation()->GetMinerals() >= 75) {
        //build widow mine for defence
    }

    //build our 2nd com center's supply depot
    if (supplies > 36 && Observation()->GetMinerals() >= 100 && depots.size() < 2) {
        //build supply depot behind the minerals next to the 2nd comm center
    }

    //build our first air unit
    if (starports.size() == 1 && vikings.size() == 0) {
        //build a viking
    }

    //build our first tank
    if (tanks.size() == 0 && vikings.size() == 1 && Observation()->GetMinerals() >= 150 && Observation()->GetVespene() >= 125) {
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

    //build 2 barracks to later move
    if (supplies >= 46 && Observation()->GetMinerals() >= 300) {
        //build 2 more barracks next to the star port and factory
    }

    //this is another tricky one, when the tank is FINISHED we want to move the factory on to a tech lab and the star port on to a reactor
    if (tanks.size() == 1) {
        //move buildings
    }

    //build our first eng bay
    if (Observation()->GetGameLoop() > 4320 && Observation()->GetMinerals() >= 200 && engbays.size() == 0) { //4320 ticks ~ 4mins30sec
        //build engineering bay and a gas refinery at the 2nd comm center
    }

    //once the space from the factory and star port are open, move our new barracks there
    if (/*factory and star port have been moved*/true) {
        //move the 2 newest barracks to the tech labs that are now open
    }

    const Unit* engbay;
    if (engbays.size() != 0) {
        engbay = engbays.front();
    }
    if (doneConstruction(engbay)) {
        //upgrade infantry weapons to level 2 and research stim in the tech labs
    }

    //fully build out our second base
    if (/*mineral line is fully saturated*/true && Observation()->GetMinerals() >= 75 && refineries.size() < 3) {
        //build second refinery for the gas
        const Unit *commandcenter = bases.front();
        BuildRefinery(commandcenter, mainSCV);
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