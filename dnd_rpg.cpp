// dnd_rpg.cpp
// Text-based D&D-style RPG (C++20-friendly, no <print>/<expected>)
// Compile with: g++ -std=c++20 dnd_rpg.cpp -O2 -o dnd_rpg

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

// ---------------------- Dice ----------------------
class Dice {
    std::mt19937 engine;

public:
    Dice() : engine((std::random_device{})()) {}

    int roll(int sides) {
        if (sides <= 1) return 1;
        std::uniform_int_distribution<int> dist(1, sides);
        return dist(engine);
    }

    bool chance(int percent) {
        return roll(100) <= percent;
    }
};

// ---------------------- Item ----------------------
struct Item {
    std::string name;
    std::string type; // "potion", "weapon", "armor", ...
    int effect = 0;
};

// Forward declare Player
class Player;

// ---------------------- Inventory ----------------------
class Inventory {
    std::vector<Item> items;
    int gold = 0;

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

// ---------------------- Concrete Players ----------------------
class Warrior : public Player {
public:
    Warrior() : Player("Warrior", 120, 20, 15) {
        inventory.add_item({"healing_potion", "potion", 30});
        inventory.add_item({"healing_potion", "potion", 30});
        inventory.add_gold(20);
    }

    void special_move(Character &target) override {
        Dice dice;
        int roll = dice.roll(20);
        int total_attack = roll + attack;
        int dmg = static_cast<int>(std::max(0, (total_attack - target.get_defense())) * 1.5);
        target.take_damage(dmg);
        std::cout << "🛡️  Warrior used SHIELD BASH! Dealt " << dmg << " damage!\n";
    }
};

class Mage : public Player {
public:
    Mage() : Player("Mage", 80, 25, 8) {
        inventory.add_item({"healing_potion", "potion", 20});
        inventory.add_item({"mana_potion", "potion", 30});
        inventory.add_gold(30);
    }

    void special_move(Character &target) override {
        constexpr int COST = 30;
        if (mana < COST) {
            std::cout << "❌ Not enough mana! (" << mana << "/" << COST << ")\n";
            return;
        }
        spend_mana(COST);
        Dice dice;
        int roll = dice.roll(20);
        int total_attack = roll + attack + 10;
        int dmg = std::max(0, total_attack - target.get_defense());
        target.take_damage(dmg);
        std::cout << "✨ Mage cast FIREBALL! Dealt " << dmg << " damage!\n";
    }
};

class Rogue : public Player {
public:
    Rogue() : Player("Rogue", 90, 22, 10) {
        inventory.add_item({"healing_potion", "potion", 25});
        inventory.add_gold(40);
    }

    void special_move(Character &target) override {
        Dice dice;
        int roll = dice.roll(20);
        bool crit = dice.chance(25); // realistic crit chance
        int base_dmg = std::max(0, (roll + attack) - target.get_defense());
        int dmg = crit ? static_cast<int>(base_dmg * 2.5) : base_dmg;
        target.take_damage(dmg);
        if (crit)
            std::cout << "🗡️  Rogue used BACKSTAB! CRITICAL HIT! Dealt " << dmg << " damage!\n";
        else
            std::cout << "🗡️  Rogue used BACKSTAB! Dealt " << dmg << " damage!\n";
    }
};

class Barbarian : public Player {
public:
    Barbarian() : Player("Barbarian", 140, 28, 12) {
        inventory.add_item({"healing_potion", "potion", 40});
        inventory.add_gold(10);
        rage = 20;
    }

    void special_move(Character &target) override {
        int missing_hp = max_health - health;
        int rage_bonus = missing_hp / 10;
        Dice dice;
        int roll = dice.roll(20);
        int total_attack = roll + attack + rage_bonus;
        int dmg = std::max(0, total_attack - target.get_defense());
        target.take_damage(dmg);
        add_to_rage(15);
        std::cout << "😡 Barbarian used RAGE STRIKE! Dealt " << dmg << " damage (+"
                  << rage_bonus << " from rage)!\n";
    }
};

// ---------------------- Enemy ----------------------
class Enemy : public Character {
    bool is_boss_ = false;

public:
    Enemy(std::string n, int hp, int atk, int def) : Character(std::move(n), hp, atk, def) {}
    virtual ~Enemy() = default;

    void set_is_boss(bool b) noexcept { is_boss_ = b; }
    bool is_boss() const noexcept { return is_boss_; }

    void attack_move(Character &target) override {
        Dice dice;
        int roll = dice.roll(20);
        int base_dmg = std::max(0, (roll + attack) - target.get_defense());
        int fire_dmg = dice.chance(30) ? 15 : 0;
        target.take_damage(base_dmg + fire_dmg);
        if (fire_dmg > 0) std::cout << "🔥 " << name << " breathes fire!\n";
    }

    void special_move(Character &) override { /* default no-op */ }
};

class Goblin : public Enemy {
public:
    Goblin() : Enemy("Goblin", 30, 12, 5) {}
};
class Orc : public Enemy {
public:
    Orc() : Enemy("Orc", 60, 18, 8) {}
};
class Troll : public Enemy {
public:
    Troll() : Enemy("Troll", 100, 22, 10) {}
};
class Dragon : public Enemy {
public:
    Dragon() : Enemy("Ancient Dragon", 200, 30, 15) { set_is_boss(true); }
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
        std::cout << "⚔️  DUNGEONS & DRAGONS RPG (Terminal) ⚔️\n";
        std::cout << "========================================\n";
        std::cout << "1. Start Game\n2. Exit\n";
        std::cout << "Choose an option: ";
    }

    void show_class_selection() {
        std::cout << "\nChoose your hero:\n";
        std::cout << "1. Warrior  (Tank)\n";
        std::cout << "2. Mage     (Burst)\n";
        std::cout << "3. Rogue    (Crit)\n";
        std::cout << "4. Barbarian(Rage)\n";
        std::cout << "\nYour choice: ";
    }

    void initialize_player(int choice) {
        switch (choice) {
        case 1: player = std::make_unique<Warrior>(); break;
        case 2: player = std::make_unique<Mage>(); break;
        case 3: player = std::make_unique<Rogue>(); break;
        case 4: player = std::make_unique<Barbarian>(); break;
        default: player = std::make_unique<Warrior>(); break;
        }
        std::cout << "\n🌟 You are a brave " << player->get_name() << "!\n";
        player->print_full_stats();
        std::cout << "Starting gold: " << player->get_inventory().get_gold() << "\n";
    }

    std::unique_ptr<Enemy> spawn_random_enemy() {
        if (turns >= 20 && !dragon_defeated) {
            std::cout << "\n🌪️  The sky darkens... A DRAGON DESCENDS!\n";
            return std::make_unique<Dragon>();
        }

        int r = dice.roll(100);
        if (r <= 40) return std::make_unique<Goblin>();
        if (r <= 70) return std::make_unique<Orc>();
        if (r <= 95) return std::make_unique<Troll>();
        return std::make_unique<Dragon>();
    }

    void battle(std::unique_ptr<Enemy> &enemy) {
        std::cout << "\n========================================\n";
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
                // small stun mechanic for warrior special
                if (dynamic_cast<Warrior *>(player.get()) && dice.chance(25)) {
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
        std::cout << "\n⛲ Healing Fountain!\n";
        int heal = player->get_max_health() * 40 / 100 + dice.roll(10);
        player->heal(heal);
        player->restore_mana(20);
        std::cout << "✨ Restored " << heal << " HP and 20 Mana.\n";
    }

    void trap_event() {
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
        std::cout << "\n🚀 Adventure begins...\n";
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
            std::cout << " VICTORY - YOU SLAYED THE DRAGON!\n";
            std::cout << " The realm is saved by your valor!\n";
        } else {
            std::cout << "\n========================================\n";
            std::cout << " GAME OVER - Your legend ends here.\n";
        }
    }

public:
    void run() {
        while (true) {
            show_main_menu();
            int choice = get_choice(1, 2);
            if (choice == 2) {
                std::cout << "👋 Farewell, hero!\n";
                break;
            }

            show_class_selection();
            int cls = get_choice(1, 4);
            initialize_player(cls);
            game_loop();

            if (!ask_yes_no("\nPlay again?")) {
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

    cout << "🎮 D&D RPG — Terminal Edition ("
         << (1900 + localTime.tm_year) << '-'
         << setw(2) << setfill('0') << (localTime.tm_mon + 1) << '-'
         << setw(2) << setfill('0') << localTime.tm_mday
         << ")\n";

    GameEngine engine;
    engine.run();

    return 0;
}
