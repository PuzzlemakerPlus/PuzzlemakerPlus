// viewport_control.cpp
// functions for input based events in viewport
#include "viewport.h"

#include <chrono>
#include <iostream>
#include <random>

#include "../undo.h"
#include "../util/debug.h"

namespace PMP
{
// This function gets called every 16ms
void Viewport::onFrameUpdate()
{
	bool updateGraphics = false;
	QVector2D currentMouseGlobal = QVector2D(this->mapFromGlobal(QCursor::pos()));
	QVector2D delta = currentMouseGlobal - m_lastMousePos;
	if (m_isOrbiting)
	{
		QVector3D deltaOrbitAngle = 0.2f * QVector3D(delta.y() / 2.0f, 0, delta.x() / 2.0f); // todo make this customizable
		m_cameraAngles += deltaOrbitAngle;

		// Don't allow flipping the camera
		// 0 is straight down and -180 is straight up due to the way we handle the view matrix transform right now
		if (m_cameraAngles.x() > 0.0f)
			m_cameraAngles.setX(0.0f);
		else if (m_cameraAngles.x() < -180.0f)
			m_cameraAngles.setX(-180.0f);

		m_orbitVelocity = deltaOrbitAngle;
		m_shouldOrbitInertia = true;
		updateGraphics = true;
	}

	if (m_isPanning)
	{
		QMatrix4x4 camAngle;
		camAngle.rotate(QQuaternion::fromEulerAngles(m_cameraAngles));
		camAngle = camAngle.transposed(); // Reverse the transformation

		// TODO: increase pan speed the more zooned out you are
		QVector3D moveAmount(delta.x(), -delta.y(), 0.0f);
		moveAmount = camAngle * moveAmount * 1.4f;

		m_cameraTarget += moveAmount;
		updateGraphics = true;
	}

	m_lastMousePos = currentMouseGlobal;

	if (m_shouldOrbitInertia && !m_isOrbiting)
	{
		m_cameraAngles += m_orbitVelocity;

		// Clamp again
		if (m_cameraAngles.x() > 0.0f)
			m_cameraAngles.setX(0.0f);
		else if (m_cameraAngles.x() < -180.0f)
			m_cameraAngles.setX(-180.0f);

		// Dampen inertia
		m_orbitVelocity *= 0.9f;

		// Stop when very slow
		if (m_orbitVelocity.lengthSquared() < 0.0001f)
		{
			m_orbitVelocity = QVector3D(0, 0, 0);
			m_shouldOrbitInertia = false;
		}
		updateGraphics = true;
	}
	if (updateGraphics)
	{
		update(); // Draws the frame again because the view changed
	}
}

void Viewport::keyPressEvent(QKeyEvent* event)
{
	Q_ASSERT(m_selector);
	Q_ASSERT(m_testChamber);
	bool actionPerformed = false;
	if (event->matches(QKeySequence::Undo))
	{
		m_testChamber->undoStack().undo();
		actionPerformed = true;
	}
	else if (event->matches(QKeySequence::Redo))
	{
		m_testChamber->undoStack().redo();
		actionPerformed = true;
	}
	else
	{
		switch (event->key())
		{
		case Qt::Key_F:
			m_fog = !m_fog; // toggle fog. No need to reload chamber mesh and selection
			event->accept();
			update();
			break;
		case Qt::Key_S:
			m_drawShadow = !m_drawShadow; // toggle shadows. No need to reload chamber mesh and selection
			event->accept();
			update();
			break;
		case Qt::Key_D:
			m_debugSurface = !m_debugSurface; // toggle debug mode. No need to reload chamber mesh and selection
			event->accept();
			update();
			break;
		case Qt::Key_Equal: // Extrude surface
			actionPerformed = m_testChamber->extrudeSurfaces(m_selector);
			break;
		case Qt::Key_Minus: // Recede surface
			actionPerformed = m_testChamber->recedeSurfaces(m_selector);
			break;
			// Grid snap adjustment
		case Qt::Key_BracketLeft:
			m_testChamber->decreaseGridSize();
			updateGridSizeLabel();
			break;
		case Qt::Key_BracketRight:
			m_testChamber->increaseGridSize();
			updateGridSizeLabel();
			break;
		case Qt::Key_Escape:
			m_selector->clearSelection();
			actionPerformed = true;
			break;
		case Qt::Key_F5: // force visual reload for debugging
			actionPerformed = true;
			break;
		case Qt::Key_H: // debug key to test
		{
			static thread_local std::mt19937 rng(std::random_device{}());
			std::uniform_real_distribution<float> dist(-0.5f * m_testChamber->getRootNodeSizeInt(),
			                                           0.5f * m_testChamber->getRootNodeSizeInt());

			float x = dist(rng);
			float y = dist(rng);
			float z = dist(rng);
			Debugger::instance().clearDebugPoints();
			Debugger::instance().addDebugPoint(QVector3D(x, y, z), QVector3D(0.0f, 0.5f, 0.5f));

			auto start2 = std::chrono::high_resolution_clock::now();
			NodeID n2 = m_testChamber->getLeafNodeAtCoord(QVector3D(x, y, z));
			auto end2 = std::chrono::high_resolution_clock::now();
			auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2).count();
			std::cout << "getLeafNodeAtCoord took " << duration2 << " microseconds.\n";
			// std::cout << "Closest node: " << n << " Leaf node at coord: " << n2 << "\n";
			QVector3D center = m_testChamber->getNodeCenter(n2);
			Debugger::instance().addDebugPoint(center, QVector3D(1.0f, 0.5f, 0.0f));
			// std::cout << "correct pos: " << center.x() << " " << center.y() << " " << center.z() << " \n";
			Debugger::instance().drawDebugPoints();
		}
		break;
		default: QWidget::keyPressEvent(event); break;
		}
	}
	if (actionPerformed)
	{
		event->accept();
		reloadChamberMeshAndSelection();
		update();
	}
}

void Viewport::mousePressEvent(QMouseEvent* event)
{
	Q_ASSERT(m_testChamber);
	QMatrix4x4 viewMatrix;
	getViewMatrix(viewMatrix);
	std::optional<RayHitResult> rayCastChamber = m_testChamber->rayCast(
	    screenPosToWorldRay(width(), height(), this->mapFromGlobal(QCursor::pos()), m_projection, viewMatrix));

	switch (event->button())
	{
	case Qt::MouseButton::LeftButton:
		// Needs to be called after mainViewport.show()
		if (rayCastChamber.has_value())
		{
#if 0
				m_origin = event->pos();
				//Start dragging
				setSelecting(true, QRect(m_origin, QSize()));
#endif
			m_selector->startSelection(rayCastChamber.value());
			m_isDragging = true; // We can consider a left click to always be dragging in the new selection system
			loadSelectionLines();
			update();
		}
		else
		{
			// Orbit if mouse click outside test chamber
			m_isOrbiting = true;
		}
		m_lastMouseClickPos = QVector2D(event->position());
		event->accept();
		break;
	case Qt::MouseButton::MiddleButton:
		// Middle click always orbits
		m_isOrbiting = true;
		m_lastMouseClickPos = QVector2D(event->position());
		event->accept();
		break;
	case Qt::MouseButton::RightButton:
		// To match PTI, rmb hold+drag needs to always pan, but releasing without moving opens context menus
		// For now, it always pans since we don't have context menus yet
		if (m_isPanning)
			break; // already panning
		m_isPanning = true;
		m_lastMouseClickPos = QVector2D(event->position());
		event->accept();
		break;
	default: break;
	}
	m_lastMousePos = QVector2D(event->position());
}

void Viewport::mouseMoveEvent(QMouseEvent* event)
{
	if (m_isDragging)
	{
#if 0
			// Frame buffer selection method. will be useful for item select
			m_selector->clearSurfaces();
			setSelecting(true, QRect(m_origin, event->pos()).normalized());
			m_lastDragRect = m_rect;
			ScreenSurfaceSet surfaces;
			readSelectionRectCPU(surfaces);
			for (const auto& s : surfaces)
			{
				//std::cout << s.toString() << " ";
				auto surf = m_testChamber->getSurface(s.parentID, s.wallSide);
				if (surf.has_value()) m_selector->addSelection(surf.value());
			}
#endif
		// TODO: Probably want to switch this to use the frame buffer sampling, ray casts work for now though
		QMatrix4x4 viewMatrix;
		getViewMatrix(viewMatrix);
		std::optional<RayHitResult> rayCastChamber = m_testChamber->rayCast(
		    screenPosToWorldRay(width(), height(), this->mapFromGlobal(QCursor::pos()), m_projection, viewMatrix));

		if (rayCastChamber.has_value())
		{
			m_selector->updateSelection(rayCastChamber.value());
			loadSelectionLines();
			update();
		}
	}
}

void Viewport::mouseReleaseEvent(QMouseEvent* event)
{
	QVector2D newPos = QVector2D(event->position());
	// detects whether the mouse moved since last frame.
	// used to determine click vs drag
	// so the 'click' actions are run when the mouse is released without moving it
	if ((newPos - m_lastMouseClickPos).length() > 1)
	{
		switch (event->button())
		{
		case Qt::MouseButton::LeftButton: // End Orbiting/Dragging
			if (m_isDragging)
			{
				m_selector->endSelection();
				m_isDragging = false;
			}
			m_isOrbiting = false;
			event->accept();
			break;
		case Qt::MouseButton::MiddleButton: // End Orbiting
			m_isOrbiting = false;
			event->accept();
			break;
		case Qt::MouseButton::RightButton: // End Panning
			m_isPanning = false;
			event->accept();
			break;
		default: break;
		}
	}
	else // Mouse did not move
	{
		QMatrix4x4 viewMatrix;
		switch (event->button())
		{
		case Qt::MouseButton::LeftButton: // Click Select Surface
			if (m_isDragging)
			{
				m_selector->endSelection();
				m_isDragging = false;
			}
			m_isOrbiting = false;
			event->accept();
			break;
		case Qt::MouseButton::MiddleButton:
			m_isOrbiting = false;
			event->accept();
			break;
		default: break;
		}
	}
	update();
}

void Viewport::wheelEvent(QWheelEvent* event)
{
	float prev_dist = m_cameraDistance;
	m_cameraDistance -= event->angleDelta().y() * 6.0f; // roughly matches PTI - make this customizable

	if (m_cameraDistance < cameraDistMin)
		m_cameraDistance = cameraDistMin;
	else if (m_cameraDistance > cameraDistMax)
		m_cameraDistance = cameraDistMax;

	if (prev_dist - m_cameraDistance != 0)
		update();
	event->accept();
}
} // namespace PMP
