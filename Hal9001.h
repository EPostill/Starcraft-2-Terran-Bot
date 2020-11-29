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
enum RelDir { LEFT, RIGHT, FRONT, FRONTLEFT, FRONTRIGHT, BEHINDLEFT, BEHINDRIGHT, BEHIND };

// For easy identification of location in the map
// M for missing
enum Corner { NW, SW, NE, SE, M };

class Hal9001 : public sc2::Agent {
public:
	virtual void OnGameStart();
	virtual void OnStep();

	virtual void OnUnitIdle(const Unit* unit);
	//makes scvs without orders mine by default
	void MineIdleWorkers();
	// returns mineral patch nearest to start
	const Unit* FindNearestMineralPatch(const Point2D &start);
	// returns geyser nearest to start
	const Unit* FindNearestGeyser(const Point2D &start);
	// returns supply depot nearest to start
	const Unit* FindNearestDepot(const Point2D &start);
	// returns free expansion location nearest to main base
	const Point3D FindNearestExpansion();
	void BuildStructure(ABILITY_ID ability_type_for_structure, float x, float y, const Unit *builder = nullptr);
	void BuildOrder(const ObservationInterface *observation);
	// Manage our army
	void ManageArmy();
	// build a refinery near the given command center
	void BuildRefinery(const Unit *commcenter, const Unit *builder = nullptr);
	void updateSupplies();
	void ReconBase(const ObservationInterface* observation);
	
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
	// gets all depots (lowered and raised)
	Units getDepots();
	// gets all widow mines (burrowed and raised)
	Units getWidowMines();
	// gets all comm centers (orbital and normal)
	Units getCommCenters();


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
	
	Point2D PointTowardCenter(GameInfo game_info_, Point3D buildingloc, float ratio);

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
	@desc attempts to build a unit
	@param unit, ability
	@return if the action was performed successfully
	*/
	bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type);

	/*
	@desc gets a random unit from the list
	@param unit, unit type
	@return if a unit was gotten
	*/
	bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type);


	/*
	@desc This will return where a structure is in the map
	@param unit
	@return Corner enum
	*/
	Corner cornerLoc(const Unit* unit);

	/*
	@desc This will return true or false if a build ability is placeable in the given position
	@param unit
	@return bool
	*/
	bool isPlaceable(ABILITY_ID abilityId, Point2D points);

	/*
	@desc This will return true or false if a order is already given to an scv and is in progress
	@param abilityid
	@return bool
	*/
	bool isOrdered(ABILITY_ID abilityId, UNIT_TYPEID unitTypeId);

	/*
	@desc This will land a build structure in the vicinity
	@param unit id for building, rel dir (try to land here first)
	@return void
	*/
	void landFlyer(const Unit* flyer, RelDir relDir, ABILITY_ID aid_to_land);

private:
	// counts the number of units of a given type (does not include those in training)
	size_t CountUnitType(UNIT_TYPEID unit_type);
	// returns a vec of units of given type (does not include those in training)
	Units GetUnitsOfType(UNIT_TYPEID unit_type);
	// returns a vec of a certain number of units of the given type (default num is 1)
	// if location is given, only looks for units within a 30.0 radius of the location
	Units GetRandomUnits(UNIT_TYPEID unit_type, Point3D location = Point3D(0,0,0), int num = 1);
	// returns true if unit has finished being built
	bool doneConstruction(const Unit *unit);
	bool build_complete = false;

	// Check if we're currently scouting
	bool scouting = false;
	// Check if the enemybase has been found
	bool enemyBaseFound = false;
	// Check if build order is finished
	bool buildOrderComplete = false;
	
	// Check which possible location we're currently checking
	//bool L1;
	//bool L2;
	//bool L3;

    std::vector<Point3D> expansions;	// vector of all expansions
    Point3D startLocation;	// location of main base
	Point2D depotLocation;	// location where to build the first depot
	Point2D enemyBase;
	const Unit *mainSCV;	// main scv worker
	const Unit* scout;		// scout unit
	int supplies;			// supply count
	int minerals;			// mineral count
	int vespene;			// gas count

	// map name
	MapName map_name;

	// map width and height
	int map_width;
	int map_height;

	// which corner the base is on the map
	Corner corner_loc;

};

#endif
