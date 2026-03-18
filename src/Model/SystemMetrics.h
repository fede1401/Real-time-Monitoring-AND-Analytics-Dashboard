#ifndef SYSTEMMETRICS_H
#define SYSTEMMETRICS_H

#include <string>
#include "../common.h"


struct CPU
{
    std::string architecture;
    int cores;
    std::string vendor;
};


struct RAM
{
    std::string total;
    std::string used;
    std::string buffCache;
    std::string available;
};



struct ActiveConnection
{
    Protocol protocol;
    std::string serverAddress;          // internal
    std::string serverPort;
    std::string clientAddress;          // external
    std::string clientPort;
    std::string state;
    std::string pidProgram;
};

struct NetworkInfo
{
    ActiveConnection activeConnection;

};

struct ProcessInfo
{
    std::string user;
    unsigned int pid;
    double percCPU;
    double percRAM;
    std::string start;
    std::string time;
    std::string command;
};


struct DiskInfo
{
    std::string fileSystem;
    std::string size;
    std::string used;
    std::string available;
    std::string use;
    std::string mountedOn;
};


class SystemMetrics
{

    void setInfoCPU(CPU infoCpu)
    {
        this->infoCpu_ = infoCpu;
    }

    CPU getInfoCPU()
    {
        return infoCpu_;
    }

    void setInfoRAM(RAM infoRam)
    {
        this->infoRam_ = infoRam;
    }

    RAM getInfoRAM()
    {
        return infoRam_;
    }




private:
    CPU infoCpu_;
    RAM infoRam_;


};


#endif // SYSTEMMETRICS_H
