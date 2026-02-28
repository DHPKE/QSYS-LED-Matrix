// web_server.h - Simple HTTP server for network configuration

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <string>
#include <functional>

class WebServer {
public:
    WebServer(int port = 8080);
    ~WebServer();
    
    void start();
    void stop();
    
private:
    int port_;
    int server_fd_;
    bool running_;
    
    void handleClient(int client_fd);
    std::string handleRequest(const std::string& method, const std::string& path, const std::string& body);
    std::string getConfigPage();
    std::string getCurrentConfig();
    bool saveConfig(const std::string& json);
    
    static void* serverThread(void* arg);
};

#endif // WEB_SERVER_H
