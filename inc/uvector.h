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

#define UVECTOR_VERSION             1.1.0

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
    uvector<T>& operator = (const uvector &vect);

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

template<typename T>
T& uvector<T>::at(uint32_t i){
    if((i >= size_val) || (capacity_val == 0)){
        uvector_debug("uvector<T>::at(): index %lu is out of range\n", (long unsigned int)i);
        return err_retval;
    }
    else return container_ptr[i];
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
        for(uint32_t i = 0; i < capacity_val; i++){
            container_ptr_res[i] = container_ptr[i];
        }
        for(uint32_t i = 0; i < new_capacity - capacity_val; i++){
            container_ptr_res[capacity_val + i] = T();
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
uint32_t uvector<T>::capacity(){
    return capacity_val;
}

template<typename T>
void uvector<T>::clear(){
    size_val = 0;
}

template<typename T>
bool uvector<T>::resize(uint32_t new_vect_size){
    if(new_vect_size == size_val){
        return true;
    }
    if(new_vect_size < size_val){
        size_val = new_vect_size;
        return true;
    }

    if(new_vect_size > size_val){
        if(capacity_val >= new_vect_size){
            for(uint32_t i = 0; i < new_vect_size - size_val; i++){
                container_ptr[size_val + i] = T();
            }
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
        for(uint32_t i = 0; i < new_vect_size - old_size; i++){
            container_ptr[old_size + i] = value;
        }
        return true;
    }
    else return false;
}

template<typename T>
bool uvector<T>::push_back(T item){
    if(capacity_val > size_val){
        container_ptr[size_val] = item;
        size_val++;
        return true;
    }
    if(resize(size_val + 1) != false){
        container_ptr[size_val - 1] = item;
        return true;
    }
    return false;
}

template<typename T>
bool uvector<T>::pop_back(){
    if(size_val > 0){
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
        dfree(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr), USING_PTR_ADDRESS);
        capacity_val = 0;
        return true;
    }

    if(reserve_new_memory(size_val, &container_ptr_res) != false){
        for(uint32_t i = 0; i < size_val; i++){
            container_ptr_res[i] = container_ptr[i];
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
void uvector<T>::info(){
    uvector_debug("vector info(): start ptr: 0x%08lX, stop ptr: 0x%08lX, total size: %d bytes\n", container_ptr, static_cast<uint8_t *>(&container_ptr[capacity() - 1]), capacity()*sizeof(T));
}

template<typename T>
uvector<T>& uvector<T>::operator = (const uvector &vect){
    if(&vect != this){
        size_val = vect.size_val;
        capacity_val = vect.capacity_val;
        container_ptr = vect.container_ptr;
        container_ptr_res = vect.container_ptr_res;
        this->alloc_mem_ptr = vect.alloc_mem_ptr;
        for(uint32_t i = 0; i < vect.size(); i++){
            this->push_back(vect.data()[i]);
        }
    }
    return *this;
}

template<typename T>
uvector<T>::~uvector(){
    dfree(this->alloc_mem_ptr, reinterpret_cast<void**>(&container_ptr), USING_PTR_ADDRESS);
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
    uint32_t elements_num = static_cast<uint32_t>(((static_cast<float>(_size))*CAPACITY_RESERVE_KOEF));
    uint32_t alloc_size = sizeof(T)*elements_num;
    dalloc(this->alloc_mem_ptr, alloc_size, &container_ptr);
    if(container_ptr != NULL){
        size_val = _size;
        capacity_val = elements_num;
    }
    else {
        size_val = 0;
        capacity_val = 0;
    }
    container_ptr_res = NULL;
}

#endif // UVECTOR_H
