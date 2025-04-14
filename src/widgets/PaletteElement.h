//
// Created by hannah on 13/04/25.
//

#ifndef PUZZLEMAKERPLUS_PALETTEELEMENT_H
#define PUZZLEMAKERPLUS_PALETTEELEMENT_H

#include <QAbstractButton>

namespace PMP {
    class PaletteElement : public QWidget {

    Q_OBJECT

    public slots:
        void handleClick();

    public:
        int m_id;

        explicit PaletteElement(int id) : m_id(id) {};

    private:
        int id{};
    };
}
#endif //PUZZLEMAKERPLUS_PALETTEELEMENT_H
