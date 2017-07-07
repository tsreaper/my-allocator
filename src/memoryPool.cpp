#include <vector>
#include "memoryPool.h"

// Get the single memory pool instance
MemoryPool& MemoryPool::getInstance()
{
    static MemoryPool instance;
    return instance;
}

// Constructor
MemoryPool::MemoryPool()
{
    head = NULL;
}

// Destructor
MemoryPool::~MemoryPool()
{
    if (head == NULL)
        return;
    
    std::vector<void*> toDelete;
    Chunk* chunk = head;
    
    do
    {
        if (chunk->pre == NULL)
            toDelete.push_back(reinterpret_cast<void*>(chunk));
        chunk = chunk->nxtNode;
    }
    while(chunk != head);
    
    for (void* ptr : toDelete)
        operator delete(ptr);
}

// Allocate block of storage
MemoryPool::pointer MemoryPool::allocate(size_type size)
{
    if (head == NULL || head->size < size)
    {
        // Allocate new block if needed
        size_type realSize;
        data_pointer ptr = allocateBlock(size, realSize);
        Chunk* chunk = reinterpret_cast<Chunk*>(ptr);

        chunk->free = 1;
        chunk->pre = NULL;
        chunk->size = realSize - HEADER_SIZE - sizeof(BLOCK_END);

        insertChunk(chunk, head);
        head = chunk;
    }

    pointer ret = reinterpret_cast<pointer>(
        reinterpret_cast<data_pointer>(head) + HEADER_SIZE
    );

    // Split head chunk into two chunks
    Chunk* chunk = reinterpret_cast<Chunk*>(
        reinterpret_cast<data_pointer>(head) + HEADER_SIZE + size
    );
    
    // Original next chunk
    Chunk* nxtChunk = reinterpret_cast<Chunk*>(
        reinterpret_cast<data_pointer>(head) + HEADER_SIZE + head->size
    );

    head->free = 0;
    if (head->size > size + HEADER_SIZE)
    {
        chunk->free = 1;
        chunk->pre = head;
        chunk->size = head->size - size - HEADER_SIZE;

        head->size = size;
        if (*reinterpret_cast<data_pointer>(nxtChunk) != BLOCK_END)
            nxtChunk->pre = chunk;

        Chunk* oldHead = head;
        insertChunk(chunk, oldHead);
        removeChunk(oldHead);
    }
    else
        // No enough space for a new chunk
        removeChunk(head);

    updatePool();
    return ret;
}

// Release block of storage
void MemoryPool::deallocate(data_pointer ptr)
{
    Chunk* chunk = reinterpret_cast<Chunk*>(ptr - HEADER_SIZE);
    Chunk* preChunk = chunk->pre;
    Chunk* nxtChunk = reinterpret_cast<Chunk*>(ptr + chunk->size);

    chunk->free = 1;

    // Merge next neighboring chunk
    if (ptr[chunk->size] != BLOCK_END && nxtChunk->free)
    {
        mergeNextChunk(chunk);
        removeChunk(nxtChunk);
    }

    // Merge previous neighboring chunk
    if (preChunk != NULL && preChunk->free)
        mergeNextChunk(preChunk);
    else
        insertChunk(chunk, head);

    updatePool();
}

// Allocate block of memory from OS
MemoryPool::data_pointer MemoryPool::allocateBlock(size_type size, size_type& realSize)
{
    realSize = size + HEADER_SIZE + sizeof(BLOCK_END);
    realSize = (realSize + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE;
    data_pointer ret = reinterpret_cast<data_pointer>(operator new(realSize));
    ret[realSize - sizeof(BLOCK_END)] = BLOCK_END;
    return ret;
}

// Insert chunk after pre
void MemoryPool::insertChunk(Chunk* chunk, Chunk* pre)
{
    if (pre == NULL)
    {
        // Empty linked list
        chunk->preNode = chunk->nxtNode = chunk;
        head = chunk;
    }
    else
    {
        chunk->preNode = pre;
        chunk->nxtNode = pre->nxtNode;
        pre->nxtNode->preNode = chunk;
        pre->nxtNode = chunk;
    }
}

// Remove chunk from memory pool
void MemoryPool::removeChunk(Chunk* chunk)
{
    if (head == chunk)
        // Change head if head is being removed
        head = head->nxtNode;

    if (chunk->preNode == chunk && chunk->nxtNode == chunk)
        // Only one element in linked list
        head = NULL;
    else
    {
        chunk->preNode->nxtNode = chunk->nxtNode;
        chunk->nxtNode->preNode = chunk->preNode;
    }
}

// Merge chunk with its next chunk
void MemoryPool::mergeNextChunk(Chunk* chunk)
{
    data_pointer ptr = reinterpret_cast<data_pointer>(chunk);
    Chunk* nxtChunk = reinterpret_cast<Chunk*>(ptr + HEADER_SIZE + chunk->size);
    chunk->size += HEADER_SIZE + nxtChunk->size;

    ptr = reinterpret_cast<data_pointer>(nxtChunk);
    if (ptr[HEADER_SIZE + nxtChunk->size] != BLOCK_END)
    {
        nxtChunk = reinterpret_cast<Chunk*>(ptr + HEADER_SIZE + nxtChunk->size);
        nxtChunk->pre = chunk;
    }
}

// Update memory pool
void MemoryPool::updatePool()
{
    if (head == NULL)
        return;

    if (head->size < head->nxtNode->size)
        head = head->nxtNode;
    else if (head->size > head->nxtNode->size && head->nxtNode->nxtNode != head)
    {
        Chunk* chunk = head->nxtNode;
        removeChunk(chunk);
        insertChunk(chunk, head->preNode);
    }
}
