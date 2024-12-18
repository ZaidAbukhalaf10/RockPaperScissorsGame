#include <libwebsockets.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 256
#define STATIC_FILE_DIR "./static"

// Global variables
static int connected_clients = 0;  // Number of connected clients
static struct lws** client_wsi_list = NULL;  // Dynamic array of client WebSocket handles
static int max_clients = 10;  // Initial size of the client list

void add_client_wsi(struct lws* wsi) {
    if (connected_clients >= max_clients) {
        max_clients *= 2;
        client_wsi_list = realloc(client_wsi_list, max_clients * sizeof(struct lws*));
    }
    client_wsi_list[connected_clients++] = wsi;
}

void remove_client_wsi(struct lws* wsi) {
    for (int i = 0; i < connected_clients; i++) {
        if (client_wsi_list[i] == wsi) {
            client_wsi_list[i] = client_wsi_list[connected_clients - 1];
            connected_clients--;
            break;
        }
    }
}

void broadcast_message(const char* message) {
    for (int i = 0; i < connected_clients; i++) {
        unsigned char buffer[LWS_PRE + BUFFER_SIZE];
        unsigned char* p = &buffer[LWS_PRE];
        memcpy(p, message, strlen(message));
        lws_write(client_wsi_list[i], p, strlen(message), LWS_WRITE_TEXT);
    }
}

// Callback for WebSocket communication
static int callback_game(struct lws* wsi, enum lws_callback_reasons reason,
    void* user, void* in, size_t len) {
    static char message[BUFFER_SIZE];
    unsigned char buffer[LWS_PRE + BUFFER_SIZE];
    unsigned char* p = &buffer[LWS_PRE];

    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        add_client_wsi(wsi);
        printf("Client connected. Total clients: %d\n", connected_clients);

        snprintf(message, BUFFER_SIZE, "Connected clients: %d", connected_clients);
        broadcast_message(message);
        break;

    case LWS_CALLBACK_RECEIVE: {
        int client_choice = atoi((const char*)in);
        int server_choice = rand() % 3;

        const char* result = (client_choice == server_choice) ? "It's a tie!" :
            (client_choice == 0 && server_choice == 2) ||
            (client_choice == 1 && server_choice == 0) ||
            (client_choice == 2 && server_choice == 1) ? "You win!" :
            "You lose!";

        snprintf(message, BUFFER_SIZE, "Result: %s (You: %d, Server: %d)", result, client_choice, server_choice);
        size_t msg_len = strlen(message);
        memcpy(p, message, msg_len);
        lws_write(wsi, p, msg_len, LWS_WRITE_TEXT);
        break;
    }

    case LWS_CALLBACK_CLOSED:
        remove_client_wsi(wsi);
        printf("Client disconnected. Total clients: %d\n", connected_clients);

        snprintf(message, BUFFER_SIZE, "Connected clients: %d", connected_clients);
        broadcast_message(message);
        break;

    default:
        break;
    }

    return 0;
}

// Callback for HTTP requests
static int callback_http(struct lws* wsi, enum lws_callback_reasons reason,
    void* user, void* in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_HTTP: {
        const char* requested_file = (const char*)in;

        if (strlen(requested_file) == 1 && requested_file[0] == '/') {
            requested_file = "/index.html";
        }

        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s%s", STATIC_FILE_DIR, requested_file);

        const char* mime_type = "text/html";

        if (strstr(requested_file, ".css")) {
            mime_type = "text/css";
        }
        else if (strstr(requested_file, ".js")) {
            mime_type = "application/javascript";
        }

        if (lws_serve_http_file(wsi, file_path, mime_type, NULL, 0)) {
            printf("Failed to serve file: %s\n", file_path);
            return -1;
        }
        break;
    }

    default:
        break;
    }

    return 0;
}

// Protocols
static struct lws_protocols protocols[] = {
    { "http-only", callback_http, 0, 0 },
    { "game-protocol", callback_game, 0, 0 },
    { NULL, NULL, 0, 0 }
};

int main() {
    srand((unsigned int)time(NULL));

    client_wsi_list = malloc(max_clients * sizeof(struct lws*));

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = 8080;
    info.protocols = protocols;

    struct lws_context* context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Failed to create WebSocket context\n");
        return -1;
    }

    printf("Server running at http://localhost:8080\n");

    while (1) {
        lws_service(context, 100);
    }

    lws_context_destroy(context);
    free(client_wsi_list);
    return 0;
}
