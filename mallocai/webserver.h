#ifndef SIMPLE_AI_SERVER_H
#define SIMPLE_AI_SERVER_H

/*
 * Minimal C HTTP server for POST /ai.
 * Receives JSON {"prompt":"..."} and returns {"text":"..."} with byte count.
 * Gemini API call simulated with dummy response.
 * No SSL or real HTTP client implemented.
 * Use Linux or Windows sockets.
 */

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

#define PORT 3000
#define MAX_BUF 4096

#ifdef _WIN32
#define CLOSESOCK closesocket
#else
#define CLOSESOCK close
#endif

static int recv_all(int sock, char *buf, int max_len) {
    int total = 0;
    while (total < max_len - 1) {
#ifdef _WIN32
        int r = recv(sock, buf + total, max_len - 1 - total, 0);
#else
        int r = read(sock, buf + total, max_len - 1 - total);
#endif
        if (r <= 0) break;
        total += r;
        if (strstr(buf, "\r\n\r\n")) break; // crude end of headers
    }
    buf[total] = 0;
    return total;
}

static int extract_prompt(const char *body, char *out, int max_len) {
    const char *p = strstr(body, "\"prompt\"");
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != '"') return 0;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < max_len - 1) {
        if (*p == '\\' && *(p+1) == '"') {
            out[i++] = '"';
            p += 2;
        } else {
            out[i++] = *p++;
        }
    }
    out[i] = 0;
    return 1;
}

static void build_response(const char *text, char *out, int max_len) {
    int n = snprintf(out, max_len,
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n"
        "{\"text\":\"%s\"}",
        strlen(text) + 10, text);
    if (n < 0 || n >= max_len) out[0] = 0;
}

static int ws_starter(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;
#endif

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) return 1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) return 1;
    if (listen(listen_sock, 5) < 0) return 1;

    while (1) {
        struct sockaddr_in client_addr;
#ifdef _WIN32
        int len = sizeof(client_addr);
#else
        socklen_t len = sizeof(client_addr);
#endif
        int client = accept(listen_sock, (struct sockaddr*)&client_addr, &len);
        if (client < 0) continue;

        char buf[MAX_BUF];
        int r = recv_all(client, buf, MAX_BUF);

        if (r > 0 && strncmp(buf, "POST /ai", 8) == 0) {
            char *body = strstr(buf, "\r\n\r\n");
            if (body) body += 4;

            char prompt[1024];
            if (body && extract_prompt(body, prompt, sizeof(prompt))) {
                char resp[1024];
                build_response("1024", resp, sizeof(resp)); // fixed dummy response
                send(client, resp, (int)strlen(resp), 0);
            } else {
                const char *err = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n";
                send(client, err, (int)strlen(err), 0);
            }
        } else {
            const char *notfound = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
            send(client, notfound, (int)strlen(notfound), 0);
        }

        CLOSESOCK(client);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

#endif // SIMPLE_AI_SERVER_H