#ifndef __GPU_MEM_H
#define __GPU_MEM_H
 
#include <cstddef>
#include <cuda_runtime.h>
#include <assert.h>
#include <vector>
/* TODO */
#include <ostream>

#include "gpu_util.h"


template <class T>
struct gpu_managed_alloc {
    typedef T value_type;
    gpu_managed_alloc(){};
    template <class U> gpu_managed_alloc(const gpu_managed_alloc<U>& other){}
    T* allocate(std::size_t n){
        T *ret = NULL;
        CUDA_CHECK(cudaMallocManaged(&ret, sizeof(T) * n));
        /* TODO: proper exceptions */
        assert(ret);
        return ret;
    };
    void deallocate(T* p, std::size_t n){
        CUDA_CHECK(cudaFree(p));
    };
};
template <class T, class U>
bool operator==(const gpu_managed_alloc<T>&, const gpu_managed_alloc<U>&){
    return true; }
template <class T, class U>
bool operator!=(const gpu_managed_alloc<T>&, const gpu_managed_alloc<U>&){
    return false; }

template <class T>
struct gvector : std::vector<T, gpu_managed_alloc<vec> >{
    gvector() : std::vector<T, gpu_managed_alloc<vec> >(){};
    gvector(std::size_t s) : std::vector<T, gpu_managed_alloc<vec> >(s){}

    __device__ __host__
    T &operator[](int a){
        /* TODO: heh heh */
        return ((T *) this->_M_impl._M_start)[a];
    };

    __device__ __host__
    const T &operator[](int a) const{
        /* TODO: heh heh */
        return ((T *) this->_M_impl._M_start)[a];
    };

};

class gpu_visible {
public:
    void *operator new(size_t len) {
        void *ptr = NULL;
        CUDA_CHECK(cudaMallocManaged(&ptr, len));
        /* TODO */
        assert(ptr);
        return ptr;
    }

    void operator delete(void *ptr) {
        CUDA_CHECK(cudaFree(ptr));
    }
};



#endif
