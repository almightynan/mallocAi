/***************************************************************************
 * 
 *  mallocAi.h - Header for mallocAi library
 *  author: almightynan [https://almightynan.cc]
 *
 * AI-guided memory allocator that communicates with a local AI server.
 * Sends prompts to localhost:3000/gemini over HTTP, expects JSON reply
 * of the form: {"text": "<number>"}. Resulting number is malloc'd.
 *
 * Header defines two interfaces:
 *   - mallocAi(prompt): calls server and allocates, silently.
 *   - mallocAi_verbose(prompt, verbose): same but logs size if verbose=1.
 *
 * WARNING: If the server replies with "undefined", "infinite", or invalid
 * values, the allocator will intentionally crash with a segmentation fault.
 *
 * Intended for creative misuse, testing absurd ideas, or deliberate failure.
 * Not designed for safety, performance, or serious use cases.
 *
 * No guarantees. No warranty. No reason. Use at your own existential risk.
 ***************************************************************************/

#ifndef MALLOCAI_H
#define MALLOCAI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <webserver.h>
#endif

/*
 * NAME
 *   mallocAi_verbose()
 *
 * DESCRIPTION
 *   Connects to a local AI server, sends a prompt, and receives a JSON response
 *   containing the number of bytes to allocate. If verbose is nonzero, prints
 *   information about the allocation.
 *
 * PARAMETERS
 *   prompt  - The input string describing what to allocate.
 *   verbose - If nonzero, prints allocation details to stdout.
 *
 * RETURNS
 *   Pointer to allocated memory of the size determined by the AI.
 *   Crashes intentionally on invalid or dangerous responses.
 */

void *mallocAi_verbose(const char *prompt, int verbose) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#else
    ws_starter();
#endif

    int sock;
    struct sockaddr_in server;
#ifdef _WIN32
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
#else
    sock = socket(AF_INET, SOCK_STREAM, 0);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
#endif

    server.sin_family = AF_INET;
    server.sin_port = htons(3000);
    connect(sock, (struct sockaddr*)&server, sizeof(server));

    char json_body[1024];
#ifdef _WIN32
    _snprintf(json_body, sizeof(json_body), "{ \"prompt\": \"%s\" }", prompt);
#else
    snprintf(json_body, sizeof(json_body), "{ \"prompt\": \"%s\" }", prompt);
#endif

    char len_str[16];
#ifdef _WIN32
    _snprintf(len_str, sizeof(len_str), "%d", (int)lstrlenA(json_body));
#else
    snprintf(len_str, sizeof(len_str), "%d", (int)strlen(json_body));
#endif

    char req[2048];
#ifdef _WIN32
    int req_len = _snprintf(req, sizeof(req),
#else
    int req_len = snprintf(req, sizeof(req),
#endif
        "POST /gemini HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %s\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        len_str, json_body
    );

#ifdef _WIN32
    send(sock, req, req_len, 0);
#else
    write(sock, req, req_len);
#endif

    char buffer[4096];
    int total = 0, bytes;
    for (;;) {
#ifdef _WIN32
        bytes = recv(sock, buffer + total, sizeof(buffer) - total - 1, 0);
#else
        bytes = read(sock, buffer + total, sizeof(buffer) - total - 1);
#endif
        if (bytes <= 0) break;
        total += bytes;
    }
    buffer[total] = 0;

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
    ws_stopper();
#endif

    char *start = strstr(buffer, "{\"text\":\"");
    if (!start) return NULL;
    start += 9;

    char *end = strchr(start, '"');
    if (!end) return NULL;
    *end = 0;

    for (char *p = start; *p; p++) {
        if (*p == '\n' || *p == '\r') {
            *p = 0;
            break;
        }
    }

    if (strstr(start, "undefined") || strstr(start, "infinite") || strstr(start, "NaN")) {
        fprintf(stderr, "mallocAi_verbose(): fatal: invalid allocation size\n");
        fflush(stderr);
        *(volatile int *)0 = 0;
    }

    int size = atoi(start);
    if (size <= 0) {
        fprintf(stderr, "mallocAi_verbose(): fatal: invalid size \"%s\"\n", start);
        fflush(stderr);
        *(volatile int *)0 = 0;
    }

    if (verbose) {
        printf("mallocAi_verbose(): allocating %d bytes for prompt \"%s\"\n", size, prompt);
    }

    return malloc(size);
}

/**
 * ## mallocAi() - Allocate memory based on AI response.
 * @param prompt prompt sent to the AI server.
 *
 * @note By default, verbose output is disabled.
 * @note To enable verbose output showing allocation size, use mallocAi_verbose().
 *
 * @returns pointer to allocated memory, or NULL on failure.
 */

void *mallocAi(const char *prompt) {
    return mallocAi_verbose(prompt, 0);
}

#ifdef __cplusplus
}
#endif

#endif
