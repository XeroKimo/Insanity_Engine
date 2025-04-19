module;

#include <cstdint>
#include <bit>
#include <memory>
#include <cassert>

export module InsanityFramework.Allocator;
import InsanityFramework.Memory;

namespace InsanityFramework
{
	class BufferView
	{
		std::byte* buffer = nullptr;
		std::size_t size = 0;

	public:
		BufferView() = default;

		BufferView(void* bufferStart, std::size_t size) :
			buffer{ static_cast<std::byte*>(bufferStart) },
			size{ size }
		{

		}

		void* Raw() const noexcept { return buffer; }

		template<class Ty>
		Ty* RawAs() const noexcept { return static_cast<Ty*>(Raw()); }

		std::size_t Size() const { return size; }

		BufferView SplitFromStart(std::size_t offset)
		{
			return (offset <= size) ? 
				BufferView{ buffer + offset, size - offset } :
				BufferView{ nullptr, 0 };
		}

		BufferView SplitFromStart(std::size_t offset, std::size_t newSize)
		{
			return (offset <= newSize && newSize <= size) ?
				BufferView{ buffer + offset, newSize } :
				BufferView{ nullptr, 0 };
		}

		BufferView SplitFromEnd(std::size_t offset)
		{
			return (offset <= size) ?
				BufferView{ buffer + size - offset, offset } :
				BufferView{ nullptr, 0 };
		}

		BufferView SplitFromEnd(std::size_t offset, std::size_t newSize)
		{
			return (newSize <= offset && offset <= size) ?
				BufferView{ buffer + size - offset, newSize } :
				BufferView{ nullptr, 0 };
		}

		BufferView AlignNext(std::size_t alignment)
		{
			std::byte* newAddress = static_cast<std::byte*>(AlignCeil(buffer, alignment));
			assert(buffer < newAddress);
			std::size_t sizeDifference = newAddress - buffer;
			return (alignment < size && sizeDifference <= size) ? 
				BufferView{ newAddress, size - sizeDifference } :
				BufferView{ nullptr, 0 };
		}

		operator void* () const noexcept { return buffer; }

		template<class Ty>
		operator Ty* () const noexcept { return RawAs<Ty>(); }
	};

	export template<class Ty>
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
		Ty* RemoveSelf() noexcept
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

	export template<class Ty>
	class IntrusiveForwardListNode
	{
		Ty* next = nullptr;

	public:
		//The inputted pointer is set to next and is returned
		Ty* Append(Ty* ptr) noexcept
		{
			if(next)
			{
				ptr->next = next;
			}
			next = ptr;

			return ptr;
		}

		//Removes the next item on the list and returns it
		Ty* RemoveNext() noexcept
		{
			Ty* oldNext = next;

			if(oldNext)
				next = oldNext->next;

			return oldNext;
		}

		//Only works assuming we are the first in the list
		//Removes itself and returns itself
		Ty* RemoveSelf() noexcept
		{
			next = nullptr;
			return static_cast<Ty*>(this);
		}


		//Only works assuming we are the first in the list
		//Removes itself and returns next
		Ty* RemoveSelfAndGetNext() noexcept
		{
			return std::exchange(next, nullptr);
		}

		Ty* Next() const noexcept { return next; }
	};

	class alignas(std::max_align_t) FreeListHeader : public IntrusiveListNode<FreeListHeader>
	{
	private:
		std::size_t dataRegionSize;

	public:
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
			assert(!inUse);

			if(!Next() || Next()->inUse)
				return false;

			dataRegionSize += Next()->dataRegionSize;
			std::destroy_at(Next()->RemoveSelf());
			return true;
		}

		BufferView DataRegion() noexcept
		{
			return { this + 1, dataRegionSize };
		}

		std::size_t Size() const noexcept { return dataRegionSize; }
	};

	export class FreeListAllocator
	{
		BufferView buffer;
	public:
		FreeListAllocator(BufferView buffer) :
			buffer{ buffer }
		{
			std::construct_at<FreeListHeader>(buffer, buffer.Size() - sizeof(FreeListHeader));
		}

		~FreeListAllocator()
		{
			FreeListHeader* current = First();
			while(current)
			{
				std::destroy_at(std::exchange(current, current->Next()));
			}
		}

	public:
		void* Allocate(std::size_t size)
		{
			FreeListHeader* currentHeader = First();
			while(currentHeader && currentHeader->inUse && currentHeader->Size() >= size)
			{
				currentHeader = currentHeader->Next();
			}

			if(!currentHeader)
				return nullptr;

			if(currentHeader->Size() > size)
				currentHeader = currentHeader->Split(size);

			currentHeader->inUse = true;
			return currentHeader->DataRegion();
		}

		void Free(void* ptr)
		{
			FreeListHeader* header = GetHeaderFromPointer(ptr);
			header->inUse = false;

			header->MergeNext();
			header = header->Previous();
			if(header && !header->inUse)
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

	export template<std::size_t BucketSize, std::size_t BucketAlignment = alignof(std::max_align_t)>
		requires(std::has_single_bit(BucketAlignment))
	class PoolAllocator
	{
		class BucketHeader : public IntrusiveForwardListNode<BucketHeader>
		{
		public:
			BufferView DataRegion() { return { this + 1, BucketSize }; }
		};

	public:
		static constexpr std::size_t alignedBucketSize = AlignCeil(BucketSize + sizeof(BucketHeader), BucketAlignment);

	private:
		BufferView buffer;
		BucketHeader* freeList = nullptr;
	public:
		PoolAllocator() = default;
		PoolAllocator(BufferView buffer) :
			buffer{ buffer.AlignNext(BucketAlignment) }
		{
			BufferView currentAddress = buffer;

			if(currentAddress.Size() >= alignedBucketSize)
			{
				BucketHeader* currentBucket = freeList = std::construct_at(std::exchange(currentAddress, currentAddress.SplitFromStart(alignedBucketSize)).RawAs<BucketHeader>());

				while(currentAddress.Size() >= alignedBucketSize)
				{
					BucketHeader* oldBucket = currentBucket;
					currentBucket = std::construct_at(std::exchange(currentAddress, currentAddress.SplitFromStart(alignedBucketSize)).RawAs<BucketHeader>());
					oldBucket->Append(currentBucket);
				}
			}
		}

		void* Allocate(std::size_t size)
		{
			if(size > BucketSize)
				return nullptr;

			if(!freeList)
				return nullptr;
			
			BucketHeader* selected = freeList;
			freeList = freeList->RemoveSelfAndGetNext();
			return selected->DataRegion();
		}

		void Free(void* ptr)
		{
			BucketHeader* header = GetBucketFromPointer(ptr);
			header->Append(freeList);
			freeList = header;
		}

	private:
		BucketHeader* GetBucketFromPointer(void* ptr)
		{
			return std::launder(static_cast<BucketHeader*>(ptr) - 1);
		}
	};
}