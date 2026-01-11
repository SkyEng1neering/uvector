# uvector

Dynamic array container for embedded systems using [dalloc](https://github.com/SkyEng1neering/dalloc) memory allocator.

**Version:** 1.3.0
**License:** Apache 2.0
**Author:** Alexey Vasilenko

## Features

- Dynamic resizing with automatic capacity growth (20% reserve)
- Support for POD and non-POD types with proper constructor/destructor calls
- Two modes: multi-heap (explicit heap pointer) and single-heap (global default)
- STL-like interface (`at`, `front`, `back`, `push_back`, `pop_back`, etc.)
- Memory defragmentation support via dalloc
- No memory fragmentation issues for long-running embedded applications

## Dependencies

uvector requires [dalloc](https://github.com/SkyEng1neering/dalloc) v1.5.0+ memory allocator.

## API Reference

### Constructors

| Constructor | Description |
|-------------|-------------|
| `uvector()` | Default constructor (single-heap mode only) |
| `uvector(heap_t* heap)` | Constructor with heap pointer (multi-heap mode) |
| `uvector(uint32_t size, heap_t* heap)` | Constructor with initial capacity |
| `uvector(const uvector& other)` | Copy constructor |

### Element Access

| Method | Description |
|--------|-------------|
| `T& at(uint32_t i)` | Access element with bounds checking |
| `T& operator[](uint32_t i)` | Access element without bounds checking |
| `T& front()` | Access first element |
| `T& back()` | Access last element |
| `T* data()` | Direct access to underlying array |

### Capacity

| Method | Description |
|--------|-------------|
| `bool empty()` | Check if container is empty |
| `uint32_t size()` | Return number of elements |
| `uint32_t capacity()` | Return current capacity |
| `bool reserve(uint32_t n)` | Reserve capacity for n elements |
| `bool shrink_to_fit()` | Reduce capacity to match size |

### Modifiers

| Method | Description |
|--------|-------------|
| `bool push_back(T value)` | Add element to end |
| `bool pop_back()` | Remove last element (calls destructor) |
| `bool pop(uint32_t i)` | Remove element at index (calls destructor) |
| `void clear()` | Remove all elements (calls destructors) |
| `bool resize(uint32_t n)` | Change size (calls destructors when shrinking) |
| `bool resize(uint32_t n, T value)` | Change size with fill value |

### Assignment

| Method | Description |
|--------|-------------|
| `uvector& operator=(const uvector& other)` | Copy assignment |

## Object Lifecycle

uvector properly manages object lifecycles for non-POD types:

- **Construction**: Objects are constructed using placement new when capacity is reserved
- **Destruction**: Destructors are called when:
  - Elements are removed via `pop_back()`, `pop(i)`, `clear()`
  - Size is reduced via `resize()`
  - Memory is freed via `shrink_to_fit()` or destructor
- **Copy**: Copy constructors/assignment operators are used for element copying

This makes uvector safe for storing objects that manage resources (file handles, GPIO, peripherals, etc.).

## Usage

### Single-heap mode

Enable single-heap mode via compiler flags or custom config file:

```bash
# Via compiler flags
gcc -DUSE_SINGLE_HEAP_MEMORY ...

# Or via custom config file
gcc -DDALLOC_CUSTOM_CONF_FILE=\"my_dalloc_conf.h\" ...
```

```c
// my_dalloc_conf.h
#define USE_SINGLE_HEAP_MEMORY
```

Register heap buffer before using uvector:

```cpp
#include "uvector.h"

// Buffer must be 4-byte aligned!
__attribute__((aligned(4)))
static uint8_t heap_buffer[4096];

int main() {
    // Register heap (must be called before any uvector operations)
    if (!dalloc_register_heap(heap_buffer, sizeof(heap_buffer))) {
        return -1;
    }

    uvector<int> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    for (uint32_t i = 0; i < vec.size(); i++) {
        printf("Element %u: %d\n", i, vec.at(i));
    }

    return 0;
}
```

### Multi-heap mode

When `USE_SINGLE_HEAP_MEMORY` is not defined, use explicit heap pointers:

```cpp
#include "uvector.h"

#define HEAP_SIZE  1024

// Buffers must be 4-byte aligned!
__attribute__((aligned(4)))
static uint8_t buffer1[HEAP_SIZE];
__attribute__((aligned(4)))
static uint8_t buffer2[HEAP_SIZE];

heap_t heap1, heap2;

int main() {
    heap_init(&heap1, buffer1, sizeof(buffer1));
    heap_init(&heap2, buffer2, sizeof(buffer2));

    uvector<int> vec1(&heap1);
    uvector<int> vec2(&heap2);

    vec1.push_back(10);
    vec2.push_back(20);

    // Cleanup
    heap_deinit(&heap1);
    heap_deinit(&heap2);

    return 0;
}
```

### Using with complex types

uvector works with classes that have constructors/destructors:

```cpp
class Sensor {
public:
    int gpio_pin;

    Sensor() : gpio_pin(-1) {}
    Sensor(int pin) : gpio_pin(pin) { init_gpio(pin); }
    ~Sensor() { if (gpio_pin >= 0) deinit_gpio(gpio_pin); }
};

uvector<Sensor> sensors(&heap);
sensors.push_back(Sensor(GPIO_PIN_1));
sensors.push_back(Sensor(GPIO_PIN_2));

sensors.pop_back();  // Destructor called, GPIO deinitialized
sensors.clear();     // All destructors called
```

### Platform-specific memory placement

```cpp
// ESP32: Use internal SRAM for fast access
__attribute__((section(".iram1"), aligned(4)))
static uint8_t fast_heap[4096];

// STM32F4/F3 (F405, F407, F303, etc.): Use CCM RAM (64KB, CPU-only, no DMA)
__attribute__((section(".ccmram"), aligned(4)))
static uint8_t ccm_heap[8192];

// STM32: Generic aligned buffer
__ALIGN_BEGIN uint8_t heap_buffer[4096] __ALIGN_END;

// Any platform: ensure 4-byte alignment
alignas(4) static uint8_t heap_buffer[4096];
```

### Memory optimization

When pushing elements one by one, maximum capacity is approximately `(HEAP_SIZE / sizeof(T)) / 2` due to reallocation. Pre-reserve to maximize usage:

```cpp
uvector<int> vec(&heap);
vec.reserve(100);  // Reserve space for 100 elements upfront

// Or at construction:
uvector<int> vec2(100, &heap);
```

## Debugging

Use dalloc debug functions to inspect heap state:

```cpp
// Multi-heap mode
print_dalloc_info(&heap);
dump_dalloc_ptr_info(&heap);
dump_heap(&heap);

// Single-heap mode
print_def_dalloc_info();
dump_def_dalloc_ptr_info();
dump_def_heap();

// Check heap state
heap_t *heap = dalloc_get_default_heap();
if (heap != NULL) {
    printf("Used: %lu / %lu bytes\n",
           (unsigned long)heap->offset,
           (unsigned long)heap->total_size);
}
```

## Configuration

uvector inherits configuration from dalloc. Key options:

| Option | Default | Description |
|--------|---------|-------------|
| `USE_SINGLE_HEAP_MEMORY` | undefined | Enable single-heap mode |
| `MAX_NUM_OF_ALLOCATIONS` | 100 | Maximum simultaneous allocations |
| `ALLOCATION_ALIGNMENT_BYTES` | 4 | Memory alignment (typically 4 for ARM) |
| `USE_THREAD_SAFETY` | undefined | Enable thread-safe operations |

Configure via compiler flags:
```bash
gcc -DUSE_SINGLE_HEAP_MEMORY -DMAX_NUM_OF_ALLOCATIONS=50 ...
```

Or via custom config file:
```bash
gcc -DDALLOC_CUSTOM_CONF_FILE=\"my_dalloc_conf.h\" ...
```

## Testing

uvector includes comprehensive unit tests using Google Test:

```bash
cd libs/uvector/tests
mkdir build && cd build
cmake ..
make -j$(nproc)
./uvector_tests              # Multi-heap tests (180 tests)
./uvector_single_heap_tests  # Single-heap tests (23 tests)
```

Tests cover:
- Basic operations (construction, push, pop, access)
- Capacity management (reserve, resize, shrink_to_fit)
- Copy semantics (copy constructor, assignment)
- Object lifecycle (constructor/destructor calls)
- Edge cases (empty vector, bounds, heap exhaustion)
- Stress tests (rapid push/pop, multiple vectors)
- Integrity checks (double destruction detection, memory corruption)

All tests run with AddressSanitizer and UndefinedBehaviorSanitizer enabled.

## License

Apache License 2.0. See [LICENSE](LICENSE) for details.
