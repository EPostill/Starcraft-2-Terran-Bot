#include "Hal9001.h"

void Hal9001::OnGameStart() { return; }

void Hal9001::OnStep() { 
    TryBuildSupplyDepot();
    TryBuildRefinery();
    TryBuildBarracks();

    switch (progress) {

    case 0:
        if (true) { //game time = 12s
            //move scv to supply depot build location
        }

        if (supplies >= 14 && Observation()->GetMinerals() >= 100) {
            //build supply depot towards center
            ++progress;
        }
        break;

    case 1:
        if (supplies >= 16 && Observation()->GetMinerals() >= 150) {
            //build barracks toward center
            ++progress;
        }
        break;

    case 2:
        if (supplies >= 16 && Observation()->GetMinerals() >= 75) {
            //build refinery on nearest gas
            ++progress;
        }
        break;

    case 3:
        //scout with scv
        if (/*we have 0 marines total (queue and active)*/true && Observation()->GetMinerals() >= 50) {
            //queue up marine
        }

        if (supplies >= 19 && Observation()->GetMinerals() >= 150) {
            //upgrade command center to orbital command
            ++progress;
        }
        break;

    case 4:
        if (supplies >= 20 && Observation()->GetMinerals() >= 400) {
            //build command center
            ++progress;
        }
        break;

    case 5:
        if (/*marine finishes building*/true && Observation()->GetMinerals() >= 150) {
            //build reactor on the barracks
            //build a depot next to the reactor
            ++progress;
        }
        break;

    case 6:
        if (supplies >= 22 && /*gas >= 100*/true) {
            //build factory in between command centers
            ++progress;
        }
        break;

    case 7:
        if (supplies >= 23 && Observation()->GetMinerals() >= 100) {
            //build bunker towards the center from command center 2
            ++progress;
        }
        break;

    case 8:
        if (/*we have less than 3 marines (in queue + out) and 1 reactor*/true && Observation()->GetMinerals() >= 50) {
            //queue a marine
            //move marines in front of bunker
        }
        if (supplies >= 26 && Observation()->GetMinerals() >= 75 && /*one empty gas next to first command center*/true) {
            //build second refinery next to first command center
            ++progress;
        }
        break;

    case 9:
        if (/*we own a factory*/true && Observation()->GetMinerals() >= 150 && /*gas >= 100*/true) {
            //build a star port next to the factory
        }
        if (/*we don't have a tech lab yet*/true && Observation()->GetMinerals() >= 50) {
            //build a tech lab connected to the factory
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
    case UNIT_TYPEID::TERRAN_SCV: {
        const Unit *mineral_target = FindNearestMineralPatch(unit->pos);
        if (!mineral_target) {
            break;
        }
        Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
        break;
    }
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

bool Hal9001::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type) {
    const ObservationInterface* observation = Observation();
    // If a unit already is building a supply structure of this type, do nothing.
    const Unit *unit_to_build = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self);
    for (const auto& unit : units) {
        for (const auto& order : unit->orders) {
            if (order.ability_id == ability_type_for_structure) {
                return false;
            }
        }
        // gets the svc that will build the structure
        if (unit->unit_type == unit_type) {
            unit_to_build = unit;
        }
    }

    float rx = GetRandomScalar();
    float ry = GetRandomScalar();

    Actions()->UnitCommand(unit_to_build,
        ability_type_for_structure,
        Point2D(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f));

    return true;    
}

bool Hal9001::TryBuildSupplyDepot(){
    const ObservationInterface* observation = Observation();

    // only build when supply capped
    if (observation->GetFoodUsed() <= observation->GetFoodCap() - 2){
        return false;
    }

    return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);

}

bool Hal9001::TryBuildBarracks(){

    // need to have a supply depot first
    if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1){
        return false;
    }

    // if we already have a barracks don't build
    if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) > 0){
        return false;
    }

    return TryBuildStructure(ABILITY_ID::BUILD_BARRACKS);    
}

bool Hal9001::TryBuildRefinery(){
    // build at most 2 refineries (can change this later)
    if (CountUnitType(UNIT_TYPEID::TERRAN_REFINERY) > 2){
        return false;
    }

    const ObservationInterface* observation = Observation();
    // If a unit already building a refinery, do nothing.
    const Unit *unit_to_build = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self);
    for (const auto& unit : units) {
        for (const auto& order : unit->orders) {
            if (order.ability_id == ABILITY_ID::BUILD_REFINERY) {
                return false;
            }
        }
        // gets the svc that will the refinery
        if (unit->unit_type == UNIT_TYPEID::TERRAN_SCV) {
            unit_to_build = unit;
        }
    }
    
    const Unit *geyser = FindNearestGeyser(unit_to_build->pos);
    // no geysers
    if (!geyser){
        return false;
    }
    Actions()->UnitCommand(unit_to_build, ABILITY_ID::BUILD_REFINERY, geyser);
    return true;
}

size_t Hal9001::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

void Hal9001::updateSupplies() {
    supplies = Observation()->GetFoodUsed();
}