#pragma once
#include "util/3d/mesh.h"
#include "test_chamber.h"

namespace PMP
{
    class TestChamberQuads
    {
        public:
            TestChamberQuads(TestChamber& tc) : testChamber(tc) {};
            void generateQuads();
            const std::vector<Quad>& GetQuads() {return quads;};
        private:
            TestChamber& testChamber;
            std::vector<Quad> quads;
    };
}