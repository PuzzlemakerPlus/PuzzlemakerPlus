//
// Created by hannah on 13/04/25.
//

#ifndef PUZZLEMAKERPLUS_PALETTEELEMENT_H
#define PUZZLEMAKERPLUS_PALETTEELEMENT_H


#include <QAbstractButton>

namespace pmp {
    class PaletteElement : public QWidget {

    Q_OBJECT

    public:
        PaletteElement();

    public slots:
        void handleClick();
    };
}
#endif //PUZZLEMAKERPLUS_PALETTEELEMENT_H
