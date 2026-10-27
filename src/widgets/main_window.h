// main_window.h
// Declares the main window class for the editor as a subclass of QMainWindow
#pragma once

#include <memory>
#include <qaction.h>
#include <qmainwindow.h>
#include <qwidget.h>

#include "../package/package_loader.h"
#include "../selection.h"
#include "../test_chamber.h"
#include "viewport.h"

namespace PMP
{
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);

	void newChamber();
	// TODO: loadChamber etc.

	// read access to package loader, this is kinda dumb
	[[nodiscard]] const PackageLoader* packageLoader() const { return m_packageLoader.get(); }

private:
	Selector m_selector;
	Viewport* m_mainViewport;

	std::unique_ptr<PackageLoader> m_packageLoader;

	std::unique_ptr<TestChamber> m_currentChamber;
};
} // namespace PMP
