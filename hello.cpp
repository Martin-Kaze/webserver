#include <iostream>
#include <sys/socket.h>
#include <cstdio>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

int main()
{

    constexpr int PORT = 8080;

    sockaddr_in addr{};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    int val = 1;
    int sp = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    if (sp == -1)
    {
        perror("socket option");
        close(server_fd);
        return 1;
    }

    int bin = bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));

    if (bin == -1)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    int list = listen(server_fd, SOMAXCONN);

    if (list == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Listening to port " << PORT << std::endl;

    while (true)
    {

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
        if (client_fd == -1)
        {
            perror("accept");
            continue;
        }
        char buffer[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &client_addr.sin_addr, buffer, sizeof(buffer));
        std::cout << "Client connected: " << buffer << ":" << ntohs(client_addr.sin_port) << "\n";

        char reqBuff[4096];
        std::string response;

        ssize_t n = read(client_fd, reqBuff, sizeof(reqBuff));

        if (n == -1)
        {
            perror("read");
            close(client_fd);
            continue;
        }

        std::string body = "Hello from the server\n";

        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/plain\r\n"
                   "Content-Length: " +
                   std::to_string(body.size()) + "\r\n"
                                                 "\r\n" +
                   body;

        ssize_t written = write(client_fd, response.data(), response.size());

        if (written == -1)
        {
            perror("write");
            close(client_fd);
            continue;
        }

        close(client_fd);
    }
}