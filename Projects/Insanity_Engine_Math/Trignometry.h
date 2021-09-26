#pragma once
#include "MathConcepts.h"
#include <math.h>
#include <numbers>

namespace InsanityEngine::Math
{
    namespace Types
    {
        template<Concepts::FloatingPoint T>
        struct Degrees;

        template<Concepts::FloatingPoint T>
        struct Radians;
    }

    namespace Constants
    {
        template<Concepts::FloatingPoint T>
        constexpr T pi = std::numbers::pi_v<T>;

        template<Concepts::FloatingPoint T>
        constexpr T radiansToDegrees = static_cast<T>(180) / std::numbers::pi_v<T>;

        template<Concepts::FloatingPoint T>
        constexpr T degreesToRadians = std::numbers::pi_v<T> / static_cast<T>(180);
    }

    namespace Functions::Trignometry
    {
        using Types::Degrees;
        using Types::Radians;

        template<Concepts::FloatingPoint T>
        constexpr T ToRadians(T degrees) { return degrees * Constants::degreesToRadians<T>; }

        template<Concepts::FloatingPoint T>
        constexpr T ToDegrees(T radians) { return radians * Constants::radiansToDegrees<T>; }

        template<Concepts::FloatingPoint T>
        constexpr Radians<T> ToRadians(Degrees<T> degrees);

        template<Concepts::FloatingPoint T>
        constexpr Degrees<T> ToDegrees(Radians<T> radians);
    }

    namespace Types
    {
        template<Concepts::FloatingPoint T>
        struct Degrees
        {
            using value_type = T;

        private:
            value_type m_data = 0;

        public:
            constexpr Degrees() = default;
            explicit constexpr Degrees(value_type value) : m_data(value) {}

        public:
            constexpr Degrees<value_type> operator+=(const Degrees<value_type>& rh)
            {
                m_data += rh.m_data;
                return *this;
            }
            constexpr Degrees<value_type> operator-=(const Degrees<value_type>& rh)
            {
                m_data -= rh.m_data;
                return *this;
            }
            constexpr Degrees<value_type> operator*=(const Degrees<value_type>& rh)
            {
                m_data *= rh.m_data;
                return *this;
            }
            constexpr Degrees<value_type> operator/=(const Degrees<value_type>& rh)
            {
                m_data /= rh.m_data;
                return *this;
            }

            friend constexpr Degrees<value_type> operator+(Degrees<value_type> lh, const Degrees<value_type>& rh)
            {
                return (lh += rh);
            }
            friend constexpr Degrees<value_type> operator-(Degrees<value_type> lh, const Degrees<value_type>& rh)
            {
                return (lh -= rh);
            }
            friend constexpr Degrees<value_type> operator*(Degrees<value_type> lh, const Degrees<value_type>& rh)
            {
                return (lh *= rh);
            }
            friend constexpr Degrees<value_type> operator/(Degrees<value_type> lh, const Degrees<value_type>& rh)
            {
                return (lh /= rh);
            }

            friend auto operator<=>(const Degrees<value_type>& lh, const Degrees<value_type>& rh) = default;

        public:
            value_type Data() const { return m_data; }
            Radians<value_type> ToRadians() const;
        };

        template<Concepts::FloatingPoint T>
        struct Radians
        {
            using value_type = T;

        private:
            value_type m_data = 0;

        public:
            constexpr Radians() = default;
            explicit constexpr Radians(value_type value) : m_data(value) {}

        public:
            constexpr Radians<value_type>& operator+=(const Radians<value_type>& rh)
            {
                m_data += rh.m_data;
                return *this;
            }
            constexpr Radians<value_type>& operator-=(const Radians<value_type>& rh)
            {
                m_data -= rh.m_data;
                return *this;
            }
            constexpr Radians<value_type>& operator*=(const Radians<value_type>& rh)
            {
                m_data *= rh.m_data;
                return *this;
            }
            constexpr Radians<value_type>& operator/=(const Radians<value_type>& rh)
            {
                m_data /= rh.m_data;
                return *this;
            }

            friend constexpr Radians<value_type> operator+(Radians<value_type> lh, const Radians<value_type>& rh)
            {
                return (lh += rh);
            }
            friend constexpr Radians<value_type> operator-(Radians<value_type> lh, const Radians<value_type>& rh)
            {
                return (lh -= rh);
            }
            friend constexpr Radians<value_type> operator*(Radians<value_type> lh, const Radians<value_type>& rh)
            {
                return (lh *= rh);
            }
            friend constexpr Radians<value_type> operator/(Radians<value_type> lh, const Radians<value_type>& rh)
            {
                return (lh /= rh);
            }

            friend auto operator<=>(const Radians<value_type>& lh, const Radians<value_type>& rh) = default;

        public:
            value_type Data() const { return m_data; }
            Degrees<value_type> ToDegrees() const;
        };

        template<Concepts::FloatingPoint T>
        Degrees<T> Radians<T>::ToDegrees() const { return Functions::Trignometry::ToDegrees(*this); }

        template<Concepts::FloatingPoint T>
        Radians<T> Degrees<T>::ToRadians() const { return Functions::Trignometry::ToRadians(*this); }

    }

    namespace Functions::Trignometry
    {
        template<Concepts::FloatingPoint T>
        constexpr Radians<T> ToRadians(const Degrees<T> degrees) { return Radians<T>(ToRadians(degrees.Data())); }

        template<Concepts::FloatingPoint T>
        constexpr Degrees<T> ToDegrees(const Radians<T> radians) { return Degrees<T>(ToDegrees(radians.Data())); }
    }
}