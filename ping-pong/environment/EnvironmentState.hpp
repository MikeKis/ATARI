#ifndef ENVIRONMENTSTATE_HPP
#define ENVIRONMENTSTATE_HPP

#include <memory>
#include <string>
#include <utility>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "../ping-pong-environment.h"

class EnvironmentState
{
    std::unique_ptr<boost::interprocess::shared_memory_object> shm;
    std::unique_ptr<boost::interprocess::mapped_region>        region;
    std::string                                                strSharedMemoryName;
    std::pair<std::pair<float, float>, float>                  pprrr_LocalState;
public:
    std::pair<float, float> *pprr_Ball;
    float                   *prRacket = NULL;
    EnvironmentState()
    {
        strSharedMemoryName = ENVIRONMENT_STATE_SHARED_MEMORY_NAME;
        bool bExists = true;
        try {
            shm.reset(new boost::interprocess::shared_memory_object(boost::interprocess::open_only, strSharedMemoryName.c_str(), boost::interprocess::read_write));
        } catch (...) {
            bExists = false;
        }
        if (bExists) {
            //Map the whole shared memory in this process
            region.reset(new boost::interprocess::mapped_region(*shm, boost::interprocess::read_write));
            pprr_Ball = (std::pair<float, float> *)region->get_address();
        } else pprr_Ball = &pprrr_LocalState.first;
    }
    void ResetBall(bool bPunishment = true);
    void ResetRacket();
};

#endif // ENVIRONMENTSTATE_HPP
