// main.cpp
// Defines the entry point for the application

#include <QApplication>
#include "widgets/viewport.h"

#include "widgets/palette.h"
#include "widgets/PaletteElement.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
//    QPushButton button("Hello world!", nullptr);
    QPushButton button("This is the first element in the palette");
    button.resize(400, 200);
    button.show();

    pmp::PaletteElement* firstElement;
    QObject::connect(&button, SIGNAL(clicked(bool)), firstElement, SLOT(handleClick()));


    // TODO: We probably want the main viewport to be a child of a separate "base" panel
    PMP::Viewport mainViewport(nullptr);
    mainViewport.resize(1280, 720);
    mainViewport.setWindowTitle("Puzzlemaker Plus");

    // Configure OpenGL format
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3,3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    mainViewport.setFormat( format );

    mainViewport.show();

    return QApplication::exec();
}
