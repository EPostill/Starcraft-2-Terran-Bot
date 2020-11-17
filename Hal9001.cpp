#include "Hal9001.h"

#include <iostream>
using namespace std;

void Hal9001::OnGameStart() {
    progress = 0;
    supplies = 14;
}

void Hal9001::OnStep() { 
    cout << "progress: " << progress << endl;
    updateSupplies();
    cout << "supplies: " << supplies << endl;
    switch (progress) {

    case 0:
        if (true) { //game time = 12s
            //move scv to supply depot build location
            
        }

        if (supplies >= 14 && Observation()->GetMinerals() >= 100) {
            //build supply depot towards center
            Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
            const Unit *commcenter = units.front();
            // need to find main ramp location
            BuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, commcenter->pos.x + 5, commcenter->pos.y + 8);
            ++progress;
        }
        break;

    case 1:
        if (supplies >= 16 && Observation()->GetMinerals() >= 150) {
            //build barracks next to supply depot
            Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
            const Unit *depot = units.front();
            // need to figure out how to place it nicely
            BuildStructure(ABILITY_ID::BUILD_BARRACKS, depot->pos.x - 3, depot->pos.y);
            ++progress;
        }
        break;

    case 2:
        if (supplies >= 16 && Observation()->GetMinerals() >= 75) {
            //build refinery on nearest gas
            BuildRefinery();
            // move to next stage when barracks is done
            Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_BARRACKS);
            if (!units.empty()){
                const Unit *barracks = units.front();
                if (doneConstruction(barracks)){
                    ++progress;
                }
            }
        }
        break;

    case 3:
        //scout with scv
        if (/*we have 0 marines total (queue and active)*/true && Observation()->GetMinerals() >= 50) {
            //queue up marine
        }

        if (supplies >= 19 && Observation()->GetMinerals() >= 150) {
            //upgrade command center to orbital command
            Units units = GetUnitsOfType(UNIT_TYPEID::TERRAN_COMMANDCENTER);
            const Unit *commcenter = units.front();    // check if this gets the 1st command center
            
            Actions()->UnitCommand(commcenter, ABILITY_ID::MORPH_ORBITALCOMMAND);
            
            ++progress;
        }
        break;

    // case 4:
    //     if (supplies >= 20 && Observation()->GetMinerals() >= 400) {
    //         //build command center
    //         ++progress;
    //     }
    //     break;

    // case 5:
    //     if (/*marine finishes building*/true && Observation()->GetMinerals() >= 150) {
    //         //build reactor on the barracks
    //         //build a depot next to the reactor
    //         ++progress;
    //     }
    //     break;

    // case 6:
    //     if (supplies >= 22 && /*gas >= 100*/true) {
    //         //build factory in between command centers
    //         ++progress;
    //     }
    //     break;

    // case 7:
    //     if (supplies >= 23 && Observation()->GetMinerals() >= 100) {
    //         //build bunker towards the center from command center 2
    //         ++progress;
    //     }
    //     break;

    // case 8:
    //     if (/*we have less than 3 marines (in queue + out) and 1 reactor*/true && Observation()->GetMinerals() >= 50) {
    //         //queue a marine
    //         //move marines in front of bunker
    //     }
    //     if (supplies >= 26 && Observation()->GetMinerals() >= 75 && /*one empty gas next to first command center*/true) {
    //         //build second refinery next to first command center
    //         ++progress;
    //     }
    //     break;

    // case 9:
    //     if (/*we own a factory*/true && Observation()->GetMinerals() >= 150 && /*gas >= 100*/true) {
    //         //build a star port next to the factory
    //     }
    //     if (/*we don't have a tech lab yet*/true && Observation()->GetMinerals() >= 50) {
    //         //build a tech lab connected to the factory
    //     }
    default: {
        break;
    }
    }
}

void Hal9001::OnUnitIdle(const Unit *unit) {
    switch (unit->unit_type.ToType()) {
    // tells command center to build SCVs
    case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
        break;
    }
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
    supplies = CountUnitType(UNIT_TYPEID::TERRAN_SCV);
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