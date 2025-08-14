#include <random>
#include <chrono>
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
        tc.createChildrenOfNode(parentID);
        std::cout << "Added Children to Node " << parentID << std::endl;
        tc.printNodesTree();
    }

void removeChildNodesAndPrint(PMP::TestChamber& tc, PMP::NodeID parentID)
    {
        tc.destroyChildrenOfNode(parentID);
        std::cout << "Removed Children of Node " << parentID << std::endl;
        tc.printNodesTree();
    }


void RunRandomNodeSearches(PMP::TestChamber& tc, int numTests = 5)
{
    const int size = tc.GetRootNodeSizeInt();
    const int minBound = -size / 2;
    const int maxBound = size / 2;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(minBound, maxBound);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numTests; ++i) {
        float x = dist(gen);
        float y = dist(gen);
        float z = dist(gen);

        PMP::NodeID id = tc.searchNodeWithCoord({x, y, z});
        std::cout << "Found Node that contains (" << x << "," << y << "," << z << "): " << id << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Total time for " << numTests << " queries: "
              << duration.count() << " microseconds ("
              << duration.count() / 1000.0 << " ms)" << std::endl;
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

	tc.testChildNodeGeneration();

	RunRandomNodeSearches(tc, 100);
    return 0;
}
