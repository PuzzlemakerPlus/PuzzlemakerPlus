// palette.h
// Implements the item palette/list widget
#pragma once

#include "palette_element.h"

namespace PMP
{

class Palette : public QWidget
{

private:
	std::vector<PMP::PaletteElement *> m_paletteElements;

public:
	explicit Palette(QWidget *parent);

	void addItem(PMP::PaletteElement *item);
	void addItems(std::vector<PMP::PaletteElement *> &newElements);

	PMP::PaletteElement *getItem(int index);
	std::vector<PMP::PaletteElement *> getItems();
};
} // namespace PMP
