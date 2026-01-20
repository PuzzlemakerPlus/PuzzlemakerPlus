// main_window.h
// Declares the main window class for the editor as a subclass of QMainWindow
#pragma once

#include <memory>
#include <qmainwindow.h>
#include <qwidget.h>
#include <qaction.h>

#include "viewport.h"
#include "../test_chamber.h"
#include "../selection.h"

namespace PMP
{
	class MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		explicit MainWindow(QWidget* parent = nullptr);

		void newChamber();
		// TODO: loadChamber etc.


	private:
		Selector m_selector;
		Viewport* m_mainViewport;

		std::unique_ptr<TestChamber> m_currentChamber;
	};
}