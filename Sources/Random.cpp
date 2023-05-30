#include "Random.hpp"

#include <boost/fiber/mutex.hpp>

/************************************************************************/

void SteamBot::Random::makeRandomNumber(std::function<void(SteamBot::Random::Generator&)> callback)
{
    static boost::fibers::mutex mutex;
    static Generator generator((std::random_device())());

    std::lock_guard<decltype(mutex)> lock(mutex);
    callback(generator);
}

/************************************************************************/

SteamBot::Random::Generator::result_type SteamBot::Random::generateRandomNumber()
{
    Generator::result_type result;
    makeRandomNumber([&result](Generator& generator) {
        result=generator();
    });
    return result;
}
