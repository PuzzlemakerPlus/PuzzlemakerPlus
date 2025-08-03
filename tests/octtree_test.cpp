#include <iostream>
#include <qvectornd.h>
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
    //tc.PrintNodesTree();

    addChildNodesAndPrint(tc, 0);

    addChildNodesAndPrint(tc, 1);

    addChildNodesAndPrint(tc, 2);

    addChildNodesAndPrint(tc, 14);

    addChildNodesAndPrint(tc, 19);

    removeChildNodesAndPrint(tc, 14);

    removeChildNodesAndPrint(tc, 14);

    addChildNodesAndPrint(tc, 15);

	addChildNodesAndPrint(tc, 36);

	addChildNodesAndPrint(tc, 48);

	addChildNodesAndPrint(tc, 54);

	tc.TestChildNodeGeneration();

    return 0;
}
