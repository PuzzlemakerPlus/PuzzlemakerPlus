#include <iostream>
#include "../src/test_chamber.h"

int main(){
    std::cout << "Running OctTree Test" << std::endl;
    PMP::TestChamber tc;
    tc.PrintNodes();
    //tc.CreateChildrenForNode(0);
    tc.PrintNodes();
    //tc.CreateChildrenForNode(1);
    tc.PrintNodes();
    return 0;
}
