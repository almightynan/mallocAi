/***************************************************************************
 *  mallocAi - AI-guided memory allocator
 *  author: almightynan [https://almightynan.cc]                   
 *
 * AI-guided memory allocator that communicates with a local AI server.
 * Sends prompts to localhost:3000/gemini over HTTP, receives byte count
 * in JSON {"text": "<number>"}, and mallocs the resulting size.
 *
 * Written for experimentation, satire, or extreme overengineering.
 * Handles invalid input ("undefined", "infinite", "NaN") by printing
 * error and causing intentional segmentation fault.
 *
 * Interfaces:
 *   void *mallocAi(const char *prompt);
 *   void *mallocAi_verbose(const char *prompt, int verbose);
 *
 * Requirements:
 *   - Local server responding to POST /gemini
 *   - Response format: JSON with "text" field as byte count
 *
 * Not production safe. Not deterministic. Not rational. Use accordingly.
 *
 * No warranty implied. If this code allocates 4GB for "a joke", it’s your fault.
 ***************************************************************************/

#include "mallocai.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

/*
 * NAME
 *   mallocAi()
 *
 * DESCRIPTION
 *   Allocates memory of a size determined by an AI model, based on the given prompt.
 *   This is a convenience wrapper for mallocAi_verbose() with verbose output disabled.
 */

void *mallocAi(const char *prompt) {
    return mallocAi_verbose(prompt, 0);
}

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
    WSAStartup(MAKEWORD(2,2), &wsa); /* Initialize Windows Sockets */
#endif

    int sock;
    struct sockaddr_in server;
#ifdef _WIN32
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
#else
    sock = socket(AF_INET, SOCK_STREAM, 0);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr); /* Convert IP string to binary */
#endif

    server.sin_family = AF_INET;
    server.sin_port = htons(3000); /* Connect to port 3000 */
    connect(sock, (struct sockaddr*)&server, sizeof(server));

    /* Build JSON request body */
    char json_body[1024];
#ifdef _WIN32
    _snprintf(json_body, sizeof(json_body), "{ \"prompt\": \"%s\" }", prompt);
#else
    snprintf(json_body, sizeof(json_body), "{ \"prompt\": \"%s\" }", prompt);
#endif

    /* Calculate Content-Length */
    char len_str[16];
#ifdef _WIN32
    _snprintf(len_str, sizeof(len_str), "%d", (int)lstrlenA(json_body));
#else
    snprintf(len_str, sizeof(len_str), "%d", (int)strlen(json_body));
#endif

    /* Build HTTP POST request */
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
    send(sock, req, req_len, 0); /* Send request on Windows */
#else
    write(sock, req, req_len);   /* Send request on Unix */
#endif

    /* Read response into buffer */
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
    buffer[total] = 0; /* Null-terminate */

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

    /* Parse JSON response: look for "text" field */
    char *start = strstr(buffer, "{\"text\":\"");
    if (!start) return 0;
    start += 9;

    /* Find end quote and terminate string */
    char *end = strchr(start, '"');
    if (!end) return 0;
    *end = 0;

    /* Remove newline and carriage return characters */
    char *p = start;
    while (*p) {
        if (*p == '\n' || *p == '\r') {
            *p = 0;
            break;
        }
        p++;
    }

    /* Parse the number from the response */
    int size = atoi(start);

    /* Check for invalid or dangerous responses */
    if (strstr(start, "undefined") || strstr(start, "infinite") || strstr(start, "NaN")) {
        fprintf(stderr, "mallocAi_verbose(): fatal: cannot allocate memory for that (you think this is a data center?)\n");
        fflush(stderr);
        *(volatile int *)0 = 0; /* Intentional crash */
    }

    if (size <= 0) {
        fprintf(stderr, "mallocAi_verbose(): fatal: invalid size \"%s\"\n", start);
        fflush(stderr);
        *(volatile int *)0 = 0; /* Crash on purpose */
    }

    /* Print verbose info if requested */
    if (verbose) {
        printf("mallocAi_verbose(): info: chose %d bytes for \"%s\" [run mallocAi() to disable this]\n", size, prompt);
    }

    /* Allocate memory of requested size */
    return malloc(size);
}
