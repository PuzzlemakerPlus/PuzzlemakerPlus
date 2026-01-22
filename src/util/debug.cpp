// debug.cpp
// Provides functionality for drawing 3D debug overlays
#include "debug.h"

namespace PMP
{
Debugger& Debugger::instance(Viewport* vp)
{
	static Debugger instance;
	if (!instance.hasViewport())
	{
		if (vp)
			instance.setViewport(vp);
		else
			throw std::runtime_error("Debugger instance not initialized with viewport. Please call "
			                         "Debugger::instance(Viewport*) at least once");
	}
	return instance;
}

void Debugger::drawPoint(QVector3D point)
{
	// This is safe because we check for viewport existence in instance()
	m_viewport->loadDots({{point, {1.0, 0, 0}}});
}

void Debugger::drawPoints(std::vector<QVector3D> points)
{
	// This is safe because we check for viewport existence in instance()
	std::vector<DebugPoint> dp;
	for (int i = 0; i < points.size(); i++)
	{
		dp.push_back({points[i], {1.0, 0, 0}});
	}
	m_viewport->loadDots(dp);
}

void Debugger::addDebugPoint(QVector3D point, QVector3D color)
{
	m_debugPoints.push_back({point, color});
}

void Debugger::drawDebugPoints()
{
	if (m_debugPoints.empty())
		return;
	m_viewport->loadDots(m_debugPoints);
}

void Debugger::clearDebugPoints()
{
	m_debugPoints.clear();
}
} // namespace PMP
