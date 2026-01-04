// ============================================================================
// STRANGER THINGS: THE UPSIDE DOWN - Text-Based RPG
// ============================================================================
// School Project: Object-Oriented Programming (OOP) Demonstration
// Theme: Stranger Things Netflix Series
// Language: C++20
// Compile: g++ -std=c++20 dnd_rpg.cpp -O2 -o stranger_things_rpg
//
// OOP CONCEPTS DEMONSTRATED:
// 1. INHERITANCE    - Player/Enemy classes inherit from Character base class
// 2. POLYMORPHISM   - special_move() behaves differently for each character
// 3. ENCAPSULATION  - Private members with public getter/setter methods
// 4. ABSTRACTION    - Character is abstract (has pure virtual function)
// 5. COMPOSITION    - Player "has-a" Inventory object
//
// GAME DESCRIPTION:
// Fight monsters from the Upside Down as your favorite Stranger Things heroes!
// Choose from Mike Wheeler, Eleven, Steve Harrington, or Jim Hopper.
// Battle Demobats, Demodogs, Flayed Ones, and face the Mind Flayer!
// ============================================================================

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::literals;

// ============================================================================
// DICE CLASS - Random Number Generator
// ============================================================================
// Purpose: Simulates dice rolls for combat and random events
// Used for: Attack rolls, damage calculation, chance-based events
// ============================================================================
class Dice {
    // Private member: Random number generator engine
    // std::mt19937 is a high-quality random number generator (Mersenne Twister)
    std::mt19937 engine;

public:
    // Constructor: Initialize the random engine with a random seed
    Dice() : engine((std::random_device{})()) {}

    // Roll a dice with 'sides' number of sides (e.g., roll(20) = d20)
    // Returns: Random number between 1 and sides (inclusive)
    int roll(int sides) {
        if (sides <= 1) return 1;  // Minimum roll is 1
        std::uniform_int_distribution<int> dist(1, sides);
        return dist(engine);
    }

    // Check if a random event happens based on percentage chance
    // Example: chance(25) has 25% probability of returning true
    bool chance(int percent) {
        return roll(100) <= percent;
    }
};

// ============================================================================
// ITEM STRUCT - Simple Data Container
// ============================================================================
// Note: 'struct' is like a class but members are public by default
// Used for: Simple data storage without complex behavior
// ============================================================================
struct Item {
    std::string name;        // Item name (e.g., "healing_potion")
    std::string type;        // Item category: "potion", "weapon", "armor"
    int effect = 0;          // Effect value (healing amount, damage bonus, etc.)
};

// Forward declaration: Tell compiler that Player class exists
// Needed because Inventory::use_item() takes a Player parameter
class Player;

// ============================================================================
// INVENTORY CLASS - Item and Gold Management
// ============================================================================
// OOP CONCEPT: COMPOSITION
// Player "has-a" Inventory (composition relationship)
// ============================================================================
class Inventory {
    // ENCAPSULATION: Private members can't be accessed directly from outside
    std::vector<Item> items;  // Dynamic array of items (can grow/shrink)
    int gold = 0;             // Player's gold amount

public:
    void add_item(const Item &it) { items.push_back(it); }
    void add_gold(int amount) { gold += amount; }
    int get_gold() const noexcept { return gold; }
    void set_gold(int value) noexcept { gold = value; }

    bool has_item(std::string_view name) const {
        for (auto &it : items)
            if (it.name == name) return true;
        return false;
    }

    const std::vector<Item> &get_items() const noexcept { return items; }

    std::optional<Item> remove_item(std::string_view name) {
        auto it = std::find_if(items.begin(), items.end(),
                               [&](auto &i) { return i.name == name; });
        if (it != items.end()) {
            Item copy = *it;
            items.erase(it);
            return copy;
        }
        return std::nullopt;
    }

    // Use an item on player. Returns optional error message (empty on success)
    std::optional<std::string> use_item(std::string_view name, Player &player);
};

// ---------------------- Character (base) ----------------------
class Character {
protected:
    std::string name;
    int health;
    int max_health;
    int attack;
    int defense;

public:
    Character(std::string n, int hp, int atk, int def)
        : name(std::move(n)), health(hp), max_health(hp), attack(atk), defense(def) {}

    virtual ~Character() = default;

    const std::string &get_name() const noexcept { return name; }
    int get_health() const noexcept { return health; }
    int get_max_health() const noexcept { return max_health; }
    int get_attack() const noexcept { return attack; }
    int get_defense() const noexcept { return defense; }
    bool is_alive() const noexcept { return health > 0; }

    virtual void take_damage(int dmg) {
        int actual = std::max(0, dmg - defense);
        health = std::max(0, health - actual);
    }

    void heal(int amount) { health = std::min(max_health, health + amount); }

    virtual void attack_move(Character &target) {
        Dice dice;
        int roll = dice.roll(20);
        int total = roll + attack;
        int dmg = std::max(0, total - target.get_defense());
        target.take_damage(dmg);
    }

    virtual void special_move(Character &target) = 0;

    void print_stats() const {
        std::cout << name << " | HP: " << health << '/' << max_health
                  << " | ATK: " << attack << " | DEF: " << defense << '\n';
    }
};

// ---------------------- Player (base) ----------------------
class Player : public Character {
protected:
    int mana = 100;
    int max_mana = 100;
    int rage = 0;
    Inventory inventory;

public:
    Player(std::string n, int hp, int atk, int def) : Character(std::move(n), hp, atk, def) {}

    int get_mana() const noexcept { return mana; }
    int get_max_mana() const noexcept { return max_mana; }
    int get_rage() const noexcept { return rage; }
    Inventory &get_inventory() noexcept { return inventory; }

    void restore_mana(int amount = 10) { mana = std::min(max_mana, mana + amount); }
    void spend_mana(int cost) { mana = std::max(0, mana - cost); }
    void add_to_rage(int amount) { rage = std::min(100, rage + amount); }
    void reset_rage() { rage = 0; }

    void print_full_stats() const {
        print_stats();
        std::cout << "  Mana: " << mana << '/' << max_mana << " | Rage: " << rage << "/100\n";
    }
};

// Implement Inventory::use_item
std::optional<std::string> Inventory::use_item(std::string_view name, Player &player) {
    auto opt = remove_item(name);
    if (!opt) {
        return std::string("You don't have '"s + std::string(name) + "'.");
    }
    Item it = *opt;
    if (it.type == "potion") {
        if (it.name == "healing_potion") {
            player.heal(it.effect);
            std::cout << "🧪 You used a Healing Potion and restored " << it.effect << " HP!\n";
            return std::nullopt;
        } else if (it.name == "mana_potion") {
            player.restore_mana(it.effect);
            std::cout << "💧 You used a Mana Potion and restored " << it.effect << " Mana!\n";
            return std::nullopt;
        } else {
            // unknown potion, put it back
            add_item(it);
            return std::string("Unknown potion type.");
        }
    } else {
        // For now other types cannot be used directly
        add_item(it);
        return std::string("Can't use '"s + std::string(name) + "' right now.");
    }
}

// ============================================================================
// PLAYER CLASSES - Heroes of the Realm
// ============================================================================
// OOP CONCEPT: INHERITANCE + POLYMORPHISM
// Each hero inherits from Player and implements their own special_move()
// This demonstrates polymorphism: same function name, different behavior
// ============================================================================

// WIZARD - The Arcane Scholar
// Role: Tank (high HP and defense, strategic magic)
// Special: Arcane Shield - Protective magic with bonus damage
class Wizard : public Player {
public:
    // Constructor: Initialize Wizard with tank stats
    // HP: 120 (high), ATK: 20 (medium), DEF: 15 (high)
    Wizard() : Player("Wizard", 120, 20, 15) {
        // Starting inventory: 2 healing potions and some gold
        inventory.add_item({"healing_potion", "potion", 30});
        inventory.add_item({"healing_potion", "potion", 30});
        inventory.add_gold(20);
    }

    // OOP CONCEPT: POLYMORPHISM - Override special_move() with Wizard's ability
    // Wizard's Special: "Arcane Shield" - Protective magic with bonus damage
    void special_move(Character &target) override {
        Dice dice;
        int roll = dice.roll(20);
        int total_attack = roll + attack;
        // 1.5x damage multiplier (arcane power)
        int dmg = static_cast<int>(std::max(0, (total_attack - target.get_defense())) * 1.5);
        target.take_damage(dmg);
        std::cout << "🔮 Wizard cast ARCANE SHIELD! Dealt " << dmg << " damage!\n";
    }
};


// SORCERER - The Elemental Master
// Role: Burst Damage (high attack, uses elemental magic)
class Sorcerer : public Player {
public:
    // Constructor: Initialize Sorcerer with glass cannon stats
    // HP: 80 (low), ATK: 25 (very high), DEF: 8 (low)
    Sorcerer() : Player("Sorcerer", 80, 25, 8) {
        inventory.add_item({"healing_potion", "potion", 20});
        inventory.add_item({"mana_potion", "potion", 30});  // "Mana Restore"
        inventory.add_gold(30);
    }

    // Sorcerer's Special: "ELEMENTAL FURY" - Powerful elemental attack
    // Costs mana but deals massive damage
    void special_move(Character &target) override {
        constexpr int COST = 30;  // Mana cost
        
        // Check if enough mana available
        if (mana < COST) {
            std::cout << "❌ Not enough mana! (" << mana << "/" << COST << ")\n";
            return;
        }
        
        spend_mana(COST);  // Use mana
        Dice dice;
        int roll = dice.roll(20);
        int total_attack = roll + attack + 10;  // +10 bonus for elemental power
        int dmg = std::max(0, total_attack - target.get_defense());
        target.take_damage(dmg);
        std::cout << "🔥 Sorcerer unleashed ELEMENTAL FURY! Dealt " << dmg << " damage!\n";
    }
};


// KNIGHT - The Noble Warrior
// Role: Critical Hitter (medium stats, high crit chance)
class Knight : public Player {
public:
    // Constructor: Initialize Knight with balanced stats
    // HP: 90 (medium), ATK: 22 (medium-high), DEF: 10 (medium)
    Knight() : Player("Knight", 90, 22, 10) {
        inventory.add_item({"healing_potion", "potion", 25});
        inventory.add_gold(40);
    }

    // Knight's Special: "HOLY STRIKE" - High critical hit chance
    // 25% chance to deal 2.5x damage (critical hit)
    void special_move(Character &target) override {
        Dice dice;
        int roll = dice.roll(20);
        bool crit = dice.chance(25);  // 25% critical hit chance
        int base_dmg = std::max(0, (roll + attack) - target.get_defense());
        int dmg = crit ? static_cast<int>(base_dmg * 2.5) : base_dmg;
        target.take_damage(dmg);
        
        if (crit)
            std::cout << "⚔️ Knight used HOLY STRIKE! CRITICAL HIT! Dealt " << dmg << " damage!\n";
        else
            std::cout << "⚔️ Knight used HOLY STRIKE! Dealt " << dmg << " damage!\n";
    }
};


// BARD - The Charismatic Performer
// Role: Support/DPS hybrid (high HP, rage-based damage)
class Bard : public Player {
public:
    // Constructor: Initialize Bard with hybrid stats
    // HP: 140 (very high), ATK: 28 (very high), DEF: 12 (medium-high)
    Bard() : Player("Bard", 140, 28, 12) {
        inventory.add_item({"healing_potion", "potion", 40});
        inventory.add_gold(10);
        rage = 20;  // Starts with some rage (performance energy)
    }

    // Bard's Special: "BATTLE SONG" - Damage increases when hurt
    // The more HP missing, the more bonus damage (inspiring performance)
    void special_move(Character &target) override {
        int missing_hp = max_health - health;
        int rage_bonus = missing_hp / 10;  // +1 damage per 10 HP lost
        
        Dice dice;
        int roll = dice.roll(20);
        int total_attack = roll + attack + rage_bonus;
        int dmg = std::max(0, total_attack - target.get_defense());
        target.take_damage(dmg);
        add_to_rage(15);  // Gain rage after using ability
        
        std::cout << "🎵 Bard performed BATTLE SONG! Dealt " << dmg << " damage (+"
                  << rage_bonus << " from inspiration)!\n";
    }
};


// ZOOMER - The Swift Assassin
// Role: Speed-based DPS (medium HP, multiple quick strikes)
class Zoomer : public Player {
public:
    // Constructor: Initialize Zoomer with speed-focused stats
    // HP: 100 (medium), ATK: 24 (high), DEF: 9 (low-medium)
    Zoomer() : Player("Zoomer", 100, 24, 9) {
        inventory.add_item({"healing_potion", "potion", 25});
        inventory.add_item({"healing_potion", "potion", 25});
        inventory.add_gold(35);
    }

    // Zoomer's Special: "RAPID STRIKE" - Multiple quick attacks
    // Attacks twice in one turn with reduced damage
    void special_move(Character &target) override {
        Dice dice;
        
        // First strike
        int roll1 = dice.roll(20);
        int dmg1 = std::max(0, (roll1 + attack) - target.get_defense());
        target.take_damage(dmg1);
        
        // Second strike (if enemy still alive)
        if (target.is_alive()) {
            int roll2 = dice.roll(20);
            int dmg2 = std::max(0, (roll2 + attack) - target.get_defense());
            target.take_damage(dmg2);
            std::cout << "⚡ Zoomer used RAPID STRIKE! Dealt " << dmg1 << " + " << dmg2 << " = " << (dmg1 + dmg2) << " damage!\n";
        } else {
            std::cout << "⚡ Zoomer used RAPID STRIKE! First hit dealt " << dmg1 << " damage (enemy defeated)!\n";
        }
    }
};

// ============================================================================
// ENEMY BASE CLASS
// ============================================================================
// OOP CONCEPT: INHERITANCE
// Enemy inherits from Character and adds boss-specific functionality
// ============================================================================
class Enemy : public Character {
    bool is_boss_ = false;  // Flag to mark boss enemies

public:
    // Constructor: Pass parameters to Character base class
    Enemy(std::string n, int hp, int atk, int def) 
        : Character(std::move(n), hp, atk, def) {}
    
    // Virtual destructor (important for proper cleanup in inheritance)
    virtual ~Enemy() = default;

    // Boss management functions
    void set_is_boss(bool b) noexcept { is_boss_ = b; }
    bool is_boss() const noexcept { return is_boss_; }

    // OOP CONCEPT: POLYMORPHISM - Override attack_move from Character
    void attack_move(Character &target) override {
        Dice dice;
        int roll = dice.roll(20);  // Roll d20
        int base_dmg = std::max(0, (roll + attack) - target.get_defense());
        
        // 30% chance for bonus psychic damage (Upside Down energy)
        int psychic_dmg = dice.chance(30) ? 15 : 0;
        target.take_damage(base_dmg + psychic_dmg);
        
        if (psychic_dmg > 0) 
            std::cout << "⚡ " << name << " unleashes psychic energy!\n";
    }

    // Default special move (does nothing for basic enemies)
    void special_move(Character &) override { /* no-op */ }
};

// ============================================================================
// ENEMY CLASSES - Monsters from the Upside Down
// ============================================================================
// OOP CONCEPT: INHERITANCE
// All enemies inherit from Enemy base class, which inherits from Character
// Inheritance hierarchy: Character → Enemy → Demobat/Demodog/etc.
// ============================================================================

// DEMOBAT - Weak flying creature from the Upside Down
// Based on: Stranger Things Season 4 bat-like creatures
// Difficulty: Easy (Common enemy)
class Demobat : public Enemy {
public:
    // Constructor: Initialize with Demobat-specific stats
    // HP: 25 (low), ATK: 12 (low), DEF: 4 (very low)
    Demobat() : Enemy("Demobat", 25, 12, 4) {}
};

// DEMODOG - Adolescent Demogorgon, pack hunter
// Based on: Stranger Things Season 2 dog-like creatures
// Difficulty: Medium (Uncommon enemy)
class Demodog : public Enemy {
public:
    // Constructor: Medium difficulty stats
    // HP: 50 (medium), ATK: 16 (medium), DEF: 7 (low-medium)
    Demodog() : Enemy("Demodog", 50, 16, 7) {}
};

// FLAYED ONE - Human possessed by the Mind Flayer
// Based on: Stranger Things Season 3 possessed victims
// Difficulty: Hard (Rare enemy)
class FlayedOne : public Enemy {
public:
    // Constructor: High difficulty stats
    // HP: 80 (high), ATK: 20 (high), DEF: 10 (medium)
    FlayedOne() : Enemy("Flayed One", 80, 20, 10) {}
};

// MIND FLAYER - The Shadow Monster, final boss
// Based on: Stranger Things Season 2-3 main antagonist
// Difficulty: BOSS (Final enemy)
class MindFlayer : public Enemy {
public:
    // Constructor: Boss-level stats
    // HP: 250 (very high), ATK: 35 (very high), DEF: 18 (high)
    MindFlayer() : Enemy("Mind Flayer", 250, 35, 18) { 
        set_is_boss(true);  // Mark as boss enemy
    }
};

// ---------------------- Game Engine ----------------------
class GameEngine {
    Dice dice;
    std::unique_ptr<Player> player;
    int turns = 0;
    bool dragon_defeated = false;

    static int get_choice(int min, int max) {
        int choice;
        while (true) {
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input. Try again: ";
                continue;
            }
            if (choice >= min && choice <= max) {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return choice;
            }
            std::cout << "Choose between " << min << " and " << max << ": ";
        }
    }

    bool ask_yes_no(std::string_view prompt) {
        std::string input;
        while (true) {
            std::cout << prompt << " (y/n): ";
            if (!std::getline(std::cin, input)) return false;
            if (input.empty()) continue;
            char c = static_cast<char>(std::tolower(static_cast<unsigned char>(input[0])));
            if (c == 'y') return true;
            if (c == 'n') return false;
            std::cout << "Please enter 'y' or 'n'.\n";
        }
    }

    void show_main_menu() {
        std::cout << "\n========================================\n";
        std::cout << "🎮 STRANGER THINGS: THE UPSIDE DOWN 🎮\n";
        std::cout << "========================================\n";
        std::cout << "\n📖 Storyteller: \"Greetings, brave adventurer! The realm needs heroes...\"\n";
        std::cout << "1. Start Game\n2. Exit\n";
        std::cout << "Choose an option: ";
    }

    void show_class_selection() {
        std::cout << "\n📖 Storyteller: \"Five legendary heroes stand before you. Choose wisely...\"\n";
        std::cout << "\nChoose your hero:\n";
        std::cout << "1. Wizard     (Tank/Magic)\n";
        std::cout << "2. Sorcerer   (Burst/Elemental)\n";
        std::cout << "3. Knight     (Balanced/Crit)\n";
        std::cout << "4. Bard       (Support/Rage)\n";
        std::cout << "5. Zoomer     (Speed/Multi-hit)\n";
        std::cout << "\nYour choice: ";
    }

    void initialize_player(int choice) {
        // OOP CONCEPT: POLYMORPHISM - Store different player types in same pointer
        // std::unique_ptr<Player> can point to any child class (Wizard, Sorcerer, etc.)
        switch (choice) {
        case 1: player = std::make_unique<Wizard>(); break;
        case 2: player = std::make_unique<Sorcerer>(); break;
        case 3: player = std::make_unique<Knight>(); break;
        case 4: player = std::make_unique<Bard>(); break;
        case 5: player = std::make_unique<Zoomer>(); break;
        default: player = std::make_unique<Wizard>(); break;
        }
        std::cout << "\n📖 Storyteller: \"Ah, " << player->get_name() << "! A fine choice indeed...\"\n";
        std::cout << "🌟 You are " << player->get_name() << "!\n";
        player->print_full_stats();
        std::cout << "Starting gold: " << player->get_inventory().get_gold() << "\n";
        std::cout << "\n📖 Storyteller: \"Your journey begins now. May fortune favor you!\"\n";
    }

    std::unique_ptr<Enemy> spawn_random_enemy() {
        // After turn 20, spawn the final boss (Mind Flayer)
        if (turns >= 20 && !dragon_defeated) {
            std::cout << "\n📖 Storyteller: \"The air grows cold... darkness approaches...\"\n";
            std::cout << "\n🌩️  The Upside Down tears open... THE MIND FLAYER EMERGES!\n";
            std::cout << "📖 Storyteller: \"This is it, hero! The final battle begins!\"\n";
            return std::make_unique<MindFlayer>();
        }

        // Random enemy spawning (weighted probabilities)
        int r = dice.roll(100);
        if (r <= 40) {
            std::cout << "\n📖 Storyteller: \"A creature stirs in the shadows...\"\n";
            return std::make_unique<Demobat>();
        }
        if (r <= 70) {
            std::cout << "\n📖 Storyteller: \"You hear growling in the distance...\"\n";
            return std::make_unique<Demodog>();
        }
        if (r <= 95) {
            std::cout << "\n📖 Storyteller: \"An eerie presence fills the air...\"\n";
            return std::make_unique<FlayedOne>();
        }
        std::cout << "\n📖 Storyteller: \"Impossible! The Mind Flayer appears early!\"\n";
        return std::make_unique<MindFlayer>();
    }

    void battle(std::unique_ptr<Enemy> &enemy) {
        std::cout << "\n========================================\n";
        std::cout << "📖 Storyteller: \"Steel yourself! Battle is upon you!\"\n";
        std::cout << " BATTLE: " << player->get_name() << " vs " << enemy->get_name() << "\n";
        enemy->print_stats();

        bool enemy_stunned = false;

        while (player->is_alive() && enemy->is_alive()) {
            std::cout << "\n--- Your Turn ---\n";
            player->print_full_stats();
            std::cout << enemy->get_name() << " HP: " << enemy->get_health() << "/" << enemy->get_max_health() << "\n";
            std::cout << "1. Attack | 2. Special | 3. Item | 4. Run | 5. Inspect\n";
            std::cout << "Choose: ";

            int choice = get_choice(1, 5);

            if (choice == 1) {
                int prev = enemy->get_health();
                player->attack_move(*enemy);
                std::cout << "👊 You hit for " << (prev - enemy->get_health()) << " damage!\n";
            } else if (choice == 2) {
                player->special_move(*enemy);
                // small stun mechanic for Wizard's arcane shield
                if (dynamic_cast<Wizard *>(player.get()) && dice.chance(25)) {
                    enemy_stunned = true;
                    std::cout << "🎯 " << enemy->get_name() << " is STUNNED!\n";
                }
            } else if (choice == 3) {
                const auto &items = player->get_inventory().get_items();
                if (items.empty()) {
                    std::cout << "🎒 Inventory empty.\n";
                    continue;
                }
                std::cout << "\nInventory:\n";
                for (size_t i = 0; i < items.size(); ++i) {
                    auto &it = items[i];
                    std::cout << i + 1 << ". " << it.name;
                    if (it.type == "potion") {
                        std::cout << " (" << it.effect << ")";
                    }
                    std::cout << "\n";
                }
                std::cout << "Select (0=cancel): ";
                int sel = get_choice(0, static_cast<int>(items.size()));
                if (sel == 0) continue;
                auto err = player->get_inventory().use_item(items[sel - 1].name, *player);
                if (err) {
                    std::cout << "⚠️  " << *err << "\n";
                }
            } else if (choice == 4) {
                int rate = enemy->is_boss() ? 20 : 70;
                if (dice.chance(rate)) {
                    std::cout << "🏃 Escaped!\n";
                    return;
                } else {
                    std::cout << "❌ Escape failed!\n";
                    enemy->attack_move(*player);
                    std::cout << "💥 Took " << (player->get_max_health() - player->get_health()) << " damage!\n";
                    if (!player->is_alive()) break;
                }
            } else { // inspect
                std::cout << "\n── " << enemy->get_name() << " ──\n";
                enemy->print_stats();
                std::cout << "(Press Enter to continue)";
                std::cin.get();
                continue;
            }

            if (!enemy->is_alive()) {
                std::cout << "\n📖 Storyteller: \"Victory is yours! Well fought, hero!\"\n";
                std::cout << "\n🎉 Victory!\n";
                int gold = dice.roll(20) + (enemy->is_boss() ? 100 : 10);
                player->get_inventory().add_gold(gold);
                std::cout << "💰 Looted " << gold << " gold.\n";
                int heal_amount = std::max(1, player->get_max_health() / 5);
                player->heal(heal_amount);
                std::cout << "✨ Restored " << heal_amount << " HP after battle.\n";
                if (!enemy->is_boss() && dice.chance(40)) {
                    player->get_inventory().add_item({"healing_potion", "potion", 30});
                    std::cout << "🧪 Found a Healing Potion!\n";
                }
                if (enemy->is_boss()) dragon_defeated = true;
                return;
            }

            // Enemy turn
            std::cout << "\n--- Enemy Turn ---\n";
            if (enemy_stunned) {
                std::cout << "😵 " << enemy->get_name() << " is stunned and skips its turn!\n";
                enemy_stunned = false;
            } else {
                int prev = player->get_health();
                enemy->attack_move(*player);
                std::cout << "💢 " << enemy->get_name() << " hits you for " << (prev - player->get_health()) << " damage!\n";
            }
        }
    }

    void treasure_room() {
        std::cout << "\n📖 Storyteller: \"Ah! Fortune smiles upon you!\"\n";
        std::cout << "\n💎 Treasure Room!\n";
        int gold = dice.roll(30) + 20;
        player->get_inventory().add_gold(gold);
        std::cout << "💰 Found " << gold << " gold.\n";
        if (dice.chance(50)) {
            player->get_inventory().add_item({"healing_potion", "potion", 30});
            std::cout << "🧪 Healing Potion!\n";
        }
        if (dice.chance(20)) {
            player->get_inventory().add_item({"mana_potion", "potion", 30});
            std::cout << "💧 Mana Potion!\n";
        }
    }

    void healing_fountain() {
        std::cout << "\n📖 Storyteller: \"A sacred fountain! Rest and recover...\"\n";
        std::cout << "\n⛲ Healing Fountain!\n";
        int heal = player->get_max_health() * 40 / 100 + dice.roll(10);
        player->heal(heal);
        player->restore_mana(20);
        std::cout << "✨ Restored " << heal << " HP and 20 Mana.\n";
    }

    void trap_event() {
        std::cout << "\n📖 Storyteller: \"Wait! Something's not right...\"\n";
        std::cout << "\n⚠️  Trap triggered!\n";
        int r = dice.roll(20);
        if (r <= 5) {
            std::cout << "✅ Dodged!\n";
        } else if (r <= 15) {
            int dmg = dice.roll(10) + 5;
            player->take_damage(dmg);
            std::cout << "OUCH! Took " << dmg << " damage.\n";
        } else {
            int dmg = dice.roll(20) + 15;
            player->take_damage(dmg);
            std::cout << "💥 Heavy damage: " << dmg << "!\n";
        }
    }

    void story_event() {
        int event = dice.roll(4);
        if (event == 1) {
            std::cout << "\n👴 Old traveler: \"Help me?\"\n";
            std::cout << "1. Help | 2. Refuse\n";
            if (get_choice(1, 2) == 1) {
                player->get_inventory().add_gold(25);
                player->get_inventory().add_item({"healing_potion", "potion", 30});
                std::cout << "📦 Chest: 25g + potion!\n";
            } else {
                player->get_inventory().add_gold(-10);
                std::cout << "💸 Lost 10 gold.\n";
            }
        } else if (event == 2) {
            if (player->get_inventory().has_item("healing_potion")) {
                std::cout << "\n🐺 Wounded wolf. Heal? (1=yes, 2=no)\n";
                if (get_choice(1, 2) == 1) {
                    auto err = player->get_inventory().use_item("healing_potion", *player);
                    if (err) std::cout << *err << "\n";
                    player->get_inventory().add_gold(15);
                    std::cout << "🐾 Wolf blesses you: +15g!\n";
                }
            }
        } else if (event == 3) {
            if (player->get_inventory().get_gold() >= 10) {
                std::cout << "\n🔮 Shrine: Sacrifice 10g? (1=yes 2=no)\n";
                if (get_choice(1, 2) == 1) {
                    player->get_inventory().add_gold(-10);
                    player->heal(20);
                    player->restore_mana(20);
                    std::cout << "✨ Blessed: +20 HP, +20 Mana!\n";
                }
            }
        } else {
            std::cout << "\n⚔️ Cursed sword (+5 ATK). Take? (1=yes 2=no)\n";
            if (get_choice(1, 2) == 1) {
                // direct stat change; in real project prefer equipment system
                // note: attack is protected member so we cast
                // We'll use a lambda to increase attack (not ideal design but simple)
                struct Hack : Player {
                    using Player::attack;
                };
                // Ugly but simple: use dynamic_cast to access attack
                // Instead we'll provide an exposed method normally; for now do simple hack:
                // (Since attack is protected in Character, but we are in GameEngine scope,
                // we cannot access it. So instead, print and store buff as "temp buff" via item.)
                player->get_inventory().add_item({"cursed_sword_plus5", "weapon", 5});
                std::cout << "⚡ You picked up a cursed sword (+5 ATK stored as item). Use a proper equip step to apply.\n";
            }
        }
    }

    void generate_random_event() {
        ++turns;
        int r = dice.roll(100);
        if (r <= 40) {
            auto enemy = spawn_random_enemy();
            battle(enemy);
        } else if (r <= 65) {
            treasure_room();
        } else if (r <= 80) {
            healing_fountain();
        } else if (r <= 90) {
            trap_event();
        } else {
            story_event();
        }
    }

    void game_loop() {
        std::cout << "\n� Storyteller: \"And so, your tale begins in the Upside Down...\"\n";
        std::cout << "\n�🚀 Your journey into the Upside Down begins...\n";
        while (player->is_alive() && !dragon_defeated) {
            std::cout << "\n-----------------------------\n";
            std::cout << " Turn " << (turns + 1) << '\n';
            player->print_stats();
            std::cout << "💰 Gold: " << player->get_inventory().get_gold() << '\n';
            std::cout << "Press Enter to continue...";
            std::cin.get();
            generate_random_event();
        }

        if (dragon_defeated) {
            std::cout << "\n========================================\n";
            std::cout << "📖 Storyteller: \"INCREDIBLE! You have done the impossible!\"\n";
            std::cout << " VICTORY - YOU DEFEATED THE MIND FLAYER!\n";
            std::cout << " Hawkins is safe! The Upside Down is sealed!\n";
            std::cout << "📖 Storyteller: \"Your legend will be told for generations!\"\n";
        } else {
            std::cout << "\n========================================\n";
            std::cout << "📖 Storyteller: \"Alas... even heroes fall...\"\n";
            std::cout << " GAME OVER - The Upside Down consumed you.\n";
            std::cout << "📖 Storyteller: \"But fear not, for every end is a new beginning...\"\n";
        }
    }

public:
    void run() {
        while (true) {
            show_main_menu();
            int choice = get_choice(1, 2);
            if (choice == 2) {
                std::cout << "📖 Storyteller: \"Farewell, brave soul. Until we meet again!\"\n";
                std::cout << "👋 Farewell, hero!\n";
                break;
            }

            show_class_selection();
            int cls = get_choice(1, 5);  // 5 classes now
            initialize_player(cls);
            game_loop();

            if (!ask_yes_no("\nPlay again?")) {
                std::cout << "📖 Storyteller: \"May your path be filled with adventure!\"\n";
                std::cout << "Thanks for playing! 🎮\n";
                break;
            }
            player.reset();
            turns = 0;
            dragon_defeated = false;
        }
    }
};

// ---------------------- main ----------------------
int main() {
    using namespace std;
    using namespace std::chrono;

    // Convert system_clock::now() to time_t
    auto now = system_clock::now();
    time_t t = system_clock::to_time_t(now);

    // Convert to tm struct (human readable time)
    tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &t);
#else
    localTime = *localtime(&t);
#endif

    cout << "🎮 STRANGER THINGS: The Upside Down RPG ("
         << (1900 + localTime.tm_year) << '-'
         << setw(2) << setfill('0') << (localTime.tm_mon + 1) << '-'
         << setw(2) << setfill('0') << localTime.tm_mday
         << ")\n";
    cout << "📖 Storyteller: \"Welcome, traveler, to a world of magic and mystery...\"\n\n";

    GameEngine engine;
    engine.run();
    
    cout << "\n📖 Storyteller: \"And thus, another tale comes to an end...\"\n";

    return 0;
}
