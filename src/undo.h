#pragma once
#include <deque>
#include <memory>

namespace PMP
{
    class Undoable
    {
        public:
        virtual ~Undoable() {}
        virtual void undo() = 0;
        virtual void redo() = 0;
    };
    // Singleton stack for keeping track of all actions
    class UndoStack
    {
        public:
        static UndoStack& instance();
        void addAction(std::unique_ptr<Undoable> action);
        void undo();
        void redo();
        private:
        UndoStack() = default;
        UndoStack(const UndoStack&) = delete;
        int m_pos; //0 is current
        std::deque<std::unique_ptr<Undoable>> m_stack;
    };
}