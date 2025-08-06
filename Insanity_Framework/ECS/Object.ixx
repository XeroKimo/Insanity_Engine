module;

#include <cstdint>
#include <memory>
#include <concepts>
#include <cassert>

export module InsanityFramework.ECS.Scene:Object;
import InsanityFramework.Memory;
import InsanityFramework.Allocator;
import InsanityFramework.AnyRef;

namespace InsanityFramework
{
	export class Object;
	export class ObjectAllocator;

	export template<std::derived_from<InsanityFramework::Object> Ty>
	class UniqueObject;

	export class Object
	{
		friend ObjectAllocator;

		template<std::derived_from<InsanityFramework::Object> Ty>
		friend class UniqueObject;
	public:
		struct Key
		{
			friend ObjectAllocator;
		private:
			Key() = default;
		};

	protected:
		Object(Key) {};
		virtual ~Object() = default;

	private:
		void* operator new(std::size_t size, ObjectAllocator& allocator);

		void operator delete(void* ptr, ObjectAllocator& allocator);

		void operator delete(void* ptr);

	private:
		bool isRoot = true;

	public:
		bool IsRoot() const noexcept { return isRoot; }
	};

	export class ObjectAllocator
	{
		friend class Object;

		static constexpr std::size_t pageSize = AlignNextPow2<size_t>(8'000'000);
		class Page : public IntrusiveForwardListNode<Page>
		{
			FreeListAllocator allocator;

		public:
			void* operator new(size_t size)
			{
				return ::operator new(pageSize, std::align_val_t{ pageSize });
			}

			void operator delete(void* ptr)
			{
				::operator delete(ptr, std::align_val_t{ pageSize });
			}

		public:
			Page() :
				allocator{ { this + 1, pageSize - sizeof(Page) } }
			{
			}

			void* Allocate(std::size_t size)
			{
				return allocator.Allocate(size);
			}

			void Free(void* ptr)
			{
				assert(GetPageFrom(ptr) == this);
				allocator.Free(ptr);
			}

		public:
			static Page* GetPageFrom(void* ptr)
			{
				return std::launder(static_cast<Page*>(AlignFloorPow2(ptr, pageSize)));
			}
		};


	private:
		Page* firstPage = new Page{ /*this*/ };

	public:
		ObjectAllocator() = default;

		~ObjectAllocator()
		{
			while(firstPage)
			{
				delete std::exchange(firstPage, firstPage->RemoveSelfAndGetNext());
			}
		}

		template<std::derived_from<Object> Ty, class... Args>
		Ty* New(Args&&... args)
		{
			return new(*this) Ty{ Object::Key{}, std::forward<Args>(args)... };
		}

		static void Delete(Object* ptr)
		{
			delete ptr;
		}

		bool Contains(Object* ptr) const
		{
			Page* page = Page::GetPageFrom(ptr);
			Page* temp = firstPage;
			while (temp)
			{
				if (temp == page)
					return true;
				temp = temp->Next();
			}
			return false;
		}

	private:
		void* Allocate(std::size_t size)
		{
			Page* currentPage = firstPage;
			void* ptr = currentPage->Allocate(size);
			while(!ptr)
			{
				Page* oldPage = currentPage;
				currentPage = currentPage->Next();

				if(!currentPage)
				{
					currentPage = new Page{/* this*/ };
					oldPage->Append(currentPage);
				}

				ptr = currentPage->Allocate(size);
			}

			return ptr;
		}

		static void Free(void* ptr)
		{
			Page::GetPageFrom(ptr)->Free(ptr);
		}
	};

	void* Object::operator new(std::size_t size, ObjectAllocator& allocator)
	{
		return allocator.Allocate(size);
	}

	void Object::operator delete(void* ptr, ObjectAllocator& allocator)
	{
		allocator.Free(ptr);
	}

	void Object::operator delete(void* ptr)
	{
		ObjectAllocator::Free(ptr);
	}

	struct ObjectDeleter
	{
		void operator()(Object* ptr);
	};

	export template<std::derived_from<InsanityFramework::Object> Ty>
	class UniqueObject
	{
		template<std::derived_from<InsanityFramework::Object> Ty>
		friend class UniqueObject;

	private:
		std::unique_ptr<Ty, ObjectDeleter> ptr;

	public:
		UniqueObject() = default;
		UniqueObject(std::nullptr_t) : ptr{ nullptr } {}
		UniqueObject(Ty* ptr) : ptr{ ptr }
		{
			if (ptr)
				ptr->isRoot = false;
		}
		UniqueObject(const UniqueObject&) = delete;
		UniqueObject(UniqueObject&& other) noexcept :
			ptr{ std::move(other).ptr }
		{

		}

		template<class OtherTy>
			requires std::convertible_to<OtherTy*, Ty*>
		UniqueObject(UniqueObject<OtherTy>&& other) noexcept :
			ptr{ std::move(other).ptr }
		{

		}

		UniqueObject& operator=(std::nullptr_t) { ptr = nullptr; return *this; }
		UniqueObject& operator=(const UniqueObject&) = delete;
		UniqueObject& operator=(UniqueObject&& other) noexcept
		{
			UniqueObject temp{ std::move(other) };
			swap(temp);
			return *this;
		}

		template<class OtherTy>
			requires std::convertible_to<OtherTy*, Ty*>
		UniqueObject& operator=(UniqueObject<OtherTy>&& other) noexcept
		{
			UniqueObject temp{ std::move(other).ptr };
			swap(temp);
			return *this;
		}

		~UniqueObject() = default;

	public:
		auto release()
		{
			ptr->isRoot = true;
			return ptr.release();
		}

		void reset(Ty* newPtr) noexcept
		{
			ptr.reset(newPtr);
			if (newPtr)
				newPtr->isRoot = false;
		}

		void swap(UniqueObject& other) noexcept
		{
			ptr.swap(other.ptr);
		}

		auto get() { return ptr.get(); }
		auto get() const { return ptr.get(); }

		auto operator->() { return ptr.operator->(); }
		auto operator->() const { return ptr.operator->(); }

		decltype(auto) operator*() { return (ptr.operator*()); }
		decltype(auto) operator*() const { return (ptr.operator*()); }

		operator bool() const noexcept { return static_cast<bool>(ptr); }
	};
}