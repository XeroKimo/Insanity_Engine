#pragma once
#include <variant>

namespace InsanityEngine::Utils
{
    template<class Error>
    class Unexpected
    {
        Error e;
    public:
        Unexpected(Error e) :
            e(e)
        {

        }

        Error GetError() const { return e; };
    };

    template<class Value, class Error>
    class Expected
    {
        std::variant<Value, Error> m_var;

    public:
        Expected(Value v) :
            m_var(v)
        {

        }

        Expected(Unexpected<Error> e) :
            m_var(e.GetError())
        {

        }

        operator bool() const { return HasValue(); }

    public:
        bool HasValue() const { return std::holds_alternative<Value>(m_var); }

        Value& GetValue() { return std::get<Value>(m_var); } 
        const Value& GetValue() const { return std::get<Value>(m_var); }

        Error GetError() const { return std::get<Error>(m_var); } 
    };

}