/*
* Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __MULTI_CORE_INSTANTIATOR_H__
#define __MULTI_CORE_INSTANTIATOR_H__

#include <stdint.h>
#include<map>

namespace sdm {

template<typename Key, typename Value>
using MultiCoreIterator = typename std::map<Key, Value>::const_iterator;

template<typename Key, typename Value>
class MultiCoreInstance {
 public:
  MultiCoreInstance() { }

  void Insert(Key k, Value v) { mp[k] = v; }

  MultiCoreIterator<Key, Value> Find(Key key) { return mp.find(key); }

  MultiCoreIterator<Key, Value> End() { return mp.end(); }

  MultiCoreIterator<Key, Value> Begin() { return mp.begin(); }

  uint32_t Size() { return mp.size(); }

  bool Empty() { return mp.empty(); }

  Value &operator[](int index) { return mp[index]; }

  Value &At(int index) { return mp[index]; }

  void Erase(const Key &key) { mp.erase(key); }

  void Erase(MultiCoreIterator<Key, Value> position) { mp.erase(position); }

  void Clear() { mp.clear(); }

 private:
  std::map<Key, Value> mp;
};

}  // namespace sdm

#endif  // __MULTI_CORE_INSTANTIATOR_H__
