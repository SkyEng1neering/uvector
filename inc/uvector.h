/**
 * @file uvector.h
 * @brief Dynamic array container for embedded systems using dalloc memory allocator
 *
 * uvector is a lightweight std::vector alternative designed for embedded systems.
 * It uses the dalloc defragmenting memory allocator for dynamic memory management,
 * providing automatic pointer tracking and memory defragmentation.
 *
 * Features:
 * - Dynamic resizing with automatic capacity growth
 * - Support for POD and non-POD types (with proper constructor/destructor calls)
 * - Two modes: multi-heap (explicit heap pointer) and single-heap (global default)
 * - STL-like interface (at, front, back, push_back, pop_back, etc.)
 *
 * @note Requires dalloc library for memory allocation
 * @note Define USE_SINGLE_HEAP_MEMORY to use single global heap mode
 *
 * @author Alexey Vasilenko
 * @copyright 2021 Alexey Vasilenko, Apache License 2.0
 */

/*
 * Copyright 2021 Alexey Vasilenko
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef UVECTOR_H
#define UVECTOR_H

#include "dalloc.h"
#include <new>
#include <type_traits>

/** @brief Library version string */
#define UVECTOR_VERSION             "1.2.0"

/** @brief Capacity growth factor when resizing (20% extra space) */
#define CAPACITY_RESERVE_KOEF       1.2

/** @brief Debug output macro (defaults to printf) */
#define uvector_debug               printf

/**
 * @brief Dynamic array container using dalloc memory allocator
 *
 * @tparam T Element type (supports both POD and non-POD types)
 *
 * uvector provides a dynamic array similar to std::vector but designed for
 * embedded systems with custom memory allocation via dalloc.
 *
 * Memory is allocated from a heap_t structure provided at construction time
 * (multi-heap mode) or from a global default heap (single-heap mode).
 *
 * Example usage (multi-heap mode):
 * @code
 * uint8_t buffer[4096];
 * heap_t heap;
 * heap_init(&heap, buffer, 4096);
 *
 * uvector<int> vec(&heap);
 * vec.push_back(42);
 * vec.push_back(100);
 * printf("Size: %u\n", vec.size());  // Output: Size: 2
 * @endcode
 *
 * Example usage (single-heap mode, requires USE_SINGLE_HEAP_MEMORY):
 * @code
 * uvector<int> vec;
 * vec.push_back(42);
 * @endcode
 */
template <typename T>
class uvector
{
private:
    uint32_t size_val;          ///< Current number of elements
    uint32_t capacity_val;      ///< Allocated capacity (number of elements)
    T *container_ptr;           ///< Pointer to element storage
    T *container_ptr_res;       ///< Temporary pointer used during reallocation
    heap_t *alloc_mem_ptr;      ///< Pointer to heap used for allocation

    T err_retval;               ///< Error return value for out-of-bounds access

    /**
     * @brief Allocate raw memory for elements
     * @param new_vect_size Number of elements to allocate space for
     * @param cont_ptr Output pointer to allocated memory
     * @return true if allocation succeeded, false otherwise
     */
    bool reserve_new_memory(uint32_t new_vect_size, T **cont_ptr);

public:
    // ==================== Constructors & Destructor ====================

    /**
     * @brief Default constructor
     *
     * In single-heap mode (USE_SINGLE_HEAP_MEMORY): uses dalloc_get_default_heap().
     * In multi-heap mode: creates vector with NULL heap (must call assign_mem_pointer before use).
     */
    uvector();

#ifdef USE_SINGLE_HEAP_MEMORY
    /**
     * @brief Constructor with initial capacity reservation
     * @param _size Initial capacity to reserve
     *
     * Creates empty vector and reserves space for _size elements using default heap.
     */
    uvector(uint32_t _size);
#else
    /**
     * @brief Constructor with heap pointer and initial capacity
     * @param _size Initial capacity to reserve
     * @param _alloc_mem_ptr Pointer to heap for memory allocation
     *
     * Creates empty vector, assigns heap, and reserves space for _size elements.
     */
    uvector(uint32_t _size, heap_t *_alloc_mem_ptr);

    /**
     * @brief Constructor with heap pointer
     * @param _alloc_mem_ptr Pointer to heap for memory allocation
     *
     * Creates empty vector with specified heap. No memory is allocated until first push_back.
     */
    uvector(heap_t *_alloc_mem_ptr);
#endif

    /**
     * @brief Copy constructor
     * @param vector Source vector to copy from
     *
     * Creates a deep copy of the source vector. Inherits heap pointer from source.
     * All elements are copied using assignment operator.
     */
    uvector(const uvector &vector);

    /**
     * @brief Destructor
     *
     * Calls destructors on all constructed elements and frees allocated memory.
     */
    ~uvector();

    /**
     * @brief Copy assignment operator
     * @param vect Source vector to copy from
     * @return Reference to this vector
     *
     * Replaces contents with a deep copy of vect. Handles self-assignment safely.
     * Inherits heap pointer from source vector.
     */
    uvector<T>& operator = (const uvector &vect);

    // ==================== Element Access ====================

    /**
     * @brief Access element at index with bounds checking
     * @param i Index of element to access
     * @return Reference to element at index i, or err_retval if out of bounds
     *
     * If index is out of bounds, prints debug message and returns reference to err_retval.
     * @note Unlike std::vector::at(), does not throw exception on invalid index.
     */
    T& at(uint32_t i);

    /**
     * @brief Access element at index (alias for at())
     * @param i Index of element to access
     * @return Reference to element at index i
     * @see at()
     */
    T& operator[](uint32_t i);

    /**
     * @brief Access first element
     * @return Reference to first element
     * @warning Undefined behavior if vector is empty
     */
    T& front();

    /**
     * @brief Access last element
     * @return Reference to last element
     * @warning Undefined behavior if vector is empty
     */
    T& back();

    /**
     * @brief Get pointer to underlying array
     * @return Pointer to first element, or nullptr if empty
     *
     * Returns direct pointer to element storage for C-style array access.
     */
    T* data() const;

    // ==================== Capacity ====================

    /**
     * @brief Check if vector is empty
     * @return true if size() == 0, false otherwise
     */
    bool empty() const;

    /**
     * @brief Get number of elements
     * @return Current number of elements in the vector
     */
    uint32_t size() const;

    /**
     * @brief Get current capacity
     * @return Number of elements that can be held without reallocation
     */
    uint32_t capacity() const;

    /**
     * @brief Reserve capacity for at least new_capacity elements
     * @param new_capacity Minimum capacity to reserve
     * @return true if successful, false if allocation failed
     *
     * If new_capacity <= current capacity, does nothing.
     * Existing elements are preserved during reallocation.
     */
    bool reserve(uint32_t new_capacity);

    /**
     * @brief Resize vector to contain new_vect_size elements
     * @param new_vect_size New size of the vector
     * @return true if successful, false if allocation failed
     *
     * If new size is smaller, destructors are called on removed elements.
     * If new size is larger, new elements are default-constructed.
     */
    bool resize(uint32_t new_vect_size);

    /**
     * @brief Resize vector and initialize new elements with value
     * @param new_vect_size New size of the vector
     * @param value Value to assign to new elements
     * @return true if successful, false if allocation failed
     *
     * Like resize(), but new elements are initialized with copies of value.
     */
    bool resize(uint32_t new_vect_size, T value);

    /**
     * @brief Reduce capacity to match size
     * @return true if successful, false if reallocation failed
     *
     * Frees unused memory by reallocating to exact size.
     * If vector is empty, frees all memory.
     */
    bool shrink_to_fit();

    // ==================== Modifiers ====================

    /**
     * @brief Remove all elements
     *
     * Calls destructors on all elements, then sets size to 0.
     * Capacity and allocated memory remain unchanged.
     */
    void clear();

    /**
     * @brief Add element to the end
     * @param item Element to add (copied into vector)
     * @return true if successful, false if allocation failed
     *
     * If capacity is insufficient, automatically grows the vector.
     */
    bool push_back(T item);

    /**
     * @brief Remove last element
     * @return true if element was removed, false if vector was empty
     *
     * Calls destructor on the removed element and decreases size by 1.
     */
    bool pop_back();

    /**
     * @brief Remove element at index
     * @param i Index of element to remove
     * @return true if element was removed, false if index out of bounds
     *
     * Calls destructor on the removed element. Elements after index i
     * are shifted left to fill the gap. Size is decreased by 1.
     */
    bool pop(uint32_t i);

    // ==================== Memory Management ====================

    /**
     * @brief Set heap pointer for memory allocation
     * @param mem_ptr Pointer to heap structure
     *
     * @warning Must be called on empty vector before any allocations,
     *          otherwise may cause memory leaks.
     */
    void assign_mem_pointer(heap_t *mem_ptr);

    /**
     * @brief Get current heap pointer
     * @return Pointer to heap used for allocation
     */
    heap_t* get_mem_pointer() const;

    // ==================== Debug ====================

    /**
     * @brief Print debug information about vector
     *
     * Outputs start/end pointers and total allocated size to uvector_debug.
     */
    void info();
};

template<typename T>
T& uvector<T>::at(uint32_t i){
    if((i >= size_val) || (capacity_val == 0)){
        uvector_debug("uvector<T>::at(): index %lu is out of range\n", (long unsigned int)i);
        return err_retval;
    }
    else {
        T* obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
        return *obj_ptr;
    }
}

template<typename T>
T& uvector<T>::operator[](uint32_t i){
    return at(i);
}

template<typename T>
T& uvector<T>::front(){
    return at(0);
}

template<typename T>
T& uvector<T>::back(){
    return at(size_val - 1);
}

template<typename T>
T* uvector<T>::data() const{
    return container_ptr;
}

template<typename T>
bool uvector<T>::empty() const{
    if(size_val == 0){
        return true;
    }
    else return false;
}

template<typename T>
uint32_t uvector<T>::size() const{
    return size_val;
}

template<typename T>
bool uvector<T>::reserve_new_memory(uint32_t new_vect_size, T **cont_ptr){
    uint32_t alloc_size = sizeof(T)*new_vect_size;
    dalloc(this->alloc_mem_ptr, alloc_size, reinterpret_cast<void**>(cont_ptr));
    if(*cont_ptr != NULL){
        return true;
    }
    else return false;
}

template<typename T>
bool uvector<T>::reserve(uint32_t new_capacity){
    if(capacity_val >= new_capacity){
       return true;
    }
    if(reserve_new_memory(new_capacity, &container_ptr_res) != false){
        for(uint32_t i = 0; i < new_capacity; i++){
            new ((T*)((size_t)container_ptr_res + i*sizeof(T))) T;
        }
        for(uint32_t i = 0; i < capacity_val; i++){
            T* obj_new_ptr = (T*)((size_t)container_ptr_res + i*sizeof(T));
            T* obj_old_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
            *obj_new_ptr = *obj_old_ptr;
        }

        for(uint32_t i = 0; i < capacity_val; i++){
            T* obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
            obj_ptr->~T();
        }

        if(validate_ptr(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr), USING_PTR_ADDRESS, NULL) != false){
            dfree(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr), USING_PTR_ADDRESS);
        }
        replace_pointers(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr_res), reinterpret_cast<void**>(&container_ptr));
        capacity_val = new_capacity;
        return true;
    }
    return false;
}

template<typename T>
uint32_t uvector<T>::capacity() const{
    return capacity_val;
}

template<typename T>
void uvector<T>::clear(){
    // Call destructors on all elements being removed
    for(uint32_t i = 0; i < size_val; i++){
        T* obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
        obj_ptr->~T();
        new (obj_ptr) T;  // Reconstruct to maintain invariant
    }
    size_val = 0;
}

template<typename T>
bool uvector<T>::resize(uint32_t new_vect_size){
    if(new_vect_size == size_val){
        return true;
    }
    if(new_vect_size < size_val){
        // Call destructors on elements being removed
        for(uint32_t i = new_vect_size; i < size_val; i++){
            T* obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
            obj_ptr->~T();
            new (obj_ptr) T;  // Reconstruct to maintain invariant
        }
        size_val = new_vect_size;
        return true;
    }

    if(new_vect_size > size_val){
        if(capacity_val >= new_vect_size){
            size_val = new_vect_size;
            return true;
        }

        uint32_t elements_num = static_cast<uint32_t>(((static_cast<float>(new_vect_size)) * CAPACITY_RESERVE_KOEF));
        if(reserve(elements_num) == true){
            size_val = new_vect_size;
            return true;
        }
    }
    return false;
}

template<typename T>
bool uvector<T>::resize(uint32_t new_vect_size, T value){
    uint32_t old_size = size_val;
    if(resize(new_vect_size) != false){
    	if(new_vect_size > old_size){
    		for(uint32_t i = 0; i < new_vect_size - old_size; i++){
				T* obj_ptr = (T*)((size_t)container_ptr + (i + old_size)*sizeof(T));
				*obj_ptr = value;
			}
    	}
        return true;
    }
    else return false;
}

template<typename T>
bool uvector<T>::push_back(T item){
    if(capacity_val > size_val){
        T* obj_ptr = (T*)((size_t)container_ptr + size_val*sizeof(T));
        *obj_ptr = item;
        size_val++;
        return true;
    }
    if(resize(size_val + 1) != false){
        T* obj_ptr = (T*)((size_t)container_ptr + (size_val - 1)*sizeof(T));
        *obj_ptr = item;
        return true;
    }
    return false;
}

template<typename T>
bool uvector<T>::pop(uint32_t i){
    if(i >= size()){
        return false;
    }

    // If removing the last element, use pop_back to avoid double destruction
    if(i == size() - 1){
        return pop_back();
    }

    // Destroy the element being removed
    T* removed_obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
    removed_obj_ptr->~T();
    new (removed_obj_ptr) T;  // Reconstruct for assignment

    // Shift elements left
    for(uint32_t k = i; k < size() - 1; k++){
        T* current_obj_ptr = (T*)((size_t)container_ptr + k*sizeof(T));
        T* next_obj_ptr = (T*)((size_t)container_ptr + (k + 1)*sizeof(T));
        *current_obj_ptr = *next_obj_ptr;
    }

    // Destroy and reconstruct the last element (now a duplicate)
    T* last_obj_ptr = (T*)((size_t)container_ptr + (size_val - 1)*sizeof(T));
    last_obj_ptr->~T();
    new (last_obj_ptr) T;

    size_val--;
    return true;
}

template<typename T>
bool uvector<T>::pop_back(){
    if(size_val > 0){
        // Call destructor on the element being removed
        T* obj_ptr = (T*)((size_t)container_ptr + (size_val - 1)*sizeof(T));
        obj_ptr->~T();
        new (obj_ptr) T;  // Reconstruct to maintain invariant
        size_val--;
        return true;
    }
    return false;
}

template<typename T>
bool uvector<T>::shrink_to_fit(){
    if((size_val == capacity_val) || (capacity_val == 0)){
       return true;
    }

    if(size_val == 0){
        // Destroy all constructed elements before freeing memory
        for(uint32_t i = 0; i < capacity_val; i++){
            T* obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
            obj_ptr->~T();
        }
        dfree(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr), USING_PTR_ADDRESS);
        capacity_val = 0;
        return true;
    }

    if(reserve_new_memory(size_val, &container_ptr_res) != false){
        // Construct new objects with placement new
        for(uint32_t i = 0; i < size_val; i++){
            new ((T*)((size_t)container_ptr_res + i*sizeof(T))) T;
        }
        // Copy values
        for(uint32_t i = 0; i < size_val; i++){
            T* obj_ptr_res = (T*)((size_t)container_ptr_res + i*sizeof(T));
            T* obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
            *obj_ptr_res = *obj_ptr;
        }
        // Call destructors on old elements
        for(uint32_t i = 0; i < capacity_val; i++){
            T* obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
            obj_ptr->~T();
        }
        dfree(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr), USING_PTR_ADDRESS);
        replace_pointers(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr_res), reinterpret_cast<void**>(&container_ptr));
        capacity_val = size_val;
        return true;
    }
    return false;
}

template<typename T>
void uvector<T>::assign_mem_pointer(heap_t *mem_ptr){
    this->alloc_mem_ptr = mem_ptr;
}

template<typename T>
heap_t* uvector<T>::get_mem_pointer() const{
    return this->alloc_mem_ptr;
}

template<typename T>
void uvector<T>::info(){
    if(capacity_val == 0){
        uvector_debug("vector info(): empty (capacity=0)\n");
        return;
    }
    uvector_debug("vector info(): start ptr: 0x%08lX, stop ptr: 0x%08lX, total size: %lu bytes\n",
        (unsigned long)(size_t)container_ptr,
        (unsigned long)(size_t)&container_ptr[capacity_val - 1],
        (unsigned long)(capacity_val * sizeof(T)));
}

template<typename T>
uvector<T>& uvector<T>::operator = (const uvector &vect){
    if(&vect != this){
        // Free existing memory if allocated
        if(container_ptr != NULL){
            for(uint32_t i = 0; i < capacity_val; i++){
                T* obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
                obj_ptr->~T();
            }
            dfree(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr), USING_PTR_ADDRESS);
        }
        size_val = 0;
        capacity_val = 0;
        container_ptr = NULL;
        container_ptr_res = NULL;
        this->alloc_mem_ptr = vect.alloc_mem_ptr;
        for(uint32_t i = 0; i < vect.size(); i++){
            this->push_back(vect.data()[i]);
        }
    }
    return *this;
}

template<typename T>
uvector<T>::~uvector(){
    if(container_ptr != NULL){
        for(uint32_t i = 0; i < capacity_val; i++){
            T* obj_ptr = (T*)((size_t)container_ptr + i*sizeof(T));
            obj_ptr->~T();
        }
        dfree(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr), USING_PTR_ADDRESS);
    }
}

template<typename T>
uvector<T>::uvector(const uvector &vector){
    size_val = 0;
    capacity_val = 0;
    container_ptr = NULL;
    container_ptr_res = NULL;
    this->alloc_mem_ptr = vector.alloc_mem_ptr;
    for(uint32_t i = 0; i < vector.size(); i++){
        this->push_back(vector.data()[i]);
    }
}

#ifdef USE_SINGLE_HEAP_MEMORY
template<typename T>
uvector<T>::uvector(){
    alloc_mem_ptr = dalloc_get_default_heap();
    size_val = 0;
    capacity_val = 0;
    container_ptr = NULL;
    container_ptr_res = NULL;
}

template<typename T>
uvector<T>::uvector(uint32_t _size){
    this->alloc_mem_ptr = dalloc_get_default_heap();
    size_val = 0;
    capacity_val = 0;
    container_ptr = NULL;
    container_ptr_res = NULL;

    reserve(_size);
}
#else
template<typename T>
uvector<T>::uvector(){
    alloc_mem_ptr = NULL;
    size_val = 0;
    capacity_val = 0;
    container_ptr = NULL;
    container_ptr_res = NULL;
}

template<typename T>
uvector<T>::uvector(heap_t *_alloc_mem_ptr){
    this->alloc_mem_ptr = _alloc_mem_ptr;
    size_val = 0;
    capacity_val = 0;
    container_ptr = NULL;
    container_ptr_res = NULL;
}

template<typename T>
uvector<T>::uvector(uint32_t _size, heap_t *_alloc_mem_ptr){
    this->alloc_mem_ptr = _alloc_mem_ptr;
    size_val = 0;
    capacity_val = 0;
    container_ptr = NULL;
    container_ptr_res = NULL;

    reserve(_size);
}
#endif



#endif // UVECTOR_H
