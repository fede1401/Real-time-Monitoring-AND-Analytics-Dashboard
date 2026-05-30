#ifndef SERVER_MY_H
#define SERVER_MY_H

#include "httplib.h"
#include "../includeClass.h"
#include "../common.h"
#include "../Model/SystemMetrics.h"
#include "Agent.h"

class Server
{
    public:
        Server();

    private:
        Agent* agent_;

        std::string buildMetricsJSON(
            const CPU& cpu,
            const RAM& ram,
            const std::vector<ActiveConnection>& connections,
            const std::vector<ProcessInfo>& processes,
            const std::vector<DiskInfo>& disks
        );
};

#endif
