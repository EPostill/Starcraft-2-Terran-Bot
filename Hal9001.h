#ifndef HAL_9001_H_
#define HAL_9001_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2lib/sc2_lib.h"

#include "sc2api/sc2_unit_filters.h"
#include "sc2lib/sc2_search.h"
#include "sc2api/sc2_data.h"

using namespace sc2;

// For easy identification of map_name throught the code
enum MapName { CACTUS, BELSHIR, PROXIMA };

// For easy identification of where to place a structure relative to another
enum RelDir { LEFT, RIGHT, FRONT, FRONTLEFT, FRONTRIGHT };

// For easy identification of location in the map
// M for missing
enum Corner { NW, SW, NE, SE, M };

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
	
	// policy for training scvs
	void ManageSCVTraining();
	// policy for gathering vespene gas
	void ManageRefineries();
	// expands to free expansion that is closest to main base
	void Expand();
	// moves unit to target position
	void moveUnit(const Unit *unit, const Point2D &target);
	// sets mainSCV to the first trained scv and moves it to first supply depot build location
	void initializeMainSCV(Units &bases);
	// gets the location of where to build the first supply depot
	const Point2D getFirstDepotLocation(const Unit *commcenter);
	// checks if any scv has the given order
	bool alreadyOrdered(ABILITY_ID ability_id);

	void step14();
	void step15();
	void step16();


	// Helper functions
	// TODO: Public for now, move to private later

	/*
	@desc Builds a building adjacent to reference
	*/
	void buildNextTo(ABILITY_ID ability_id, const Unit* ref, RelDir relDir, int dist, const Unit *builder = nullptr);

	/*
	@desc 	This will return the radius of the structure to be built
	@param	abilityId - BUILD_SUPPLYDEPOT, etc
	*/
	float radiusOfToBeBuilt(ABILITY_ID abilityId);

	/*
	@desc   This will return a tuple (1,0), (1, -1), etc. 
	@param  anchor - a unit, will build relative to this
			dir    - RelDir enum
	*/
	std::pair<int, int> getRelativeDir(const Unit *anchor, const RelDir dir);

	/*
	@desc   This will return where another structure is relative to another structure anchor
			Layman: "Is target on the left or right of anchor"

	@param  anchor - a unit, from this
			target - a unit, to this

	@return LEFT or RIGHT (enum)
	*/
	RelDir getHandSide(const Unit* anchor, const Unit *target);


	/*
	@desc This will return where a structure is in the map
	@param unit
	@return Corner enum
	*/
	Corner cornerLoc(const Unit* unit);

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
	Point2D depotLocation;	// location where to build the first depot
	const Unit *mainSCV;	// main scv worker
	int supplies;			// supply count
	int minerals;			// mineral count
	int vespene;			// gas count

	// map name
	MapName map_name;

	// map width and height
	int map_width;
	int map_height;

};

#endif