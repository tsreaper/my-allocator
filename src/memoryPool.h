#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#include <cstddef>

class MemoryPool
{
public:

    // Member types
    typedef void* pointer;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef unsigned char data_type;
    typedef unsigned char* data_pointer;

    // Get the single memory pool instance
    static MemoryPool& getInstance();
    
    // Delete these method to avoid copies
    MemoryPool(const MemoryPool&) = delete;
    void operator=(const MemoryPool&) = delete;
    
    // Allocate block of storage
    pointer allocate(size_type size);

    // Release block of storage
    void deallocate(data_pointer ptr);

private:

    // Constructor
    MemoryPool();
    
    // Destructor
    ~MemoryPool();
    
    // Memory pool chunk
    struct Chunk
    {
        data_type free;
        Chunk* pre;
        size_type size;

        // Chunk on linked list
        Chunk* preNode;
        Chunk* nxtNode;
    };
    
    // Chunk header size
    static const size_type HEADER_SIZE = sizeof(Chunk);

    // Block size
    static const size_type BLOCK_SIZE = 8192;

    // Block end symbol
    static const data_type BLOCK_END = 0xFF;
    
    // Memory pool head
    Chunk* head;

    // Allocate block of memory from OS
    data_pointer allocateBlock(size_type size, size_type& realSize);

    // Insert chunk after pre
    void insertChunk(Chunk* chunk, Chunk* pre);

    // Remove chunk from memory pool
    void removeChunk(Chunk* chunk);

    // Merge chunk with its next chunk
    void mergeNextChunk(Chunk* chunk);

    // Update memory pool
    void updatePool();
};

#endif