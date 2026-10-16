// undo.h
// Implements the undo/redo system as a stack of Undoable actions
#pragma once

#include <deque>
#include <memory>

namespace PMP
{
class Undoable
{
public:
	virtual ~Undoable() {}

	virtual int undo() = 0;
	virtual int redo() = 0;
};

// per-chamber stack 
class UndoStack
{
public:
	UndoStack() = default;
	void addAction(std::unique_ptr<Undoable> action);
	void undo();
	void redo();

private:
	UndoStack(const UndoStack&) = delete; //dont allow move
	size_t m_pos = 0; // number of applied actions
	std::deque<std::unique_ptr<Undoable>> m_stack;
};
} // namespace PMP
