#ifndef _FASTMAP_H_INCLUDED
#define _FASTMAP_H_INCLUDED

#include <mutex>
#include <shared_mutex>
#include <vector>
#include <memory>
#include <iterator>
#include <algorithm>
#include <atomic>
#include <map>

class MutexPool final
{
public:
    explicit MutexPool(size_t size)
        :mutexs_()
        ,index_(0)
    {
        std::generate_n(std::back_inserter(mutexs_), size, std::make_shared<std::mutex>);
    }

    ~MutexPool() = default;

    MutexPool(const MutexPool&) = delete;
    MutexPool& operator =(const MutexPool&) = delete;

    std::shared_ptr<std::mutex> getMutex() const
    {
        return mutexs_[index_++ % mutexs_.size()];
    }

private:
    std::vector<std::shared_ptr<std::mutex>> mutexs_;
    mutable std::atomic_size_t index_;
};


template<typename T>
class MutexSharedPtr : public std::shared_ptr<T>
{
public:
    MutexSharedPtr() = default;

    MutexSharedPtr(const std::shared_ptr<T>& v, const MutexPool& mutex_pool)
        :std::shared_ptr<T>(v)
        ,mu_(mutex_pool.getMutex())
    {}

protected:
    std::shared_ptr<std::mutex> mu_;
};

template<typename T>
class LockPtr : public MutexSharedPtr<T>
{
public:
    LockPtr() = default;
    LockPtr(LockPtr<T>&&) = default;
    LockPtr(const LockPtr<T>&) = delete;
    LockPtr& operator= (const LockPtr<T>&) = delete;

    LockPtr(MutexSharedPtr<T>& ptr)
        :MutexSharedPtr<T>(ptr)
    {
        mu_->lock();
    }

    ~LockPtr()
    {
        if (mu_)
        {
            mu_->unlock();
        }
    }
};


template<typename K, typename V>
class FastMap : private std::map<K, MutexSharedPtr<V>>
{
public:
    using Base = std::map<K, MutexSharedPtr<V>>;

    explicit FastMap(size_t size)
        :Base()
        ,mutex_pool_(size)
    {}

    LockPtr<V> find(const K& key)
    {
        std::shared_lock<std::shared_mutex> guard(mu_);
        auto it = Base::find(key);
        if (Base::end() == it)
        {
            return {};
        }
        return LockPtr<V>(it->second);
    }

    template<typename...Args>
    bool insert(const K& key, Args&&...args)
    {
        std::unique_lock<std::shared_mutex> guard(mu_);
        return Base::insert(Base::value_type(key, MutexSharedPtr<V>(std::make_shared<V>(std::forward<Args>(args)...), mutex_pool_))).second;
    }

    bool erase(const K& key)
    {
        std::unique_lock<std::shared_mutex> guard(mu_);
        return 0 != Base::erase(key);
    }

private:
    mutable std::shared_mutex mu_;
    MutexPool mutex_pool_;
};

#endif //_FASTMAP_H_INCLUDED
