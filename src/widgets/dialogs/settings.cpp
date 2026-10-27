// settings.cpp
// Implements the settings dialog. (Currently, this just lists loaded packages.)

#include "settings.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>

namespace PMP
{

SettingsDialog::SettingsDialog(const std::vector<std::unique_ptr<Package>>& packageList, QWidget* parent) : QDialog(parent)
{
	setWindowTitle("Settings");
	resize(720, 480);

	auto* rootLayout = new QHBoxLayout(this);
	auto* categoryList = new QListWidget(this);
	// placeholder categories for demo
	categoryList->addItem("General");
	categoryList->addItem("Packages");
	categoryList->setCurrentRow(1);
	categoryList->addItem("Graphics");
	categoryList->addItem("Keyboard/Mouse");
	categoryList->addItem("Controller");
	categoryList->addItem("Online");
	categoryList->addItem("Game/Mod Setup");
	categoryList->addItem("Development");
	// make it a bit wider than the longest title
	categoryList->setFixedWidth(categoryList->sizeHintForColumn(0) + 2 * categoryList->frameWidth());
	categoryList->setMinimumHeight(categoryList->sizeHintForRow(0) * categoryList->count() + 2 * categoryList->frameWidth());
	rootLayout->addWidget(categoryList);

	// Layout specific to the package list page - will probably be moved later
	auto* pageLayout = new QVBoxLayout(this);
	rootLayout->addLayout(pageLayout);

	auto* packList = new QTableWidget(packageList.size(), 3, this);
	packList->verticalHeader()->hide();
	packList->setSelectionBehavior(QAbstractItemView::SelectRows);
	packList->setSelectionMode(QAbstractItemView::SingleSelection);
	packList->setEditTriggers(QAbstractItemView::NoEditTriggers);

	// column headers
	packList->setHorizontalHeaderLabels({"Name", "Author", "Description"});
	packList->setColumnWidth(0, 120);
	packList->setColumnWidth(1, 120);
	packList->setColumnWidth(2, 320);

	for (int i = 0; i < packageList.size(); i++)
	{
		packList->setItem(i, 0, new QTableWidgetItem(packageList[i]->getName().data()));
		packList->setItem(i, 1, new QTableWidgetItem(packageList[i]->getAuthor().data()));
		packList->setItem(i, 2, new QTableWidgetItem(packageList[i]->getDescription().data()));
	}
	// have to do this after adding items
	packList->setSortingEnabled(true);
	packList->sortByColumn(0, Qt::AscendingOrder);

	pageLayout->addWidget(packList);

}

}