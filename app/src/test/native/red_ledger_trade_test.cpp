#include "red_ledger_trade.h"
#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>
using namespace reverie::redledger;
int main(){
    trade::Trade t;
    assert(t.stock_drink(1)==1&&t.stock_snack(1)==1);
    assert(t.SelectDrink(6)&&t.drink()==1);
    assert(t.SelectSnack()&&t.snack()==1); // skip empty first snack
    return 0;
}
