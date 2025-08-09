#include "undo.h"
#include <memory>
namespace PMP
{
    UndoStack& UndoStack::instance() {
        static UndoStack instance;
        return instance;
    }

    void UndoStack::addAction(std::unique_ptr<Undoable> action)
    {
        while (m_stack.size() > m_pos)
            m_stack.pop_back();

        m_stack.push_back(std::move(action));
        m_pos = m_stack.size();
    }
    void UndoStack::undo() 
    {
        if (m_stack.size() != 0 && m_pos > 0)
        {
            --m_pos;
            if(!m_stack[m_pos]->undo()) m_pos++;
        }
    }
    void UndoStack::redo() 
    {
        if (m_pos < m_stack.size())
        {
            if(m_stack[m_pos]->redo()) m_pos++;
        }
    }
}