#include "omega_front_infrastructure.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;

static_assert(InfrastructureScope::UsesHearthwellPump(0), "pump defense is local");
static_assert(InfrastructureScope::UsesHearthwellPump(2), "receivership is local");
static_assert(!InfrastructureScope::UsesHearthwellPump(1), "orchard is remote");
static_assert(!InfrastructureScope::UsesHearthwellPump(3), "salvage yard is remote");
static_assert(!InfrastructureScope::UsesHearthwellPump(4), "charter gate is remote");
static_assert(!InfrastructureScope::UsesHearthwellPump(5), "caravan ford is remote");

int main() {
    for (int operation = 0; operation < 6; ++operation) {
        const bool local = operation == 0 || operation == 2;
        for (int pump = 1; pump <= 18; ++pump) {
            const int site = InfrastructureScope::BattleSiteHealth(operation, pump);
            assert(site == (local ? pump : 18));
            assert(InfrastructureScope::UsesHearthwellPump(operation) == local);
        }
    }
    // Future operations default to their own installation, not an accidental
    // mutation of the founding settlement's unique waterworks.
    assert(!InfrastructureScope::UsesHearthwellPump(6));
    assert(!InfrastructureScope::UsesHearthwellPump(-1));
    assert(InfrastructureScope::BattleSiteHealth(6, 4) == 18);
    std::cout << "Omega Front infrastructure ownership regression passed\n";
}
