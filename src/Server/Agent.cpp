#include "Agent.h"

Agent::Agent() {}

CPU Agent::takeInfoCPU()
{
    CPU infoCPU{};

    std::string result = execCommand("lscpu");

    std::vector<std::string> lines = ManageString::splitString(result, '\n');

    for (auto& line: lines)
    {
        std::vector<std::string> info = ManageString::splitString(line, ':');

        if (info[0] == "Architecture")
        {
            infoCPU.architecture = ManageString::lstrip(info[1]);
        }
        else if (info[0] == "CPU(s)")
        {
            infoCPU.cores = std::stoi(ManageString::lstrip(info[1]));
        }
        else if (info[0] == "Vendor ID")
        {
            infoCPU.vendor = ManageString::lstrip(info[1]);
        }
    }

    // std::cout << result << std::endl;
    return infoCPU;

}


RAM Agent::takeInfoRAM()
{
    RAM infoRAM{};

    std::string result = execCommand("free -h");

    std::vector<std::string> lines = ManageString::splitString(result, '\n');

    std::vector<std::string> info = ManageString::splitString(lines[1], ' ');

    std::vector<std::string> infoRAMVector = {};

    if (info[0] == "Mem:")
    {
        for (auto& value: info)
        {
            auto char_i = static_cast<uint8_t>(value[0]);
            if (char_i != 0x00)
            {
                infoRAMVector.push_back(value);
            }
        }
    }

    infoRAM.total = infoRAMVector[1];
    infoRAM.used = infoRAMVector[2];
    infoRAM.buffCache = infoRAMVector[5];
    infoRAM.available = infoRAMVector[6];

    return infoRAM;


}

std::vector<ActiveConnection> Agent::takeInfoConnection()
{
    std::vector<ActiveConnection> activeConnections{};

    std::string result = execCommand("netstat -tulpn 2>/dev/null");

    std::vector<std::string> lines = ManageString::splitString(result, '\n');

    for (int i = 2; i< lines.size(); i++)
    {
        ActiveConnection activeConnection;

        std::vector<std::string> line = ManageString::splitString(lines[i], ' ');
        std::vector<std::string> lineFinal{};

        for (const auto& element: line)
        {
            if (element != "")
                lineFinal.push_back(element);
        }

        if (lineFinal[0] == "tcp")
            activeConnection.protocol = Protocol::TCP;

        if (lineFinal[0] == "udp")
            activeConnection.protocol = Protocol::UDP;

        // Server address
        std::vector<std::string> addressPortServer = ManageString::splitString(lineFinal[3], ':');

        activeConnection.serverAddress = addressPortServer[0];
        activeConnection.serverPort = addressPortServer[1];

        // Client address
        std::vector<std::string> addressPortClient = ManageString::splitString(lineFinal[4], ':');

        activeConnection.clientAddress = addressPortClient[0];
        activeConnection.clientPort = addressPortClient[1];


        activeConnection.state = lineFinal[5];
        activeConnection.pidProgram = lineFinal[6];

        activeConnections.push_back(activeConnection);
    }

    return activeConnections;
}


std::vector<ProcessInfo> Agent::takeInfoProcess()
{
    std::vector<ProcessInfo> processInfoVect{};

    std::string result = execCommand("ps aux --sort=-%cpu,-%mem | head -10");

    std::vector<std::string> lines = ManageString::splitString(result, '\n');

    for (int i = 1; i< lines.size(); i++)
    {
        ProcessInfo processInfo;

        std::vector<std::string> line = ManageString::splitString(lines[i], ' ');
        std::vector<std::string> lineFinal{};

        for (const auto& element: line)
        {
            if (element != "")
                lineFinal.push_back(element);
        }

        processInfo.user = lineFinal[0];
        processInfo.pid = std::stoi(lineFinal[1]);
        processInfo.percCPU = std::stod(lineFinal[2]);
        processInfo.percRAM = std::stod(lineFinal[3]);
        processInfo.start = lineFinal[8];
        processInfo.time = lineFinal[9];
        processInfo.command = lineFinal[10];

        processInfoVect.push_back(processInfo);
    }

    return processInfoVect;
}

std::vector<DiskInfo> Agent::takeDiskInfos()
{
    std::vector<DiskInfo> diskInfos{};

    std::string result = execCommand("df -h");

    std::vector<std::string> lines = ManageString::splitString(result, '\n');

    for (int i = 1; i< lines.size(); i++)
    {
        DiskInfo diskInfo;

        std::vector<std::string> line = ManageString::splitString(lines[i], ' ');
        std::vector<std::string> lineFinal{};

        for (const auto& element: line)
        {
            if (element != "")
                lineFinal.push_back(element);
        }

        diskInfo.fileSystem = lineFinal[0];
        diskInfo.size = lineFinal[1];
        diskInfo.used = lineFinal[2];
        diskInfo.available = lineFinal[3];
        diskInfo.use = lineFinal[4];
        diskInfo.mountedOn = lineFinal[5];

        diskInfos.push_back(diskInfo);
    }

    return diskInfos;
}


std::string Agent::execCommand(const std::string& command)
{
    std::string result;                                             ///< Final result string that will contain the command output
    char buffer[128];                                               ///< Temporary buffer to read chunks of output

    // Open a pipe to the command in read mode ("r")
    // popen() forks a process and executes the command in a shell
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";                                           ///< If pipe creation failed, return empty string

    // Read the output chunk by chunk (128 bytes at a time)
    // fgets() reads until newline or buffer is full
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;                                           ///< Append each chunk to the result string
    }

    // Close the pipe and free resources
    // pclose() also waits for the process to terminate
    pclose(pipe);

    return result;                                                  ///< Return the complete output
}
