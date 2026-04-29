/*
 * ------------------------------------------------------------------------------------
 *  httplib.h  —  Minimal Cross-Platform HTTP Server
 *
 *  Works on Windows (Winsock2) AND Linux/Mac (POSIX).
 *  Single header, zero external dependencies.
 *
 *  Usage:
 *    Server svr;
 *    svr.Get("/path",  handler);
 *    svr.Post("/path", handler);
 *    svr.Options(".*", handler);
 *    svr.listen("0.0.0.0", 8080);
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstring>

// Platform detection & socket headers ──────
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET   sock_t;
  #define SOCK_INVALID  INVALID_SOCKET
  #define SOCK_ERR      SOCKET_ERROR
  #define sock_close(s) closesocket(s)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <signal.h>
  typedef int      sock_t;
  #define SOCK_INVALID  (-1)
  #define SOCK_ERR      (-1)
  #define sock_close(s) close(s)
#endif

using namespace std;

namespace httplib {

struct Request {
    string method;
    string path;
    string body;
    unordered_map<string,string> params;

    string get_param_value(const string& key) const {
        auto it = params.find(key);
        return it != params.end() ? it->second : "";
    }
};

struct Response {
    int    status = 200;
    string body;
    unordered_map<string,string> headers;
    void set_header(const string& k, const string& v) { headers[k] = v; }
};

using Handler = function<void(const Request&, Response&)>;
struct Route { string method; string path; Handler handler; };

class Server {
public:
    void Get    (const string& p, Handler h) { routes_.push_back({"GET",    p, h}); }
    void Post   (const string& p, Handler h) { routes_.push_back({"POST",   p, h}); }
    void Options(const string& p, Handler h) { routes_.push_back({"OPTIONS",p, h}); }

    bool listen(const string& /*host*/, int port) {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
            cerr << "WSAStartup failed\n"; return false;
        }
#else
        signal(SIGPIPE, SIG_IGN);
#endif
        sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
        if (srv == SOCK_INVALID) { cerr << "socket() failed\n"; return false; }

        int opt = 1;
        setsockopt(srv, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&opt), sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons((u_short)port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(srv, (sockaddr*)&addr, sizeof(addr)) == SOCK_ERR) {
            cerr << "bind() failed — is port " << port << " already in use?\n";
            sock_close(srv); return false;
        }
        if (::listen(srv, 64) == SOCK_ERR) {
            cerr << "listen() failed\n"; sock_close(srv); return false;
        }
        cout << "  Listening on port " << port << "\n";

        while (true) {
            sock_t client = accept(srv, nullptr, nullptr);
            if (client == SOCK_INVALID) continue;
            
            // Bypass the thread error entirely! 
            // Handle the connection directly and synchronously:
            handleConn(client);
        }
#ifdef _WIN32
        WSACleanup();
#endif
        return true;
    }

private:
    vector<Route> routes_;

    static string urlDecode(const string& s) {
        string out;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '%' && i+2 < s.size()) {
                out += (char)stoi(s.substr(i+1,2), nullptr, 16);
                i += 2;
            } else if (s[i] == '+') out += ' ';
            else out += s[i];
        }
        return out;
    }

    static void parseQuery(const string& qs, unordered_map<string,string>& p) {
        istringstream ss(qs); string tok;
        while (getline(ss, tok, '&')) {
            auto eq = tok.find('=');
            if (eq == string::npos) { p[urlDecode(tok)] = ""; continue; }
            p[urlDecode(tok.substr(0,eq))] = urlDecode(tok.substr(eq+1));
        }
    }

    static string readHeaders(sock_t fd) {
        string buf;
        char c;
        while (buf.size() < 65536) {
            int n = recv(fd, &c, 1, 0);
            if (n <= 0) break;
            buf += c;
            if (buf.size() >= 4 &&
                buf[buf.size()-4]=='\r' && buf[buf.size()-3]=='\n' &&
                buf[buf.size()-2]=='\r' && buf[buf.size()-1]=='\n')
                break;
        }
        return buf;
    }

    void handleConn(sock_t fd) {
#ifdef _WIN32
        DWORD tv = 5000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&tv), sizeof(tv));
#else
        struct timeval tv{5, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&tv), sizeof(tv));
#endif
        string raw = readHeaders(fd);
        if (raw.empty()) { sock_close(fd); return; }

        istringstream stream(raw);
        string reqLine;
        getline(stream, reqLine);
        if (!reqLine.empty() && reqLine.back() == '\r') reqLine.pop_back();

        istringstream rl(reqLine);
        Request req;
        string fullPath, ver;
        rl >> req.method >> fullPath >> ver;

        auto qpos = fullPath.find('?');
        if (qpos != string::npos) {
            parseQuery(fullPath.substr(qpos+1), req.params);
            req.path = fullPath.substr(0, qpos);
        } else {
            req.path = fullPath;
        }

        unordered_map<string,string> hdrs;
        string line;
        while (getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) break;
            auto col = line.find(':');
            if (col != string::npos) {
                string k = line.substr(0, col);
                string v = line.substr(col+1);
                size_t s = v.find_first_not_of(" \t");
                if (s != string::npos) v = v.substr(s);
                transform(k.begin(), k.end(), k.begin(), ::tolower);
                hdrs[k] = v;
            }
        }

        if (req.method == "POST") {
            int len = 0;
            auto it = hdrs.find("content-length");
            if (it != hdrs.end()) { try { len = stoi(it->second); } catch(...){} }
            if (len > 0 && len < 1048576) {
                req.body.resize(len);
                int total = 0;
                while (total < len) {
                    int r = recv(fd, &req.body[total], len - total, 0);
                    if (r <= 0) break;
                    total += r;
                }
            }
        }

        Response res;
        bool matched = false;
        for (auto& route : routes_) {
            bool mMatch = (route.method == req.method);
            bool pMatch = (route.path == ".*") || (route.path == req.path);
            if (mMatch && pMatch) {
                route.handler(req, res);
                matched = true;
                break;
            }
        }

        if (!matched) {
            res.status = 404;
            res.body   = "{\"ok\":false,\"error\":\"Not found\"}";
            res.headers["Content-Type"] = "application/json";
            res.headers["Access-Control-Allow-Origin"] = "*";
        }

        ostringstream resp;
        resp << "HTTP/1.1 " << res.status << " OK\r\n";
        resp << "Content-Length: " << res.body.size() << "\r\n";
        for (auto& kv : res.headers)
            resp << kv.first << ": " << kv.second << "\r\n";
        resp << "Connection: close\r\n\r\n" << res.body;

        string out = resp.str();
        size_t sent = 0;
        while (sent < out.size()) {
            int n = send(fd, out.c_str() + (int)sent, (int)(out.size() - sent), 0);
            if (n <= 0) break;
            sent += n;
        }
        sock_close(fd);
    }
};

} // namespace httplib