#ifndef PLAYER_H
#define PLAYER_H

#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <cstdio>
#include <string>

#define health_pen 5
#define max_health 100


enum PlayerState {
    Idle,
    Walking,
    Jumping,
    Falling,
    Hurt,
    Dead,
    Running
};

struct InventoryItem {
    std::string name;
    int quantity;
    std::vector<std::string> dialogue;

    InventoryItem(const std::string &name, int quantity,
                  const std::vector<std::string> &dialogue = {})
        : name(name), quantity(quantity), dialogue(dialogue) {}
};

struct Inventory {
    std::vector<InventoryItem> items;
    void addItem(const std::string &name, int quantity,
                 const std::vector<std::string> &dialogue = {});
    void removeItem(const std::string &name,int quantity);
    int getItemQuantity(const std::string &name) const;
};

struct Character {
    float x;
    float y;
    int health;
    Inventory inventory; // Each character has an inventory
    int isPlayer; // 1 for player, 0 otherwise
    PlayerState state;
    bool facingRight;
    std::string path; // Path to the character's sprite sheet

    Character(float x, float y, int isPlayer, const std::string &path) : x(x), y(y), health(max_health), isPlayer(isPlayer), state(Idle), facingRight(true), path(path) {};

    void updateCharacterState(Character &character, PlayerState newState);

    void updateCharacterPosition(Character &character, float newX, float newY);

    void updateCharacterHealth(Character &character, int healthChange);

};

#endif          