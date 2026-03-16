#ifndef AGENT_H
#define AGENT_H

#include "../includeClass.h"
#include "../Model/SystemMetrics.h"
#include "../Utility/ManageString.h"
#include "../LogPrint/Logint.h"

class Agent
{
public:
    Agent();

    CPU takeInfoCPU();

    RAM takeInfoRAM();

    std::vector<ActiveConnection> takeInfoConnection();

    std::string takeInfoProcess();

    /**
     * @brief Executes a shell command and returns its output
     * @param command : the shell command to execute
     * @return string containing the full output of the command, empty string if failed
    */
    std::string execCommand(const std::string& command);
};

#endif // AGENT_H
