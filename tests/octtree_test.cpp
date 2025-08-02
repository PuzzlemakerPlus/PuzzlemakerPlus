#include <iostream>
#include "../src/test_chamber.h"

template<std::size_t N>
void printArray(const std::array<PMP::NodeID, N>& arr) {
    std::cout << "[";
    for (std::size_t i = 0; i < N; ++i) {
        std::cout << arr[i];
        if (i != N - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}

void printFreeID(PMP::TestChamber& tc)
    {
        std::array<PMP::NodeID,8> free;
        tc.FindFreeIDs(free);
        printArray<8>(free);
    }

void addChildNodesAndPrint(PMP::TestChamber& tc, PMP::NodeID parentID)
    {
        tc.CreateChildrenForNode(parentID);
        std::cout << "Added Children to Node " << parentID << std::endl;
        tc.PrintNodesTree();
    }

void removeChildNodesAndPrint(PMP::TestChamber& tc, PMP::NodeID parentID)
    {
        tc.DestroyChildrenForNode(parentID);
        std::cout << "Removed Children of Node " << parentID << std::endl;
        tc.PrintNodesTree();
    }

int main(){
    std::cout << "Running OctTree Test" << std::endl;
    PMP::TestChamber tc;
    tc.PrintNodesTree();

    addChildNodesAndPrint(tc, 0);

    printFreeID(tc);

    addChildNodesAndPrint(tc, 1);

    printFreeID(tc);

    addChildNodesAndPrint(tc, 14);

    printFreeID(tc);

    removeChildNodesAndPrint(tc, 14);

    printFreeID(tc);

    removeChildNodesAndPrint(tc, 14);

    printFreeID(tc);

    addChildNodesAndPrint(tc, 15);

    printFreeID(tc);

    return 0;
}
