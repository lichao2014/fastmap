/*! \file main.cpp
    \brief a test cpp file
    \date 20161211
    \author lichao
    \version 1.0

    only for test
*/
#include "fastmap.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>

class ThreadGroup
{
public:
    void addThread(std::thread&& th)
    {
        std::lock_guard<std::mutex> guard(mu_);
        threads_.push_back(std::move(th));
    }

    template<typename...Args>
    void createThread(Args&&...args)
    {
        std::lock_guard<std::mutex> guard(mu_);
        threads_.emplace_back(std::forward<Args>(args)...);
    }

    void joinAll()
    {
        std::lock_guard<std::mutex> guard(mu_);
        for (auto& th : threads_)
        {
            th.join();
        }
    }

private:
    std::mutex mu_;
    std::vector<std::thread> threads_;
};

int main()
{
    FastMap<int, std::string> map(2);

    ThreadGroup tg;

    /*
    tg.createThread([&map] {
        while (true)
        {
            map.erase(1);
            map.insert(1, "lichao");
            map.erase(2);
            map.insert(2, "lichao");
        }
    });

    tg.createThread([&map] {
        while (true)
        {
            map.erase(2);
            map.insert(2, "lichao");
            map.erase(1);
            map.insert(1, "lichao");
        }
    });
    */

    map.insert(1, "lichao");
    map.insert(2, "lichao");

    tg.createThread([&map] {
        auto v = map.find(1);
        while (v)
        {
            
            //if (v)
            {
                v->assign("xx");
            }
        }
    });

    tg.createThread([&map] {
        auto v = map.find(2);
        while (v)
        {

            //if (v)
            {
                v->assign("xx");
            }
        }
    });

    std::cin.get();
    tg.joinAll();



    return 0;
}

