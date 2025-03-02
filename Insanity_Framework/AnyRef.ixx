module;

#include <typeinfo>
#include <type_traits>
#include <concepts>
#include <utility>

export module InsanityFramework.AnyRef;

namespace InsanityFramework
{

	export template<bool isConst>
		class AnyRefT
	{
		template<bool OtherConst>
		friend class AnyRefT;
	private:
		const std::type_info* type;
		std::conditional_t<isConst, const void*, void*> ptr;

	public:
		template<class Ty>
			requires (!std::same_as<AnyRefT, Ty>)
		AnyRefT(Ty& obj) :
			type{ &typeid(Ty) },
			ptr{ &obj }
		{

		}

		AnyRefT(const AnyRefT<false>& other) requires (isConst) :
			type{ other.type },
			ptr{ other.ptr }
		{

		}

		AnyRefT(const AnyRefT& other) :
			type{ other.type },
			ptr{ other.ptr }
		{

		}

		AnyRefT(AnyRefT&& other) noexcept :
			type{ std::exchange(other.type, nullptr) },
			ptr{ std::exchange(other.ptr, nullptr) }
		{

		}

		template<class Ty>
		Ty& operator=(const Ty& obj)
		{
			auto& self = As<Ty>();
			self = obj;
			return self;
		}

		template<class Ty>
		Ty& operator=(Ty&& obj)
		{
			auto& self = As<Ty>();
			self = std::move(obj);
			return self;
		}

		template<class Ty>
		Ty& As()
		{
			if(*type != typeid(Ty))
				throw std::exception{};

			return *static_cast<Ty*>(ptr);
		}

		template<class Ty>
		const Ty& As() const
		{
			if(*type != typeid(Ty))
				throw std::exception{};

			return *static_cast<const Ty*>(ptr);
		}
	};

	export using AnyRef = AnyRefT<false>;
	export using AnyConstRef = AnyRefT<true>;
}