// palette_element.cpp
// Widget for individual palette items
#include "palette_element.h"

#include <QPushButton>
#include <iostream>

void PMP::PaletteElement::handleClick()
{
	std::cout << "Clicked on item number " << this->m_id << std::endl;
}
