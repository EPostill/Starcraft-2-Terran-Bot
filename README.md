### How to build solution Visual Studio Code 2019 and run bot
1. On Terminal, run `start ./build/Hal9001.sln`. This will open Visual Studio Code 2019.
2. On Visual Studio Code 2019, type `Ctrl+Shift+B`. This will build the solution.
3. On Terminal again, run `./build/bin/Hal9001.exe -c -a <enemy> -d <difficulty> -m <map>`. This will start StarCraft 2 and run the bot.


### Start Location per Maps (CactusValleyLE, BelShirVestigeLE, and ProximaStationLE)
`NOTE:` 
- Start Location is where the first CommandCenter is.
- All coordinates are in (x, y) format.
- The following data were extracted from multiple test runs. Do not assume or limit implementation on the coordinates below.

1. CactusValleyLE
    - **NW** ***(upper left corner)*** = (33.5, 158.5)
    - **SW** ***(lower left corner)*** = (33.5, 33.5)
    - **NE** ***(upper right corner)*** = (158.5, 158.5)
    - **SE** ***(lower right corner)*** = (158.5, 33.5)

    - ***Player can have 4 possible starting locations***
    - ***Enemy can have 3 possible starting locations***

2. BelShirVestigeLE
    - **NW** ***(upper left corner)*** = (29.5, 134.5)
    - **SE** ***(lower right corner)*** = (114.5, 25.5)

    - ***Player can have 2 possible starting locations***
    - ***Enemy can have 1 possible starting locations***

3. ProximaStationLE
    - **SW (with a litle bit to the right)** ***(lower left corner)*** = (62.5, 28.5)
    - another one

    - ***Player can have 2 possible starting locations***
    - ***Enemy can have 1 possible starting locations***

```
int player_possible = (observation -> GetGameInfo()).start_locations.size();
int enemy_possible (observation -> GetGameInfo()).enemy_start_locations.size();

// Coordinates in GetGameInfo().enemy_start_locations can be use for scouting enemy base
```

### How to get map info
```
// use ObservationInterface *observation in OnGameStart()
std::string map_name = observation -> GetGameInfo().map_name; // this will get the name of the map
```

### How to get radius of to be build structure
Get all the abilities that can be accessed via the Observation Interface. Loop through it, find matching ability id, then return footprint_radius

See `radiusOfToBeBuilt()` in Hal9001.cpp.
`NOTE:` Maybe extract helper functions later, to avoid longer file.

```
/*
@desc 	This will return the radius of the structure to be built
@param	abilityId - BUILD_SUPPLYDEPOT, etc
*/
float Hal9001::radiusOfToBeBuilt(ABILITY_ID abilityId){
    // TODO:: add assertion that only "build-type" abilities will be accepted
    
    // get observation
    const ObservationInterface *observation = Observation();

    // get vector of abilities
    const vector<AbilityData> abilities = observation ->GetAbilityData();

    // This will be the index of the searched ability
    int index;

    // loop through vector to search for ability
    for(int i = 0; i < abilities.size(); ++i){
        if(abilities[i].ability_id == abilityId){
            index = i;
        }
    }

    cout << abilities[index].link_name << " " << abilities[index].is_building << endl;

    // return the footprint radius of ability
    return abilities[index].footprint_radius;
}
```

### Relative Direction of Expansion based on Orientation
`Explanation:` Expansion depends on where the base is located on the a given map. 

In `CactusValleyLE`, player can be assigned to any of the four corners. Therefore in expansion, `getRelativeDir()` is important. This function will say where will another structure be built relative to a given unit structure (anchor). There are only 5 choices of direction `FRONT`, `LEFT`, `RIGHT`, `FRONTLEFT`, `FRONTRIGHT`. There are no `BACK` since it is an expansion. Remember that this directions are **relative** to the given object.

See this pics for more detail:

<img src="https://raw.githubusercontent.com/EPostill/CMPUT350Project/phase-1/docs/Orientation-CactusValleyLe.png?token=ALIZXBDLDBSQWXXT4WEK7PC7YIB2S" width="500">
<img src="https://raw.githubusercontent.com/EPostill/CMPUT350Project/phase-1/docs/Relative%20Location%20Building%20(Build%20Order%20%23%202).png?token=ALIZXBGNAUHYRZLDSPFHF6C7YICBG" width="500">
