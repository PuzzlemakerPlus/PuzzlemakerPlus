//
// Created by hannah on 13/04/25.
//

#include <QPushButton>
#include <iostream>
#include "PaletteElement.h"

void PMP::PaletteElement::handleClick() {
    std::cout << "Clicked on item number " << this->m_id << std::endl;
}
