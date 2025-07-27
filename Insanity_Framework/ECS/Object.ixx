module;

#include <cstdint>
#include <memory>
#include <concepts>
#include <cassert>

export module InsanityFramework.ECS.Object;
import InsanityFramework.Memory;
import InsanityFramework.Allocator;
import InsanityFramework.AnyRef;

namespace InsanityFramework
{
	export class Object;
	export class ObjectAllocator;

	export class Object
	{
		friend ObjectAllocator;

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
		void operator()(Object* ptr)
		{
			ObjectAllocator::Delete(ptr);
		}
	};

	export template<std::derived_from<InsanityFramework::Object> Ty, class DeleterTy = ObjectDeleter>
	using UniqueObject = std::unique_ptr<Ty, DeleterTy>;
}