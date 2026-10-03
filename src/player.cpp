#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <cstdio>
#include <player.h>


void Character::updateCharacterState(Character &character, PlayerState newState) {
    character.state = newState;
}

void Character::updateCharacterPosition(Character &character, float newX, float newY) {
    character.x = newX;
    character.y = newY;
}

void Character::updateCharacterHealth(Character &character, int healthChange){
    character.health -= healthChange;
    if (character.health <= 0) {
        character.health = 0;
        character.state = Dead;
    }
}



void Inventory::addItem(const std::string &name, int quantity,
                        const std::vector<std::string> &dialogue) {
    for (auto &item : items) {
        if (item.name == name) {
            item.quantity += quantity;
            if (!dialogue.empty()) item.dialogue = dialogue;
            return;
        }
    }
    items.emplace_back(name, quantity, dialogue);
}

void Inventory::removeItem(const std::string &name, int quantity) {
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->name == name) {
            it->quantity -= quantity;
            if (it->quantity <= 0) {
                items.erase(it);
            }
            return;
        }
    }
}

int Inventory::getItemQuantity(const std::string &name) const {
    for (const auto &item : items) {
        if (item.name == name) {
            return item.quantity;
        }
    }
    return 0;
}