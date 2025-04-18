module;

#include <cstdint>
#include <bit>
#include <memory>

export module InsanityFramework.Allocator;
import InsanityFramework.Memory;

namespace InsanityFramework
{
	class BufferView
	{
		void* buffer = nullptr;
		std::size_t size = 0;

	public:
		BufferView() = default;
		BufferView(void* bufferStart, std::size_t size) :
			buffer{ bufferStart },
			size{ size }
		{

		}

		void* Raw() const noexcept { return buffer; }

		template<class Ty>
		Ty* RawAs() const noexcept { return static_cast<Ty*>(buffer); }

		std::size_t Size() const { return size; }

		BufferView SplitFromStart(std::size_t offset)
		{
			return (offset <= size) ? 
				BufferView{ static_cast<char*>(buffer) + offset, size - offset } :
				BufferView{ nullptr, 0 };
		}

		BufferView SplitFromStart(std::size_t offset, std::size_t newSize)
		{
			return (offset <= newSize && newSize <= size) ?
				BufferView{ static_cast<char*>(buffer) + offset, newSize } :
				BufferView{ nullptr, 0 };
		}

		BufferView SplitFromEnd(std::size_t offset)
		{
			return (offset <= size) ?
				BufferView{ static_cast<char*>(buffer) + size - offset, offset } :
				BufferView{ nullptr, 0 };
		}

		BufferView SplitFromEnd(std::size_t offset, std::size_t newSize)
		{
			return (newSize <= offset && offset <= size) ?
				BufferView{ static_cast<char*>(buffer) + size - offset, newSize } :
				BufferView{ nullptr, 0 };
		}

		operator void* () const noexcept { return buffer; }

		template<class Ty>
		operator Ty* () const noexcept { return RawAs<Ty>(); }
	};

	template<class Ty>
	class IntrusiveListNode
	{
		Ty* next = nullptr;
		Ty* previous = nullptr;

	public:
		//The inputted pointer is set to next and is returned
		Ty* Append(Ty* ptr) noexcept
		{
			if(next)
			{
				next->previous = ptr;
				ptr->next = next;
			}
			ptr->previous = static_cast<Ty*>(this);
			next = ptr;

			return ptr;
		}

		//The inputted pointer is set to previous and is returned
		Ty* Prepend(Ty* ptr) noexcept
		{
			if(previous)
			{
				previous->next = ptr;
				ptr->previous = previous;
			}
			ptr->next = static_cast<Ty*>(this);
			previous = ptr;

			return ptr;
		}

		//Removes itself from the list and returns itself
		Ty* Splice() noexcept
		{
			if(previous)
			{
				previous->next = next;
				previous = nullptr;
			}
			if(next)
			{
				next->previous = previous;
				next = nullptr;
			}

			return static_cast<Ty*>(this);
		}

		Ty* Next() const noexcept { return next; }
		Ty* Previous() const noexcept { return previous; }
	};

	template<class Ty>
	class IntrusiveForwardListNode
	{
		Ty* next = nullptr;

	public:
		//The inputted pointer is set to next and is returned
		Ty* Append(Ty* ptr) noexcept
		{
			if(next)
			{
				next->previous = ptr;
				ptr->next = next;
			}
			ptr->previous = this;
			next = ptr;

			return ptr;
		}

		//Removes the next item on the list and returns it
		Ty* SpliceNext() noexcept
		{
			Ty* oldNext = next;

			if(oldNext)
				next = oldNext->next;

			return oldNext;
		}

		Ty* Next() const noexcept { return next; }
	};

	class alignas(std::max_align_t) FreeListHeader : public IntrusiveListNode<FreeListHeader>
	{
	private:
		std::size_t dataRegionSize;
		bool inUse = false;

	public:
		FreeListHeader(std::size_t dataRegionSize) :
			dataRegionSize{ dataRegionSize }
		{

		}

		FreeListHeader* Split(std::size_t requestedSize)
		{
			std::size_t alignedSize = AlignCeilPow2<size_t>(requestedSize + sizeof(FreeListHeader), 8);
			if(alignedSize >= dataRegionSize)
				return nullptr;

			Append(std::construct_at<FreeListHeader>(DataRegion().SplitFromEnd(alignedSize), requestedSize));
			dataRegionSize -= alignedSize;

			return Next();
		}

		bool MergeNext()
		{
			if(!Next() || Next()->inUse)
				return false;

			dataRegionSize += Next()->dataRegionSize;
			std::destroy_at(Next()->Splice());
			return true;
		}

		BufferView DataRegion() noexcept
		{
			return { this + 1, dataRegionSize };
		}

		bool SetInUse(bool use) { inUse = use; }
		bool InUse() const noexcept { return inUse; }
		std::size_t Size() const noexcept { return dataRegionSize; }
	};

	class FreeListAllocator
	{
		BufferView buffer;
	public:
		FreeListAllocator(BufferView buffer) :
			buffer{ buffer }
		{
			std::construct_at<FreeListHeader>(buffer, buffer.Size() - sizeof(FreeListHeader));
		}

	public:
		void* Allocate(std::size_t size)
		{
			FreeListHeader* currentHeader = First();
			while(currentHeader && currentHeader->InUse() && currentHeader->Size() >= size)
			{
				currentHeader = currentHeader->Next();
			}

			if(!currentHeader)
				return nullptr;

			if(currentHeader->Size() > size)
				currentHeader = currentHeader->Split(size);

			currentHeader->SetInUse(true);
			return currentHeader->DataRegion();
		}

		void Free(void* ptr)
		{
			FreeListHeader* header = GetHeaderFromPointer(ptr);
			header->SetInUse(false);

			header->MergeNext();
			header = header->Previous();
			if(header && !header->InUse())
				header->MergeNext();
		}

	private:
		FreeListHeader* First()
		{
			return buffer.RawAs<FreeListHeader>();
		}

		FreeListHeader* GetHeaderFromPointer(void* ptr)
		{
			return std::launder(static_cast<FreeListHeader*>(ptr) - 1);
		}
	};
}