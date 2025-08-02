//optional_array.h
//Provides an alternative to std::optional for arrays.
#ifndef PMP_OPTIONAL_ARRAY_H
#pragma once

#include <cstddef>
#include <array>
#include <optional>

namespace PMP
{
    template<class T, std::size_t S>
    class OptionalArray
    {
        public:
            OptionalArray(): m_IsFilled(false) {}

            OptionalArray(std::array<T, S> val) :
                m_IsFilled(true),
                m_Inner(val)
            {}

            bool has_value() {
                return m_IsFilled;
            }

            std::array<T, S>& value() {
                if (has_value()) {
                    return m_Inner;
                } else {
                    throw std::bad_optional_access();
                }
            }
            //Returns array and initializes it if it's empty
            std::array<T,S>& value_initialize() {
                if (!m_IsFilled) {
                    m_Inner = std::array<T, S>{};  // default-initialize with zeroes / default-T
                    m_IsFilled = true;
                }
                return m_Inner;
            }

            void set_value(std::array<T, S> val) {
                m_Inner.swap(val);
                m_IsFilled = true;
            }

            void reset() {
                m_IsFilled = false;
            }
        private:
            bool m_IsFilled;
            std::array<T, S> m_Inner;
    };
}
#endif // !PMP_OPTIONAL_ARRAY_H
