#ifndef HAL_9001_H_
#define HAL_9001_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2lib/sc2_lib.h"

#include "sc2api/sc2_unit_filters.h"
#include "sc2lib/sc2_search.h"
#include "sc2api/sc2_data.h"

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
	void BuildOrder(const ObservationInterface *observation);
	// build a refinery near the given command center
	void BuildRefinery(const Unit *commcenter, const Unit *builder = nullptr);
	void updateSupplies();
	//builds a building adjacent to reference
	void BuildNextTo(ABILITY_ID ability_type_for_structure, UNIT_TYPEID new_building, const Unit* reference, const Unit* builder);
	// policy for training scvs
	void ManageSCVTraining();
	// policy for gathering vespene gas
	void ManageRefineries();
	// expands to free expansion that is closest to main base
	void Expand();
	// moves unit to target position
	void moveUnit(const Unit *unit, const Point2D &target);

	void step14();
	void step15();
	void step16();

private:
	// counts the number of units of a given type (does not include those in training)
	size_t CountUnitType(UNIT_TYPEID unit_type);
	// returns a vec of units of given type (does not include those in training)
	Units GetUnitsOfType(UNIT_TYPEID unit_type);
	// returns a vec of a certain number of units of the given type (default num is 1)
	// if location is given, only looks for units within a 15.0f radius of the location
	Units GetRandomUnits(UNIT_TYPEID unit_type, Point3D location = Point3D(0,0,0), int num = 1);
	// returns true if unit has finished being built
	bool doneConstruction(const Unit *unit);
	bool build_complete = false;

    std::vector<Point3D> expansions;	// vector of all expansions
    Point3D startLocation;	// location of main base
	Point3D rampLocation;	// location of ramp leading to main base
	const Unit *mainSCV;	// main scv worker
	int supplies;			// supply count
	int minerals;			// mineral count
};

#endif