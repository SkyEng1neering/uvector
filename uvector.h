#ifndef UVECTOR_H
#define UVECTOR_H

#include "string.h"
#include "dalloc.h"

#define CAPACITY_RESERVE_KOEF 		1.2
#define uvector_debug				printf

template <typename T>
class uvector
{
private:
    uint32_t size_val;
    uint32_t capacity_val;
    T *container_ptr;
    T *container_ptr_res;
    heap_t *alloc_mem_ptr;

    T err_retval;

    bool reserve_new_memory(uint32_t new_vect_size, T **cont_ptr);

public:
    uvector(uint32_t _size, heap_t *_alloc_mem_ptr);
    uvector(heap_t *_alloc_mem_ptr);
    uvector();
    uvector(const uvector &vector);
    ~uvector();
    uvector& operator = (const uvector &vect);

    T& at(uint32_t i);
    T& operator[](uint32_t i);
    T& front();
    T& back();
    T* data() const;
    bool empty() const;
    uint32_t size() const;
    bool reserve(uint32_t new_capacity);
    uint32_t capacity();
    void clear();
    bool resize(uint32_t new_vect_size);
	bool resize(uint32_t new_vect_size, T value);
    bool push_back(T item);
    bool pop_back();
    bool shrink_to_fit();
    void assign_mem_pointer(heap_t *mem_ptr);
    void info();
};

#endif // UVECTOR_H
