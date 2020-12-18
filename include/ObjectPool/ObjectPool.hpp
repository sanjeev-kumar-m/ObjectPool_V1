#pragma once 
/**
 * @file ObjectPool.hpp
 * @author Sanjeev kumar M
 * @date 12/12/2020 11:30 PM
 * 
 * This module defines Utils::ObjectPool class
 */


#define SNJ_MAKE_NONCOPYABLE(c)\
private:                       \
  c(const c&) noexcept = delete;\
  c& operator=(const c&) noexcept = delete

#define SNJ_MAKE_NONMOVABLE(c)\
private:                      \
  c(c&&) noexcept = delete;   \
  c& operator=(c&&) noexcept = delete

#include <stdint.h>
#include <utility>
#include <cstddef>
#include <vector>
#include <type_traits>

namespace Utils {
  template<typename T>
  class ObjectPool {
   public:
    struct Ptr {
      //This is the destructor
      ~Ptr() noexcept {
        if (this->_m_ptr) {
          --*(this->_m_ref_count);
          if (*this->_m_ref_count == 0) {
            delete this->_m_ptr;
            delete this->_m_ref_count;
          }
        }
      
      }
      //This is the copy constructor
      Ptr(const Ptr& __ptr) noexcept {
        this->_m_ptr = __ptr._m_ptr;
        ++*(__ptr._m_ref_count);
        this->_m_ref_count = __ptr._m_ref_count;
      }
			
      //This is the move constructor
      Ptr(Ptr&& __ptr) noexcept {
        this->_m_ptr = __ptr._m_ptr;
        this->_m_ref_count = __ptr._m_ref_count;
        __ptr._m_ptr = nullptr;
        __ptr._m_ref_count = nullptr;
      }

      //This is copy assignment operator
      Ptr& operator=(const Ptr& __ptr) {
        this->_m_ptr = __ptr._m_ptr;
        ++*(__ptr._m_ref_count);
        this->_m_ref_count = __ptr._m_ref_count;
        return *this;
      }
			
      //This is move assignment operator
      Ptr& operator=(Ptr&& __ptr) noexcept {
        this->_m_ptr = __ptr._m_ptr; 
        this->_m_ref_count = __ptr._m_ref_count;
        __ptr._m_ptr = nullptr;
        __ptr._m_ref_count = nullptr;
        return *this;
      }
      
      //This is the default constructor
      Ptr() 
        : _m_ptr(nullptr),
        _m_ref_count(nullptr)
      {}
      
      explicit Ptr(T *__t_ptr)
        : _m_ptr(__t_ptr),
          _m_ref_count(new uint32_t())
      {
        if (_m_ptr != nullptr) ++(*_m_ref_count);
      }

      //This is deferencing operator
      Ptr& operator*() {
        return *(this->_m_ptr);
      }
	
      //This is member access operator
      Ptr operator->() {
        return this->_m_ptr;
      }
      
      T* get() {
        return this->_m_ptr;
      }
      
      int32_t get_ref_count() {
        return *(this->_m_ref_count);
      }
      
      /**
       * This function constructs the object
       * 
       * @param[in] __args 
       * argument to the constructor of T
       */
      template<typename... Args>
      void construct(Args&&... __args) { 
        new((this->_m_ptr)) T(std::forward<Args>(__args)...);
      }
    
     private:
      /**
       * This is the pointer to the object instance
       */
	    T* _m_ptr;

      /**
       * This stores the pointer to the reference 
       * count object
       */
      uint32_t *_m_ref_count;
  };

  //Life cycle management
  ~ObjectPool() = default;
  SNJ_MAKE_NONCOPYABLE(ObjectPool);
  SNJ_MAKE_NONMOVABLE(ObjectPool);
   
 public:  
  //This is the constructor
  ObjectPool(size_t __init_capacity, size_t __grow_size) 
      : _m_grow_size(__grow_size),
        _m_vector_size(next_power_of_two(std::max(__init_capacity, __grow_size))),
        _m_mask(_m_vector_size -1),
        _m_first_full_slot(0),
        _m_first_empty_slot(0),
        _m_used_object_fill_index(0),
        _m_allocated_objects(_m_vector_size),
        _m_used_objects(_m_vector_size)
    {
      replenish(__init_capacity);
    }

    /**
     * This function is used to construct the object
     * from the given arguments
     * 
     * @param[in]
     *    __args -> arguments to the constructor of T
     * 
     * @return[in]
     *    returns a new Ptr object
     */
    template<typename... Args>
    Ptr allocate(Args&&... __args) {
      if ( _m_first_full_slot == _m_first_empty_slot) {
        replenish(_m_grow_size);
      }
      _m_allocated_objects[_m_first_full_slot & _m_mask]
            .construct(std::forward<Args>(__args)...);
      _m_used_objects[_m_used_object_fill_index] = std::move(
            _m_allocated_objects[_m_first_full_slot & _m_mask]);
          ++_m_first_full_slot;
      return _m_used_objects[_m_used_object_fill_index++];
    }
    
    /**
     * This function used to deallocate  unused 
     * objects
     */
    void gc() {
      for(size_t t_id = 0; t_id < _m_used_object_fill_index;) {
        //live objects
        if (_m_used_objects[t_id].get_ref_count() > 1) {
          t_id++;
          continue;
        }
        //check whether we have slot to bookkeep
        if ( _m_first_full_slot + _m_vector_size > _m_first_empty_slot) {
          _m_allocated_objects[_m_first_empty_slot & _m_mask] 
              = std::move(_m_used_objects[t_id]);
          ++_m_first_empty_slot;
        }
        else {
          //destroy the object
          _m_used_objects[t_id].~Ptr();
        }
        _m_used_objects[t_id] = std::move(_m_used_objects[--_m_used_object_fill_index]);
      }
    }

    /**
     * This function used to allocate new objects,
     * called when we runout of preallocated number 
     * of objects 
     * 
     * @param[in] 
     *      __num_objects - number of objects to allocate
     */
    void replenish(size_t __num_objects) {
      for(size_t t_index = 0; t_index < __num_objects; ++t_index) {
        _m_allocated_objects[_m_first_empty_slot & _m_mask] 
                      = std::move(Ptr(new T()));
        ++_m_first_empty_slot;
      }

      //check whether we can accomodate, if we allocate all the objects 
      //used object, 
      size_t t_free_slots = _m_used_objects.size() - _m_used_object_fill_index;
      if (t_free_slots < __num_objects) {
        _m_used_objects.resize(_m_used_objects.size() + (__num_objects - t_free_slots));
      }
    }

    /**
     * This function used to get the next power of 2 
     *  
     * @param[in]
     *    __val - value for which to get next power
     * @return
     *    next power of 2
     */
    static size_t next_power_of_two(size_t __val) {
      --__val;
      __val |= __val >> 1;
      __val |= __val >> 2;
      __val |= __val >> 4;
      __val |= __val >> 8;
      __val |= __val >> 16;
      ++__val;
      return __val;
    }    

    /**
     * This stores number of objects we need to 
     * grow when we run out of preallocated objects
     */
    size_t _m_grow_size; 

    /**
     * This is the size of the vector we use to maitain
     * preallocated objects
     * 
     * todo: currently it's max of growsize and init_size
     */
    size_t _m_vector_size;

    /**
     * This holds the mask to compute the the index
     */
    size_t _m_mask;

    /**
     * This holds the index which points to the location
     * in _m_allocated_objects from which new object can 
     * be constructed
     */
    size_t _m_first_full_slot;

    /**
     * This holds the index which points to the location in  
     * the _m_allocated_objects where we can refill
     */
    size_t _m_first_empty_slot;

    /**
     * This holds the index which points to location 
     * in the _m_used_objects where new allocated object
     * info can be stored, it's basically move
     */
    size_t _m_used_object_fill_index;

    /**
      * This vector contains preallocated objects
      */
    std::vector<Ptr> _m_allocated_objects;

    /**
     *This vector contains used/currently being
     * used objects
     */
    std::vector<Ptr> _m_used_objects;
  };
}
