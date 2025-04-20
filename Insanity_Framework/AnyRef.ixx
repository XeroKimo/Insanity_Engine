module;

#include <typeinfo>
#include <type_traits>
#include <concepts>
#include <utility>
#include <memory>

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

	export class MoveOnlyAny
	{
		template<bool OtherConst>
		friend class AnyRefT;
	private:
		struct Base
		{
			virtual ~Base() = default;
		};

		template<class Ty>
		struct Derived : public Base
		{
			Ty value;

			Derived(Ty value) : value{ std::move(value) } {}
		};

		std::unique_ptr<Base> ptr;
	public:
		MoveOnlyAny() = default;

		template<std::move_constructible Ty>
		MoveOnlyAny(Ty&& obj) :
			ptr{ std::make_unique<Derived<Ty>>(std::forward<Ty>(obj)) }
		{

		}

		MoveOnlyAny(MoveOnlyAny&& other) noexcept :
			ptr{ std::move(other.ptr) }
		{

		}

		MoveOnlyAny& operator=(MoveOnlyAny&& other) noexcept
		{
			MoveOnlyAny temp{ std::move(other) };
			temp.ptr.swap(ptr);
			return *this;
		}

		template<class Ty>
		MoveOnlyAny& operator=(Ty&& obj)
		{
			ptr = std::make_unique<Derived<Ty>>(std::forward<Ty>(obj));
			return *this;
		}

		template<class Ty>
		Ty& As()
		{
			if(typeid(*ptr) != typeid(Derived<Ty>))
				throw std::exception{};

			return static_cast<Derived<Ty>*>(ptr.get())->value;
		}

		template<class Ty>
		const Ty& As() const
		{
			if(typeid(*ptr) != typeid(Derived<Ty>))
				throw std::exception{};

			return static_cast<Derived<Ty>*>(ptr.get())->value;
		}
	};


	export template<bool isConst>
	class AnyPtrT
	{
		template<bool OtherConst>
		friend class AnyPtrT;
	private:
		const std::type_info* type = nullptr;
		std::conditional_t<isConst, const void*, void*> ptr = nullptr;

	public:
		AnyPtrT() = default;
		AnyPtrT(std::nullptr_t) {}

		template<class Ty>
			requires (!std::same_as<AnyPtrT, Ty>)
		AnyPtrT(Ty* obj) :
			type{ &typeid(Ty) },
			ptr{ obj }
		{

		}

		AnyPtrT(const AnyPtrT<false>& other) requires (isConst) :
			type{ other.type },
			ptr{ other.ptr }
		{

		}

		AnyPtrT(const AnyPtrT& other) :
			type{ other.type },
			ptr{ other.ptr }
		{

		}

		AnyPtrT(AnyPtrT&& other) noexcept :
			type{ std::exchange(other.type, nullptr) },
			ptr{ std::exchange(other.ptr, nullptr) }
		{

		}

		AnyPtrT& operator=(const AnyPtrT& ptr) = default;
		AnyPtrT& operator=(AnyPtrT&& ptr) noexcept
		{
			type = std::exchange(ptr.type, nullptr);
			ptr = std::exchange(ptr.ptr, nullptr);
			return *this;
		}

		AnyPtrT& operator=(const AnyPtrT<false>& ptr) requires (isConst)
		{
			type = ptr.type;
			ptr = ptr.ptr;
			return *this;
		} 

		AnyPtrT& operator=(AnyPtrT<false>&& ptr) noexcept requires (isConst)
		{
			type = std::exchange(ptr.type, nullptr);
			ptr = std::exchange(ptr.ptr, nullptr);
			return *this;
		}

		template<class Ty>
		Ty* operator=(const Ty* obj)
		{
			type = &typeid(Ty);
			ptr = obj;
			return obj;
		}

		template<class Ty>
		Ty* As() const
		{
			if(!ptr)
				return nullptr;

			if(*type != typeid(Ty))
				throw std::exception{};

			return static_cast<Ty*>(ptr);
		}

		bool operator==(std::nullptr_t)
		{
			return ptr == nullptr; 
		}
	};

	export using AnyPtr = AnyPtrT<false>;
	export using AnyConstPtr = AnyPtrT<true>;
}