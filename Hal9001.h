#ifndef HAL_9001_H_
#define HAL_9001_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"
#include <sc2api/sc2_unit_filters.h>

using namespace sc2;

class Hal9001 : public sc2::Agent {
public:
	virtual void OnGameStart();
	virtual void OnStep();

	virtual void OnUnitIdle(const Unit* unit);
	
	const Unit* FindNearestMineralPatch(const Point2D &start);
	const Unit* FindNearestGeyser(const Point2D &start);
	void BuildStructure(ABILITY_ID ability_type_for_structure, float x, float y, const Unit *builder = nullptr);
	void BuildOrder();
	void BuildRefinery(const Unit *builder = nullptr);
	void updateSupplies();
	//builds a building adjacent to reference
	void BuildNextTo(ABILITY_ID ability_type_for_structure, UNIT_TYPEID new_building, const Unit* reference, const Unit* builder);
	// moves unit to target position
	void moveUnit(const Unit *unit, const Point2D &target);
	// finds the position of main ramp leading to the given command center
	const Point2D findMainRamp(const Unit *commcenter);

	void step14();
	void step15();
	void step16();

private:
	size_t CountUnitType(UNIT_TYPEID unit_type);
	Units GetUnitsOfType(UNIT_TYPEID unit_type);
	// returns true if unit has finished being built
	bool doneConstruction(const Unit *unit);
	//progression counter to mark which stage of progress we are at
	int progress;
	int supplies;
};

#endif