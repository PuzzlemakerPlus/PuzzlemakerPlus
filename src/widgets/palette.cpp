//
// Created by hannah on 13/04/25.
//

#include <QPushButton>
#include "palette.h"

PMP::Palette::Palette(QWidget* parent)
{

	//instantiate member variables
	this->m_paletteElements = std::vector<PMP::PaletteElement*>();

	//TODO: Grab in elements of the Palette from a file elsewhere in code 10 Items is a random number to demo
	for(int i = 0; i < 10; i++) 
	{

		//TODO: Eventually PMP::PaletteElement is probably going to be a custom implementation of QPushButton making this obsolete
		auto* text = new QString("Element ");
		text->append(std::to_string(i));
		auto* button = new QPushButton(*text, parent);
		//randomly picked size for now
		button->resize(75, 75);
		//x: every other item move width to the right y: move height down every 2 items
		//currently we have rows of 2 items this is subject to change
		button->move((i % 2) * 75, i / 2 * 75);

	   this->addItem(new PMP::PaletteElement(i));

	   //sign up our paletteElement to listen to click events on it's handleClick() function
	   QObject::connect(button, SIGNAL(clicked(bool)),
						this->getItem(i), SLOT(handleClick()));

		button->show();
	}
}

void PMP::Palette::addItem(PMP::PaletteElement* item) 
{
	this->m_paletteElements.push_back(item);
}

void PMP::Palette::addItems(std::vector<PMP::PaletteElement*>& newElements) 
{
	for(PMP::PaletteElement* element : newElements) 
	{
		addItem(element);
	}
}

PMP::PaletteElement* PMP::Palette::getItem(int index) 
{
	return this->m_paletteElements.at(index);
}

std::vector<PMP::PaletteElement*> PMP::Palette::getItems() 
{
	return this->m_paletteElements;
}



