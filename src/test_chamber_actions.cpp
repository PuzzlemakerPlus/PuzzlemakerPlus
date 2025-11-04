#include "test_chamber_actions.h"

namespace PMP
{
    int TCExtrudeAction::undo()
    {
        bool anyChanged = false;
        for (const auto& usi : m_extrudeUSIs) {
            const NodeID node = toNode(usi);
            anyChanged |= m_testChamber->setNodeActive(node, true);
        }

        if (!anyChanged) return 0;

        m_testChamber->generateSurfaces();
        m_selector->clearSurfaces();

        for (auto it = m_extrudeUSIs.rbegin(); it != m_extrudeUSIs.rend(); ++it) {
            if (auto surf = m_testChamber->getSurface(toNode(*it), it->side); surf.has_value()) {
                // FIXME: Selections
                //m_selector->addSelection(surf.value());
            }
        }
        qDebug() << "Undo extrude action, added " << m_extrudeUSIs.size() << " surfaces to selection.";
        return 1;
    }
    int TCExtrudeAction::redo()
    {
        bool anyChanged = false;
        for (const auto& usi : m_extrudeUSIs) {
            const NodeID node = toNode(usi);
            anyChanged |= m_testChamber->setNodeActive(node, false);
        }

        if (!anyChanged) return 0;

        m_testChamber->generateSurfaces();
        m_selector->clearSurfaces();

        for (auto it = m_newSelectUSIs.rbegin(); it != m_newSelectUSIs.rend(); ++it) {
            if (it->side == WallSide::NONE) continue;
            if (auto surf = m_testChamber->getSurface(toNode(*it), it->side); surf.has_value()) {
                // FIXME: Selections
                //m_selector->addSelection(surf.value());
            }
        }
        qDebug() << "Redo extrude action, added " << m_newSelectUSIs.size() << " surfaces to selection.";
        return 1;
    }
    int TCRecedeAction::undo()
    {
        bool anyChanged = false;
        for (const auto& usi : m_newSelectUSIs) {
            const NodeID node = toNode(usi);
            anyChanged |= m_testChamber->setNodeActive(node, false);
        }

        if (!anyChanged) return 0;

        m_testChamber->generateSurfaces();
        m_selector->clearSurfaces();

        for (auto it = m_recedeUSIs.rbegin(); it != m_recedeUSIs.rend(); ++it) {
            if (auto surf = m_testChamber->getSurface(toNode(*it), it->side); surf.has_value()) {
                // FIXME: Selections
                //m_selector->addSelection(surf.value());
            }
        }
        qDebug() << "Undo recede action, added " << m_recedeUSIs.size() << " surfaces to selection.";
        return 1;
    }
    int TCRecedeAction::redo()
    {
        bool anyChanged = false;
        for (const auto& usi : m_newSelectUSIs) {
            const NodeID node = toNode(usi);
            anyChanged |= m_testChamber->setNodeActive(node, true);
        }

        if (!anyChanged) return 0;

        m_testChamber->generateSurfaces();
        m_selector->clearSurfaces();

        for (auto it = m_recedeUSIs.rbegin(); it != m_recedeUSIs.rend(); ++it) {
            if (it->side == WallSide::NONE) continue;
            if (auto surf = m_testChamber->getSurface(toNode(*it), it->side); surf.has_value()) {
                // FIXME: Selections
                //m_selector->addSelection(surf.value());
            }
        }
        qDebug() << "Redo recede action, added " << m_recedeUSIs.size() << " surfaces to selection.";
        return 1;
    }
}