// main.cpp
// Defines the entry point for the application

#include <QApplication>
#include <iostream>

#include "absl/strings/str_format.h"

#include "widgets/main_window.h"

#include "undo.h"
#include "selection.h"
#include "test_chamber.h"
#include "widgets/viewport.h"
#include "widgets/palette.h"
#include "util/debug.h"

int main(int argc, char* argv[])
{
	//Initialize Undostack singleton before anything else
	// NOTE: This will need to be made per-chamber if we implement tabbed UI later
	PMP::UndoStack::instance();

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

	QApplication app(argc, argv);

	auto* mainWindow = new PMP::MainWindow;
	mainWindow->show();

	return app.exec();
}
