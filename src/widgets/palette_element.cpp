//
// Created by hannah on 13/04/25.
//

#include "palette_element.h"

#include <QPushButton>
#include <iostream>

void PMP::PaletteElement::handleClick()
{
	std::cout << "Clicked on item number " << this->m_id << std::endl;
}
