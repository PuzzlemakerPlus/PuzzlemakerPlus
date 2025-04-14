#include <QApplication>
#include <QPushButton>

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

    return QApplication::exec();
}
