#include "debug.h"

namespace PMP
{
    Debugger& Debugger::instance(Viewport* vp) {
        static Debugger instance;
        if (!instance.hasViewport())
        {
            if(vp) instance.setViewport(vp);
            else throw std::runtime_error("Debugger instance not initialized with viewport. Please call Debugger::instance(Viewport*) at least once");
        }
        return instance;
    }

    void Debugger::drawPoint(QVector3D point)
    {
        // This is safe because we check for viewport existence in instance()
        m_viewport->loadDots({ point });
    }
}