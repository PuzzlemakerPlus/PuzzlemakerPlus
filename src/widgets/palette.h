//
// Created by hannah on 13/04/25.
//

#ifndef PUZZLEMAKERPLUS_PALETTE_H
#define PUZZLEMAKERPLUS_PALETTE_H


#include "PaletteElement.h"

namespace PMP {

    class Palette : public QWidget {

    private:
        std::vector<PMP::PaletteElement*> m_paletteElements;

    public:

        explicit Palette(QWidget* parent);

        void addItem(PMP::PaletteElement* item);
        void addItems(std::vector<PMP::PaletteElement*>& newElements);

        PMP::PaletteElement* getItem(int index);
        std::vector<PMP::PaletteElement*> getItems();


    };
}

#endif //PUZZLEMAKERPLUS_PALETTE_H
