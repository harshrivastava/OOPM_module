# 🎮 Fantasy RPG - OOP School Project

A text-based RPG game demonstrating Object-Oriented Programming concepts in C++.

## 🎯 Features

- **5 Playable Characters**: Wizard, Sorcerer, Knight, Bard, Zoomer
- **Turn-based Combat** with unique special abilities
- **Random Events**: Battles, treasures, traps, healing fountains
- **Storyteller Narration** throughout the game
- **Boss Battle** against the Mind Flayer

## 🎓 OOP Concepts Demonstrated

1. **Inheritance** - Character → Player → Wizard/Sorcerer/etc.
2. **Polymorphism** - Each character has unique `special_move()`
3. **Encapsulation** - Private/protected members with public getters
4. **Abstraction** - Character is an abstract base class
5. **Composition** - Player "has-a" Inventory

## 🚀 How to Compile

```bash
g++ -std=c++20 dnd_rpg.cpp -O2 -o rpg_game.exe
```

## 🎮 How to Play

```bash
./rpg_game.exe
```

1. Choose your hero (1-5)
2. Survive random events
3. Defeat enemies in turn-based combat
4. Reach Turn 20 and defeat the Mind Flayer to win!

## 📊 Character Stats

| Character | HP  | ATK | DEF | Special Ability |
|-----------|-----|-----|-----|-----------------|
| Wizard    | 120 | 20  | 15  | Arcane Shield (1.5x damage) |
| Sorcerer  | 80  | 25  | 8   | Elemental Fury (costs mana) |
| Knight    | 90  | 22  | 10  | Holy Strike (25% crit) |
| Bard      | 140 | 28  | 12  | Battle Song (rage damage) |
| Zoomer    | 100 | 24  | 9   | Rapid Strike (2x attacks) |

## 📁 Project Structure

- `dnd_rpg.cpp` - Main source code
- `rpg_game.exe` - Compiled executable
- Documentation in `.gemini/antigravity/brain/` folder

## 👨‍💻 Author

School Project - Object-Oriented Programming Module

---

**Enjoy the adventure!** 🎲✨
