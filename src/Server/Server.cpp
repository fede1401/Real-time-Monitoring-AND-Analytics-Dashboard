#include "Server.h"
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

Server::Server()
{
    agent_ = new Agent();

    httplib::Server svr;

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream file(std::string(WEB_ROOT) + "/index.html");
        if (file) {
            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>()
            );
            res.set_content(content, "text/html");
        } else {
            res.status = 404;
            res.set_content("index.html not found", "text/plain");
        }
    });

    svr.Get("/metrics", [&](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        res.set_chunked_content_provider("text/event-stream",
            [&](size_t, httplib::DataSink& sink) -> bool {
                CPU cpu                                    = agent_->takeInfoCPU();
                RAM ram                                    = agent_->takeInfoRAM();
                std::vector<ActiveConnection> connections  = agent_->takeInfoConnection();
                std::vector<DiskInfo> infoDisk = agent_->takeDiskInfos();
                std::vector<ProcessInfo> infoProcess = agent_->takeInfoProcess();

                std::string msg = "data: " + buildMetricsJSON(cpu, ram, connections, infoProcess, infoDisk) + "\n\n";

                if (!sink.write(msg.c_str(), msg.size()))
                    return false; // client disconnected

                std::this_thread::sleep_for(std::chrono::seconds(1));
                return true;
            }
        );
    });

    svr.listen("0.0.0.0", 8080);
}

std::string Server::buildMetricsJSON(
    const CPU& cpu,
    const RAM& ram,
    const std::vector<ActiveConnection>& connections,
    const std::vector<ProcessInfo>& processes,
    const std::vector<DiskInfo>& disks)
{
    std::ostringstream j;

    j << "{"
      << "\"cpu\":{"
      << "\"architecture\":\"" << cpu.architecture << "\","
      << "\"cores\":"          << cpu.cores         << ","
      << "\"vendor\":\""       << cpu.vendor        << "\""
      << "},"
      << "\"ram\":{"
      << "\"total\":\""        << ram.total      << "\","
      << "\"used\":\""         << ram.used       << "\","
      << "\"buffCache\":\""    << ram.buffCache  << "\","
      << "\"available\":\""    << ram.available  << "\""
      << "},"
      << "\"connections\":[";

    for (size_t i = 0; i < connections.size(); i++) {
        const auto& c = connections[i];
        j << "{"
          << "\"protocol\":\""      << (c.protocol == Protocol::TCP ? "TCP" : "UDP") << "\","
          << "\"serverAddress\":\"" << c.serverAddress << "\","
          << "\"serverPort\":\""    << c.serverPort    << "\","
          << "\"clientAddress\":\"" << c.clientAddress << "\","
          << "\"clientPort\":\""    << c.clientPort    << "\","
          << "\"state\":\""         << c.state         << "\","
          << "\"pidProgram\":\""    << c.pidProgram    << "\""
          << "}";
        if (i + 1 < connections.size()) j << ",";
    }

    j << "],\"processes\":[";

    for (size_t i = 0; i < processes.size(); i++) {
        const auto& p = processes[i];
        j << "{"
          << "\"user\":\""    << p.user    << "\","
          << "\"pid\":"       << p.pid     << ","
          << "\"percCPU\":"   << p.percCPU << ","
          << "\"percRAM\":"   << p.percRAM << ","
          << "\"start\":\""   << p.start   << "\","
          << "\"time\":\""    << p.time    << "\","
          << "\"command\":\"" << p.command << "\""
          << "}";
        if (i + 1 < processes.size()) j << ",";
    }

    j << "],\"disks\":[";

    for (size_t i = 0; i < disks.size(); i++) {
        const auto& d = disks[i];
        j << "{"
          << "\"fileSystem\":\"" << d.fileSystem  << "\","
          << "\"size\":\""       << d.size        << "\","
          << "\"used\":\""       << d.used        << "\","
          << "\"available\":\""  << d.available   << "\","
          << "\"use\":\""        << d.use         << "\","
          << "\"mountedOn\":\""  << d.mountedOn   << "\""
          << "}";
        if (i + 1 < disks.size()) j << ",";
    }

    j << "]}";
    return j.str();
}
