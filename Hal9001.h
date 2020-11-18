#ifndef HAL_9001_H_
#define HAL_9001_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2lib/sc2_lib.h"

#include "sc2api/sc2_unit_filters.h"
#include "sc2lib/sc2_search.h"

using namespace sc2;

class Hal9001 : public sc2::Agent {
public:
	virtual void OnGameStart();
	virtual void OnStep();

	virtual void OnUnitIdle(const Unit* unit);
	// returns mineral patch nearest to start
	const Unit* FindNearestMineralPatch(const Point2D &start);
	// returns geyser nearest to start
	const Unit* FindNearestGeyser(const Point2D &start);
	// returns free expansion location nearest to main base
	const Point3D FindNearestExpansion();
	void BuildStructure(ABILITY_ID ability_type_for_structure, float x, float y, const Unit *builder = nullptr);
	void BuildOrder();
	void BuildRefinery(const Unit *builder = nullptr);
	void updateSupplies();
	// policy for training scvs
	void ManageSCVTraining();
	// expands to free expansion that is closest to main base
	void Expand();
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

    std::vector<Point3D> expansions;
    Point3D startLocation;
};

#endif