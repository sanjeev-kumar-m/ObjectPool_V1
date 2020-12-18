#include <gtest/gtest.h>
#include <ObjectPool/ObjectPool.hpp>

TEST(ObjectPoolTests, ShouldAllocateValidObjectsForPrimitiveType) {
  Utils::ObjectPool<int> memory_pool(4, 2);
  for(int i = 0; i < 6; i++) {
    Utils::ObjectPool<int>::Ptr ptr1 = memory_pool.allocate(i);
    ASSERT_NE(ptr1.get(), nullptr);
    ASSERT_EQ(*(ptr1.get()), i);
  }
}

TEST(ObjectPoolTests, ShouldAllocateValidObjectsForNonPrimitiveTypes) {
  struct TestStruct {
    uint64_t member1;
    double member2;
    TestStruct() = default;
    TestStruct(uint64_t __member1, double __member2)
      : member1(__member1),
        member2(__member2)
        {}
  };
  Utils::ObjectPool<TestStruct> memory_pool(4, 2);
  for(int i =0; i < 6; i++) {
    Utils::ObjectPool<TestStruct>::Ptr ptr = memory_pool.allocate(i, i + 10);
    ASSERT_NE(ptr.get(), nullptr);
    ASSERT_EQ((ptr.get()->member1), i);
    ASSERT_EQ((ptr.get()->member2), i+ 10);
  }
}

TEST(ObjectPoolTests, ShouldReuseUnusedObjects) {
  std::vector<int*> ptrs;
  Utils::ObjectPool<int> memory_pool(4, 2);
  for(int i = 0; i < 4; i++) {
    Utils::ObjectPool<int>::Ptr ptr1 = memory_pool.allocate(i);
    ptrs.push_back(ptr1.get());
  }
  memory_pool.gc();
  Utils::ObjectPool<int>::Ptr ptr1 = memory_pool.allocate(10);
  Utils::ObjectPool<int>::Ptr ptr2 = memory_pool.allocate(11);
  Utils::ObjectPool<int>::Ptr ptr3 = memory_pool.allocate(12);
  Utils::ObjectPool<int>::Ptr ptr4 = memory_pool.allocate(13);
  ASSERT_EQ(ptr1.get(), ptrs[0]);
  ASSERT_EQ(ptr2.get(), ptrs[3]);
  ASSERT_EQ(ptr3.get(), ptrs[2]);
  ASSERT_EQ(ptr4.get(), ptrs[1]);
}

