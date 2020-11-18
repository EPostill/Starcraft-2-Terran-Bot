#include "Hal9001.h"

#include <iostream>
#include "cpp-sc2/src/sc2api/sc2_client.cc"
using namespace std;

void Hal9001::OnGameStart() {
    progress = 0;
    supplies = 14;
}

//This function contains the steps we take at the start to establish ourselves
void Hal9001::BuildOrder() {

    const Unit* commcenter = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER).front();
    
    if (Observation()->GetGameLoop() > 192) {//game time 12s (16 ticks * 12s)\
        //move scv to supply depot build location
    }

    //first supply depot build
    if (supplies >= 14 && Observation()->GetMinerals() > 100 && CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) == 0) {
        BuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, commcenter->pos.x + 5, commcenter->pos.y);
    }

    //first barracks build
    if (supplies >= 16 && Observation()->GetMinerals() >= 150 && CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) == 0) {
        //build barracks next to supply depot
        Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
        const Unit* depot = units.front();
        // need to figure out how to place it nicely
        BuildStructure(ABILITY_ID::BUILD_BARRACKS, depot->pos.x + 5, depot->pos.y);
    }

    //build refinery
    if (supplies >= 16 && Observation()->GetMinerals() >= 75 && CountUnitType(UNIT_TYPEID::TERRAN_REFINERY) == 0) {
        //build 1 refinery on nearest gas
        if (CountUnitType(UNIT_TYPEID::TERRAN_REFINERY) < 1) {
            BuildRefinery();
        }
    }

    Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKS);
    if (!units.empty()) {
        const Unit* barracks = units.front();
        if (doneConstruction(barracks) && CountUnitType(UNIT_TYPEID::TERRAN_MARINE) == 0) {
            //scout with scv
            //build marine
        }
    }

    if (supplies >= 19 && Observation()->GetMinerals() >= 150 && CountUnitType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) == 0) {
        //upgrade command center to orbital command
        Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
        const Unit* commcenter = units.front();

        Actions()->UnitCommand(commcenter, ABILITY_ID::MORPH_ORBITALCOMMAND);
    }

    if (supplies >= 20 && Observation()->GetMinerals() >= 400 && CountUnitType(UNIT_TYPEID::TERRAN_COMMANDCENTER) == 0) {
        //build command center
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

    if (CountUnitType(UNIT_TYPEID::TERRAN_REACTOR) == 1 && CountUnitType(UNIT_TYPEID::TERRAN_WIDOWMINE) == 0 && Observation()->GetMinerals() >= 75) {
        //build widow mine for defence
    }

    if (supplies > 36 && Observation()->GetMinerals() >= 100 && CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 2) {
        //build supply depot behind the minerals next to the 2nd comm center
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_STARPORT) == 1 && CountUnitType(UNIT_TYPEID::TERRAN_VIKINGASSAULT) == 0) {
        //build a viking
    }

    if (CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) == 0 && Observation()->GetMinerals() >= 150 && Observation()->GetVespene() >= 125) {
        //build a tank
    }

    //this one is tricky, we basically want to chain depots next to each other behind the second comm center
    Units depots = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
    const Unit* current_depot = depots.front();
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
    const Unit* engbay = depots.front();
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







}

void Hal9001::OnStep() { 
    cout << "progress: " << progress << endl;
    updateSupplies();
    cout << "supplies: " << supplies << endl;

    BuildOrder();

}

void Hal9001::OnUnitIdle(const Unit *unit) {
    switch (unit->unit_type.ToType()) {
    // tells command center to build SCVs
    // case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
    //     if (supplies < 19){
    //         Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
    //         break;
    //     }
    // }
    // tells SCVs to mine minerals
    // case UNIT_TYPEID::TERRAN_SCV: {
    //     const Unit *mineral_target = FindNearestMineralPatch(unit->pos);
    //     if (!mineral_target) {
    //         break;
    //     }
    //     Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
    //     break;
    // }
    // tells barracks to train marines
    case UNIT_TYPEID::TERRAN_BARRACKS: {
        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
        break;
    }
    default: {
        break;
    }
    }
}

// returns nearest mineral patch or nullptr if none found
const Unit* Hal9001::FindNearestMineralPatch(const Point2D &start) {
    // gets all neutral units
    Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
    float distance = std::numeric_limits<float>::max();
    const Unit *target = nullptr;
    for (const auto& u : units) {
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
    for (const auto& u : units) {
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

// const Point2D Hal9001::findMainRamp(const Unit *commcenter){
//     float baseHeight = Observation()->TerrainHeight(commcenter->pos);
    
// }

size_t Hal9001::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

void Hal9001::updateSupplies() {
    supplies = Observation()->GetFoodUsed();
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