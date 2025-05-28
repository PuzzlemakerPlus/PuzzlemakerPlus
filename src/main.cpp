// main.cpp
// Defines the entry point for the application

#include <QApplication>
#include <cstdlib>
#include "widgets/viewport.h"

#include "widgets/palette.h"

int main(int argc, char* argv[])
{
    //HACK: Force X11 for now as Wayland's not working for some reason.
    #ifdef Q_OS_LINUX
        char env[] = "QT_QPA_PLATFORM=xcb";
        putenv(env);
    #endif

    QApplication a(argc, argv);

    // Configure OpenGL format
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3,2);
    format.setProfile(QSurfaceFormat::CoreProfile);
#ifndef NDEBUG
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    QSurfaceFormat::setDefaultFormat( format );

    // TODO: We probably want the main viewport to be a child of a separate "base" panel
    PMP::Viewport mainViewport(nullptr);
    mainViewport.resize(1280, 720);
    mainViewport.setWindowTitle("Puzzlemaker Plus");

    mainViewport.show();

    //creates all the buttons for the palette Right now most of the data for each element is junk for demo
    PMP::Palette Palette(&mainViewport);

    return QApplication::exec();
}
