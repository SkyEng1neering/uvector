# About the project
This is the magic implementation of dynamic array. Based on [dalloc](https://github.com/SkyEng1neering/dalloc) allocator, that solves memory fragmentation problem. So you can use it in your embedded project and not to be afraid of program crash by reason of memory fragmentation.

You can use __uvector__ like vector container from STL, it supports all base methods of std::vector.

# Dependencies
__uvector__ based on Based on [dalloc](https://github.com/SkyEng1neering/dalloc) allocator, so you should include it to your project.

# Usage
## Using uvector with single heap area
[dalloc](https://github.com/SkyEng1neering/dalloc) allocator is configurable, it can work with only one memory area that you define, or you can select which memory area should be used for each your allocation.

If you want to use in your project only one heap area, you should define "USE_SINGLE_HEAP_MEMORY" in file __dalloc_conf.h__. This is the simpliest way to use __uvector__ because it allows you to abstract away from working with memory.

```c++
/* File dalloc_conf.h */
#define USE_SINGLE_HEAP_MEMORY
#define SINGLE_HEAP_SIZE				4096UL //define heap size that you want to have
```

Then you should define uint8_t array in your code, that will be used for storing data. This array should be named "single_heap" and it should be have size SINGLE_HEAP_SIZE (defined in file __dalloc_conf.h__).

```c++
#include "uvector.h"

uint8_t single_heap[SINGLE_HEAP_SIZE] = {0};
```

Why you should implement this array in your code by yourself? Because you may want to store this array for example in specific memory region, or you may want to apply to this array some attributes, like this:

```c++
__attribute__((section(".ITCM_RAM"))) uint8_t single_heap[SINGLE_HEAP_SIZE] = {0};
```
or like this:

```c++
__ALIGN_BEGIN uint8_t single_heap[SINGLE_HEAP_SIZE] __ALIGN_END;
```

So for example that's how looks like example of using __uvector__ with single memory region on STM32 MCU:

```c++
#include "uvector.h"

__ALIGN_BEGIN uint8_t single_heap[SINGLE_HEAP_SIZE] __ALIGN_END;

void main(){
  uvector<int> vector_of_ints;
  
  vector_of_ints.push_back(1);
  vector_of_ints.push_back(2);
  vector_of_ints.push_back(3);
  vector_of_ints.push_back(4);
 
  for(uint32_t i = 0; i < vector_of_ints.size(); i++){
    printf("Vector element %lu: %d\n", i, vector_of_ints.at(i));
  }
 
  while(1){}
}  
```

By the way you can store to __uvector__ not only scalar types, it can be also structures and classes, for example you can store to __uvector__ [ustring](https://github.com/SkyEng1neering/ustring) objects:

```c++
#include "uvector.h"
#include "ustring.h"

__ALIGN_BEGIN uint8_t single_heap[SINGLE_HEAP_SIZE] __ALIGN_END;

void main(){
  ustring string1("this is the string 1");
  ustring string2("this is the string 2");

  uvector<ustring> vector_of_strings;
  
  vector_of_strings.push_back(string1);
  vector_of_strings.push_back(string2);
 
  for(uint32_t i = 0; i < vector_of_strings.size(); i++){
    printf("Vector element %lu: %s\n", i, vector_of_strings.at(i).c_str());
  }
 
  while(1){}
}  
```

### One moment that you should take into consideration
If you will push back elements to the vector one-by-one so the maximum number of elements that vector can store will be about ___(SINGLE_HEAP_SIZE/sizeof(element))/2___
because of reallocation mechanism, when you want to push_back 1 new element, if vector capacity is not enough for it - new memory area allocated with N+1 size, and data from old area is copied to new area, and only after this - old memory is freed. So if you know how much elements you need to store in vector you can reserve the memory for it by 1 procedure:

```c++
uvector<int> vector_of_ints(100); //space for 100 int numbers is reserved in memory after create vector object
```

or

```c++
uvector<int> vector_of_ints; 
vector_of_ints.reserve(100); //space for 100 int numbers is reserved in memory after create vector object
```
If you will do like this, you can store to vector not only ___(SINGLE_HEAP_SIZE/sizeof(element))/2___ elements, but almost ___SINGLE_HEAP_SIZE/sizeof(element)___ elements.

## Using uvector with different heap areas

If you want to use several different heap areas, you can define it explicitly:

```c++
/* File dalloc_conf.h */
//#define USE_SINGLE_HEAP_MEMORY //comment this define
```

```c++
#include "uvector.h"

#define HEAP_SIZE			1024

/* Declare an arrays that will be used for dynamic memory allocations */
__ALIGN_BEGIN uint8_t heap_array1[HEAP_SIZE] __ALIGN_END;
__ALIGN_BEGIN uint8_t heap_array2[HEAP_SIZE] __ALIGN_END;

/* Declare an dalloc heap structures, it will contains all allocations info */
heap_t heap1;
heap_t heap2;

void main(){
  /* Init heap memory */
  heap_init(&heap1, (void*)heap_array1, HEAP_SIZE);
  heap_init(&heap2, (void*)heap_array2, HEAP_SIZE);

  uvector<int> vector_of_ints1(&heap1);
  uvector<int> vector_of_ints2(&heap2);
  
  vector_of_ints1.push_back(11);
  vector_of_ints1.push_back(22);
  vector_of_ints1.push_back(33);
  vector_of_ints1.push_back(44);
  
  vector_of_ints2.push_back(111);
  vector_of_ints2.push_back(222);
  vector_of_ints2.push_back(333);
  vector_of_ints2.push_back(444);
  
  for(uint32_t i = 0; i < vector_of_ints1.size(); i++){
    printf("Vector element of array 1 %lu: %d\n", i, vector_of_ints1.at(i));
  }
 
  for(uint32_t i = 0; i < vector_of_ints2.size(); i++){
    printf("Vector element of array 2 %lu: %d\n", i, vector_of_ints2.at(i));
  }
 
  while(1){}
}  
  
```
## P.S.
In any time you can check what exactly is going on in your heap memory using functions:
```c++
/* If you use different heap areas in your project */
void print_dalloc_info(heap_t *heap_struct_ptr);
void dump_dalloc_ptr_info(heap_t* heap_struct_ptr);
void dump_heap(heap_t* heap_struct_ptr);
```
```c++
/* If you use single heap area in your project */
void print_def_dalloc_info();
void dump_def_dalloc_ptr_info();
void dump_def_heap();
```
