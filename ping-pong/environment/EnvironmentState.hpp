// .specs/2026-10-13_ping-pong-imitation-learning.md
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
    SharedEnvironmentState                                     LocalState = {};
public:
    SharedEnvironmentState  *pst = NULL;
    std::pair<float, float> *pprr_Ball = NULL;
    float                   *prRacket = NULL;
    bool                     bReady = false;
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
            region.reset(new boost::interprocess::mapped_region(*shm, boost::interprocess::read_write));
            pst = (SharedEnvironmentState *)region->get_address();
        } else pst = &LocalState;
        pprr_Ball = (std::pair<float, float> *)&pst->x;
        prRacket = &pst->racket_y;
    }
    void ResetBall(bool bPunishment = true);
    void ResetRacket();
};

#endif // ENVIRONMENTSTATE_HPP
