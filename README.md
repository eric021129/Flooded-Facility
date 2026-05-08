# Flooded Facility 🏃‍♂️💧

**A fast-paced C++ puzzle game where you fix broken code before the room floods!**

<img width="442" height="261" alt="Screenshot 2025-11-25 at 7 08 53 PM" src="https://github.com/user-attachments/assets/c8a89b62-0a23-4985-a4ef-d4a8b5427174" />


## Team members
_Group 12 COMP2113 Project_

| _Name_                   | _UID_      | _Profile_                                          |
| :---                     | :---       | :---                                               |
| Bekamanova Samira        | 3036344969 | [@glimmurr](https://github.com/glimmurr)           |
| Dohyun Wang              | 3036279114 | [@wangdohyun13](https://github.com/wangdohyun13)   |
| Choi Tsz Yan             | 3036408222 | [@Ctyannn](https://github.com/Ctyannn)             |
| Hwang Jeong U            | 3035859113 | [@eric021129](https://github.com/eric021129)       |
| Etsehiwot Ayalew Bezabih | 3036407785 | [@Etse2127](https://github.com/Etse2127)           |

## Quick Demo 

https://github.com/user-attachments/assets/ac1e8285-09db-4b69-bbd3-da07e744130a 

## Game Overview
You are Dr. Alex Chen🧑🏻‍💻, a C++ programmer working late at the AquaTech Research Facility. 

Suddenly, rooms start **flooding** and water is **rapidly filling with water** time by time. 💦💦

The main control system is corrupted! ☠️☠️☠️

The only way out is to repair all the malfunctioning machines🤖 by **solving real C++ code puzzles** 
to stop the flood and save your life!

If the rising water touches you or any unfixed machines → **Game Over** 🔚

Fix every machine before it’s too late → **You survive!** 🎉🎉

## Features 
-  **Randomized machine placement** every round!
-  **Data structures** for storing data:
   -  Classes: `Game` handles the main menus, UI flow, and data saving/loading; `FloodLevel` handles the stage gameplay, randomized map generation, etc.
   -  Structs: `SaveSlot` stores information on the player's save slot; `Machine` stores the coordinates of each machine, etc.

- **Dynamic memory management**
   -  Vectors: `codes`, `options`, `answers` store the information on the code puzzles, options, and answers for each machine, etc.

-  **Progress saving** – the game remembers the highest stage you have ever beaten and outputs it to `player_data.txt`!

- **Three difficulty levels** to choose:
  
  | Difficulty    | # of Machines | Water Speed | # of Blanks |
  |---------------|---------------|-------------|-------------|
  | 1. Easy       | 2             | Slow        | 2           |
  | 2. Normal     | 3             | Medium      | 3           |
  | 3. Hard       | 4             | Fast        | 3-4         |

-  **5 different maps** to play!

## Non-standard libraries
None, standard libraries only.

## How to play

1. Choose your difficulty (Easy / Normal / Hard)
2. Move with **”WASD“** within the map
3. Approach a broken machine and press **”Enter“** to open the coding terminal
4. Filling in the blanks:
   - Press **”A/D“** to choose a blank to be filled
   - Press **”W/S“** to choose an answer to fill in the blank
   - Press **”Enter“** to fill in the blank
   - Check your answer with **”R“** 
5. Go for the next machine until you fixed them all! 

### Controls Summary
| Action                              | Key          |
|-------------------------------------|--------------|
| Move and Choose                     | WASD         | 
| Select                              | Enter        |
| Back to main menu or Quit the game  | Q            | 
| Interact with the machine           | Enter        |
| Fill in the blanks                  | Enter        |
| Check your answer                   | R            |


## Compilation instructions
To play, simply type this in the console and enjoy! 
```
make
```
```
./game
```
