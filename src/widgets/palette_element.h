//
// Created by hannah on 13/04/25.
//

#pragma once

#include <QAbstractButton>

namespace PMP
{
class PaletteElement : public QWidget
{

	Q_OBJECT

public slots:
	void handleClick();

public:
	int m_id;

	explicit PaletteElement(int id) : m_id(id) {};

private:
	int id{};
};
} // namespace PMP
