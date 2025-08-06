// main.cpp
// Defines the entry point for the application

#include <QApplication>
#include <iostream>
#include "undo.h"
#include "selection.h"
#include "test_chamber.h"
#include "widgets/viewport.h"
#include "widgets/palette.h"

int main(int argc, char* argv[])
{
    PMP::UndoStack::instance(); //Initialize Undostack singleton before anything else
    QApplication a(argc, argv);
    // Configure OpenGL format
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3,2);
    format.setSwapInterval(0);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setProfile(QSurfaceFormat::CoreProfile);
#ifndef NDEBUG
    format.setOption(QSurfaceFormat::DebugContext);
    
#endif
    QSurfaceFormat::setDefaultFormat( format );

    // TODO: We probably want the main viewport to be a child of a separate "base" panel
    PMP::Viewport mainViewport(nullptr);
    PMP::Selector selector;
    mainViewport.setSelector(&selector);
    mainViewport.resize(1280, 720);
    mainViewport.setWindowTitle("Puzzlemaker Plus");

    mainViewport.show();
    
    PMP::TestChamber tc;
    tc.CreateChildrenForNode(0);
    tc.CreateChildrenForNode(6);
    tc.CreateChildrenForNode(2);
    tc.SetNodeActive(3, false);
    //tc.SetNodeActive(13, false);
    
    tc.CreateChildrenForNode(24);
    tc.SetNodeActive(31, false);
    tc.SetNodeActive(30, false);
    tc.SetNodeActive(29, false);

    tc.PrintNodesTree();
    tc.GenerateSurfaces();
    std::cout << "Loaded Test Chamber" << std::endl;
    
    selector.setTestChamber(&tc);
    mainViewport.setTestChamber(&tc);
    mainViewport.loadTestChamberMesh(); //Needs to be called after mainViewport.show()
    

    //creates all the buttons for the palette Right now most of the data for each element is junk for demo
    PMP::Palette Palette(&mainViewport);

    return QApplication::exec();
}
