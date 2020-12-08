### How to build solution Visual Studio Code 2019 and run bot
1. On root of project folder, make a folder called `build`. Than `cd` to `build` 
2. On Terminal, run `cmake ../ -G "Visual Studio 16 2019`. Then go back to root directory.
3. On Terminal, run `start ./build/Hal9001.sln`. This will open Visual Studio Code 2019.
4. On Visual Studio Code 2019, type `Ctrl+Shift+B`. This will build the solution.
5. On Terminal again, run `./build/bin/Hal9001.exe -c -a <enemy> -d <difficulty> -m <map>`. This will start StarCraft 2 and run the bot.