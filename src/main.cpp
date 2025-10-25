// main.cpp
// Defines the entry point for the application

#include <QApplication>
#include <iostream>
#include "undo.h"
#include "selection.h"
#include "test_chamber.h"
#include "widgets/viewport.h"
#include "widgets/palette.h"
#include "util/debug.h"

int main(int argc, char* argv[])
{
	PMP::UndoStack::instance(); //Initialize Undostack singleton before anything else
	QApplication a(argc, argv);
	// Configure OpenGL format
	QSurfaceFormat format;
	//QSurfaceFormat::setDefaultFormat(format);
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setVersion(3,3);
	format.setSwapInterval(0);
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	format.setProfile(QSurfaceFormat::CoreProfile);
#ifndef NDEBUG
	format.setOption(QSurfaceFormat::DebugContext);
#else
	format.setOption(QSurfaceFormat::DebugContext, false);
#endif
	
	QSurfaceFormat::setDefaultFormat( format );

	// TODO: We probably want the main viewport to be a child of a separate "base" panel
	PMP::Viewport mainViewport(nullptr);
	// Initialize debugger singleton with the main viewport
	PMP::Debugger::instance(&mainViewport);
	PMP::Selector selector;
	mainViewport.setSelector(&selector);
	mainViewport.resize(1280, 720);
	mainViewport.setWindowTitle("Puzzlemaker Plus");

	mainViewport.show();
	
	PMP::TestChamber tc;
	tc.createChildrenOfNode(0);
	tc.createChildrenOfNode(6);
	tc.createChildrenOfNode(2);
	tc.setNodeActive(3, false);
	//tc.SetNodeActive(13, false);
	
	tc.createChildrenOfNode(24);
	tc.setNodeActive(31, false);
	tc.setNodeActive(30, false);
	tc.setNodeActive(29, false);

	tc.printNodesTree();
	tc.generateSurfaces();
	std::cout << "Loaded Test Chamber" << std::endl;
	
	selector.setTestChamber(&tc);
	mainViewport.setTestChamber(&tc);
	mainViewport.loadTestChamberMesh();

	//creates all the buttons for the palette Right now most of the data for each element is junk for demo
	PMP::Palette Palette(&mainViewport);

	return QApplication::exec();
}
