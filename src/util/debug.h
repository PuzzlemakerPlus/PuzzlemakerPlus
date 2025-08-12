#pragma once
#include "../widgets/viewport.h"
#include <QVector3D>
namespace PMP
{
    
    // Singleton Debugger that displays on viewport
    class Debugger
    {
    public:
        static Debugger& instance(Viewport* vp = nullptr);
        void setViewport(Viewport* vp) { m_viewport = vp; };
        int hasViewport() const { return m_viewport != nullptr; };
        void drawPoint(QVector3D point);
        void drawPoints(std::vector<QVector3D> points);
        void addDebugPoint(QVector3D point);
        void drawDebugPoints();
    private:
        Debugger() = default;
        Debugger(const Debugger&) = delete;
        Viewport* m_viewport = nullptr;
        std::vector<QVector3D> m_debugPoints;
    };
}