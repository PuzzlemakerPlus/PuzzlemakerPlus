// optional_array.h
// Provides an alternative to std::optional for arrays.
#pragma once

#include <array>
#include <cstddef>
#include <optional>

namespace PMP
{
template <class T, std::size_t S> class OptionalArray
{
public:
	OptionalArray() : m_IsFilled(false) {}

	OptionalArray(std::array<T, S> val) : m_IsFilled(true), m_Inner(val) {}

	const bool has_value() const { return m_IsFilled; }

	// For read-only access to the array contents
	const std::array<T, S> &valueRead() const
	{
		if (has_value())
		{
			return m_Inner;
		}
		else
		{
			throw std::bad_optional_access();
		}
	}

	std::array<T, S> &value()
	{
		if (has_value())
		{
			return m_Inner;
		}
		else
		{
			throw std::bad_optional_access();
		}
	}

	// Returns array and initializes it if it's empty
	std::array<T, S> &value_initialize()
	{
		if (!m_IsFilled)
		{
			m_Inner = std::array<T, S>{}; // default-initialize with zeroes / default-T
			m_IsFilled = true;
		}
		return m_Inner;
	}

	void set_value(std::array<T, S> val)
	{
		m_Inner.swap(val);
		m_IsFilled = true;
	}

	void reset() { m_IsFilled = false; }

private:
	bool m_IsFilled;
	std::array<T, S> m_Inner;
};
} // namespace PMP
