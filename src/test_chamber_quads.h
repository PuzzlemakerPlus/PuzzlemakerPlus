#pragma once
#include "util/3d/mesh.h"
#include "test_chamber.h"

namespace PMP
{
    class TestChamberQuads
    {
        public:
            TestChamberQuads(const TestChamber& tc) : testChamber(tc) {};
            const std::vector<Quad>& GetQuads() {return quads;};
            void generateQuads();
        private:
            const TestChamber& testChamber;
            std::vector<Quad> quads;
    };
}