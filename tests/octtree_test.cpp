#include <iostream>
#include "../src/test_chamber.h"

int main(){
    std::cout << "Running OctTree Test" << std::endl;
    PMP::TestChamber tc;
    tc.PrintNodesTree();
    tc.CreateChildrenForNode(0);
    std::cout << "Added Children to Node 0" << std::endl;
    tc.PrintNodesTree();
    tc.CreateChildrenForNode(1);
    std::cout << "Added Children to Node 1" << std::endl;
    tc.PrintNodesTree();
    return 0;
}
