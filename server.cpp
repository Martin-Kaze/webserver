#include <iostream>
#include <sys/socket.h>
#include <cstdio>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <optional>
#include <string_view>
#include <utility> 

struct Response
{

    int code;
    std::string_view reason;
    std::string_view content_type;
    std::string body;
};

std::string_view reason(int code)
{
    switch (code)
    {
    case 200:
        return "OK";

    case 405:
        return "Method Not Allowed";

    case 400:
        return "Bad Request";

    case 404:
        return "Not Found";

    default:
        return "Unknown";
    }
}
std::optional<std::pair<std::string_view, std::string_view>>
split(std::string_view v, std::string_view c)
{
    auto pos = v.find(c);
    if (pos == std::string_view::npos)
    {
        return std::nullopt;
    }
    return std::pair{v.substr(0, pos), v.substr(pos + c.size())};
}
Response handleRes(std::string_view req)
{

    Response res;

    res.content_type = "text/plain";

    auto finish = [&](int code, std::string body)
    {
        res.code = code;
        res.reason = reason(code);
        res.body = std::move(body);
        return res;
    };

    auto line = split(req, "\r\n");
    if (!line)
    {
        return finish(400, "400 Bad Request\n");
    }
    auto [first_line, other] = *line;

    auto a = split(first_line, " ");
    if (!a)
    {
        return finish(400, "400 Bad Request\n");
    }
    auto [method, rest] = *a;

    auto b = split(rest, " ");
    if (!b)
    {
        return finish(400, "400 Bad Request\n");
    }
    auto [path, version] = *b;

    if( method != "GET"){
        return finish(405, "405 Method Not Allowed\n");
    }

    if(!version.starts_with("HTTP/1.")){
        return finish(400,"400 Bad Request\n");
    }

    if(path.empty() || path.front() != '/'){
         return finish(400,"400 Bad Request\n");
    }

    return  finish(200," Hello world\n");
}

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

        std::string_view view(&reqBuff[0], n);

        const Response res = handleRes(view);

        response = "HTTP/1.1 " + std::to_string(res.code) + " " + std::string(res.reason) + "\r\n"
                                                                                            "Content-Type: " + std::string(res.content_type) + "\r\n"
                                                                                            "Content-Length: " +
                   std::to_string(res.body.size()) + "\r\n"
                                                     "\r\n" +
                   res.body;

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