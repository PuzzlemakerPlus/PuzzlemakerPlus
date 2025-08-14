#pragma once
#include <QVector3D>
#include "../widgets/viewport.h"
namespace PMP
{
    // Singleton Debugger that displays on viewport
    class Debugger
    {
    public:
        static Debugger& instance(Viewport* vp = nullptr);
        int hasViewport() const { return m_viewport != nullptr; };
        // Set the viewport for the debugger. Must be set before using the debugger.
        void setViewport(Viewport* vp) { m_viewport = vp; };
        void drawPoint(QVector3D point);
        void drawPoints(std::vector<QVector3D> points);
        // Add a point to the debug points list, which can be drawn later
        void addDebugPoint(QVector3D point);
        // Draw all debug points that have been added
        void drawDebugPoints();
    private:
        Debugger() = default;
        Debugger(const Debugger&) = delete;
        Viewport* m_viewport = nullptr;
        std::vector<QVector3D> m_debugPoints;
    };
}