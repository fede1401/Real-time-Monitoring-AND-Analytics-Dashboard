#ifndef LISTENER_H
#define LISTENER_H

#include <sys/socket.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <errno.h>

#include "../common.h"
#include "../Parser/Marshal.h"
#include "../Model/Request.h"
#include "Agent.h"


class Listener
{
public:

    /**
     * Class constructor
    */
    Listener(const std::string &address, const int &port, Protocol prot);

    /**
     * Class deconstructor
     */
    ~Listener();

    void setParameterConnession();

    void bindSocket();

    void listenSocket();

    // Only for UDP
    void acceptSocket();

    void receiveData();

    /**
     * @brief Builds a complete HTTP/1.1 200 OK response with the given HTML body
     */
    std::string buildHTTPResponse(const std::string& html);

    /**
     * @brief Builds the dashboard HTML page with real system data injected
     */
    std::string buildDashboardHTML(const CPU& cpu,
                                   const RAM& ram,
                                   const std::vector<ActiveConnection>& connections,
                                   const std::vector<ProcessInfo>& process,
                                   const std::vector<DiskInfo>& disk);


    int serverSocket_;
    int clientSocket_;
    sockaddr_in serverAddress_;
    std::string address_;
    int port_;
    char buffer_[1024] = {0};

    Marshal* marshal_;                           ///< Marshal to obtain request HTTP
    Agent* agent_;

};

#endif // LISTENER_H

