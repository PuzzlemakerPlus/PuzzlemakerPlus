// main_window.cpp
// Implements the main window class for the editor as a subclass of QMainWindow
#include "main_window.h"

#include <qrandom.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qdesktopservices.h>

#include "../util/debug.h"
#include "palette.h"

namespace PMP
{
	MainWindow::MainWindow(QWidget* parent /* = nullptr */)
		: QMainWindow(parent),
		m_mainViewport(this)
	{
		// easter egg
		if ( QRandomGenerator::global()->generate() % 10000 == 0 )
			setWindowTitle("Experimental Chamber Convolver Plus");
		else
			setWindowTitle("Puzzlemaker Plus");

		// TODO: save window size across restarts
		resize(1280, 720);

		// Initialize debugger singleton with the main viewport
		PMP::Debugger::instance(&m_mainViewport);
		m_mainViewport.setSelector(&m_selector);
		setCentralWidget(&m_mainViewport);

		newChamber();

		// Set up menu bar
		auto* fileMenu = menuBar()->addMenu("File");
		fileMenu->addAction("New chamber", [this]{
			newChamber();
		});
		fileMenu->addAction("Open")->setEnabled(false);
		fileMenu->addAction("Save")->setEnabled(false);
		fileMenu->addAction("Save as")->setEnabled(false);
		fileMenu->addAction("Import puzzlemaker map")->setEnabled(false);
		fileMenu->addSeparator();
		fileMenu->addAction("Launch game")->setEnabled(false);
		fileMenu->addAction("Rebuild")->setEnabled(false);
		fileMenu->addAction("Publish")->setEnabled(false);
		fileMenu->addSeparator();
		fileMenu->addAction("Settings")->setEnabled(false);
		fileMenu->addAction( "Exit editor", [this] {
			close();
		});

		auto* editMenu = menuBar()->addMenu("Edit");
		editMenu->addAction("Undo")->setEnabled(false);
		editMenu->addAction("Redo")->setEnabled(false);
		editMenu->addAction("Select all")->setEnabled(false);

		auto* mapMenu = menuBar()->addMenu("Map");
		mapMenu->addAction("Change style")->setEnabled(false);
		mapMenu->addSeparator();
		// NOTE: These are placeholders for mockup purposes - eventually these settings should be defined in packages
		auto* coopAction = mapMenu->addAction("Cooperative puzzle");
		coopAction->setCheckable(true);
		auto* multiCaveAction = mapMenu->addAction("Multiverse cave");
		multiCaveAction->setCheckable(true);
		multiCaveAction->setChecked(true);

		auto* helpMenu = menuBar()->addMenu("Help");
		helpMenu->addAction("Check for updates")->setEnabled(false);
		helpMenu->addAction( "Report a bug", [this] {
			QDesktopServices::openUrl( { "https://github.com/PuzzlemakerPlus/PuzzlemakerPlus/issues/new" } );
		});
		helpMenu->addSeparator();
		helpMenu->addAction("About Puzzlemaker Plus")->setEnabled(false);
		helpMenu->addAction("About Qt", [this] {
			QMessageBox::aboutQt(this);
		});

		//creates all the buttons for the palette Right now most of the data for each element is junk for demo
		// Disabling this for now until we're ready to work on it more - Kelsey
		//PMP::Palette Palette(&m_mainViewport);
	}

	void MainWindow::newChamber()
	{
		// unload previous chamber
		m_currentChamber.reset();
		m_currentChamber = std::make_unique<TestChamber>();
		m_currentChamber->printNodesTree();
		m_currentChamber->generateSurfaces();
		std::cout << "New Test Chamber" << std::endl;

		// does this totally defeat the purpose of a unique_ptr idk
		m_selector.setTestChamber(m_currentChamber.get());
		m_mainViewport.setTestChamber(m_currentChamber.get());
		m_mainViewport.loadTestChamberMesh();
	}
}