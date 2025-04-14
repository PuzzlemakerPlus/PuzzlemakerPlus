//
// Created by hannah on 13/04/25.
//

#ifndef PUZZLEMAKERPLUS_PALETTE_H
#define PUZZLEMAKERPLUS_PALETTE_H


#include "PaletteElement.h"

namespace PMP {

    class Palette {

    private:
        std::vector<PMP::PaletteElement*> m_paletteElements;

    public:

        Palette();

        void addItem(const PMP::PaletteElement* item);

        PMP::PaletteElement* getItem(int index);
        std::vector<PMP::PaletteElement*> getItems();


    };
}

#endif //PUZZLEMAKERPLUS_PALETTE_H
