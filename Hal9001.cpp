#include "Hal9001.h"

void Hal9001::OnGameStart() { return; }

void Hal9001::OnStep() { 
    TryBuildSupplyDepot();
    TryBuildRefinery();
    TryBuildBarracks();

    switch (progress) {

    case (0): {

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

bool Hal9001::TryBuildStructure(ABILITY_ID ability_type_for_structure, float x, float y, const Unit *builder) {
    // if no builder is given, make the builder a random scv
    if (!builder){
        const ObservationInterface* observation = Observation();
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


Units Hal9001::GetUnitsOfType(UNIT_TYPEID unit_type){
    const ObservationInterface* observation = Observation();
    return observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
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