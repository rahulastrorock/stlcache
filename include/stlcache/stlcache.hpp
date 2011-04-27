//
// Copyright (C) 2011 Denis V Chapligin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STLCACHE_STLCACHE_HPP_INCLUDED
#define STLCACHE_STLCACHE_HPP_INCLUDED

#ifdef _MSC_VER
#pragma warning( disable : 4290 )
#endif /* _MSC_VER */

#include <map>

using namespace std;

#include <stlcache/exceptions.hpp>
#include <stlcache/policy.hpp>
#include <stlcache/policy_lru.hpp>
#include <stlcache/policy_mru.hpp>
#include <stlcache/policy_lfu.hpp>
#include <stlcache/policy_lfustar.hpp>
#include <stlcache/policy_lfuaging.hpp>
#include <stlcache/policy_lfuagingstar.hpp>
#include <stlcache/policy_adaptive.hpp>

namespace stlcache {
    template<
        class Key, 
        class Data, 
        class Policy, 
        class Compare = less<Key>, 
        template <typename T> class Allocator = allocator 
    >
    class cache {
        typedef map<Key,Data,Compare,Allocator<pair<const Key, Data> > > storageType; 
         storageType _storage;
         std::size_t _maxEntries;
         std::size_t _currEntries;
         typedef typename Policy::template bind<Key,Allocator> policy_type;
         policy_type* _policy;
         Allocator<policy_type> policyAlloc;

    public:
        typedef Key                                                                key_type;
        typedef Data                                                               mapped_type;
        typedef pair<const Key, Data>                                         value_type;
        typedef Compare                                                          key_compare;
        typedef Allocator<pair<const Key, Data> >                          allocator_type;
        typedef typename storageType::value_compare                                value_compare;
        typedef typename storageType::reference                                        reference;
        typedef typename storageType::const_reference                               const_reference;
        typedef typename storageType::size_type                                       size_type;
        typedef typename storageType::difference_type                               difference_type;
        typedef typename storageType::pointer                                          pointer;
        typedef typename storageType::const_pointer                                 const_pointer;

        //map functions wrappers
        allocator_type get_allocator() const throw() {
            return _storage.get_allocator();
        }

        size_type count ( const key_type& x ) const throw() {
            return _storage.count(x);
        }

        value_compare value_comp ( ) const throw() {
            return _storage.value_comp();
        }

        key_compare key_comp ( ) const throw() {
            return _storage.key_comp();
        }

        //Cache API
        void clear() throw() {
            _storage.clear();
            _policy->clear();
            this->_currEntries=0;
        }

        void swap ( cache<Key,Data,Policy,Compare,Allocator>& mp ) throw() {
            _storage.swap(mp._storage);
            _policy->swap(*mp._policy);

            std::size_t m=this->_maxEntries;
            this->_maxEntries=mp._maxEntries;
            mp._maxEntries=m;

            this->_currEntries=this->size();
            mp._currEntries=mp.size();
        }

        size_type erase ( const key_type& x ) throw() {
            size_type ret=_storage.erase(x);
            _policy->remove(x);

            _currEntries--;

            return ret;
        }

        bool insert(Key _k, Data _d) throw(stlcache_cache_full) {
            while (this->_currEntries >= this->_maxEntries) {
                _victim<Key> victim=_policy->victim();
                if (!victim) {
                    throw stlcache_cache_full("The cache is full and no element can be expired at the moment. Remove some elements manually");
                }
                this->erase(*victim);
            }

            _policy->insert(_k);


            bool result=_storage.insert(value_type(_k,_d)).second;
            if (result) {
                _currEntries++;
            }

            return result;
        }

        size_type max_size() const throw() {
            return this->_maxEntries;
        }

        size_type size() const throw() {
            assert(this->_currEntries==_storage.size());
            return this->_currEntries;
        }

        bool empty() const throw() {
            return _storage.empty();
        }

        const Data& fetch(const Key& _k) throw(stlcache_invalid_key) {
            if (!check(_k)) {
                throw stlcache_invalid_key("Key is not in cache",_k);
            }
            _policy->touch(_k);
            return (*(_storage.find(_k))).second;
        }

        const bool check(const Key& _k) throw() {
            _policy->touch(_k);
            return _storage.find(_k)!=_storage.end();
        }

        void touch(const Key& _k) throw() {
            _policy->touch(_k);
        }

        //Container interface
        cache<Key,Data,Policy,Compare,Allocator>& operator= ( const cache<Key,Data,Policy,Compare,Allocator>& x) throw() {
            this->_storage=x._storage;
            this->_maxEntries=x._maxEntries;
            this->_currEntries=this->_storage.size();

            policy_type localPolicy(*x._policy);
            this->_policy = policyAlloc.allocate(1);
            policyAlloc.construct(this->_policy,localPolicy);
            return *this;
        }
        explicit cache(const size_type size, const Compare& comp = Compare()) throw() {
            this->_storage=storageType(comp, Allocator<pair<const Key, Data> >());
            this->_maxEntries=size;
            this->_currEntries=0;

            policy_type localPolicy(size);
            this->_policy = policyAlloc.allocate(1);
            policyAlloc.construct(this->_policy,localPolicy);
        }
        cache(const cache<Key,Data,Policy,Compare,Allocator>& x) throw() {
            *this=x;
        }
        //Clean-up
        ~cache() {
            policyAlloc.destroy(this->_policy);
            policyAlloc.deallocate(this->_policy,1);
        }
    };
}

#endif /* STLCACHE_STLCACHE_HPP_INCLUDED */