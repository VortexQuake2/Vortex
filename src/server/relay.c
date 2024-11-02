#include "g_local.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#ifndef SOCKET
#define SOCKET int
#endif

#else
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <msgpack.h>

// relay server constants
const size_t HEADER_SIZE = sizeof(uint32_t) * 2;
const uint32_t MAGIC = 0xC0A7BEEF;
const char* COMMAND_RELAY = "Relay";


// global variables
SOCKET vrx_relay_socket = -1;
uint8_t pending_buf[16 * 1024] = {0};
size_t pending_buf_size = 0;

qboolean vrx_relay_is_connected() {
    return vrx_relay_socket != -1;
}

void vrx_relay_connect() {
    if (vrx_relay_socket != -1) {
        gi.dprintf("RS: Already connected to relay server.\n");
        return;
    }

    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    gi.dprintf("RS: Connecting to relay server...\n");
    cvar_t* server = gi.cvar("vrx_relay_server", "localhost", 0);
    cvar_t* port = gi.cvar("vrx_relay_port", "9999", 0);

    int rv = getaddrinfo(server->string, port->string, &hints, &servinfo);
    if (rv != 0) {
        gi.dprintf("RS: Failed to resolve relay server address. Errno: %s.\n", gai_strerror(rv));
        return;
    }

    qboolean connected = false;
    for (struct addrinfo* p = servinfo; p != NULL; p = p->ai_next) {
        vrx_relay_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (vrx_relay_socket == -1) {
            continue;
        }

        char ip[INET6_ADDRSTRLEN];
        if (p->ai_family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
            inet_ntop(p->ai_family, &addr->sin_addr, ip, sizeof ip);
            ip[INET_ADDRSTRLEN] = '\0';
        } else {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)p->ai_addr;
            inet_ntop(p->ai_family, &addr->sin6_addr, ip, sizeof ip);
        }

        if (connect(vrx_relay_socket, p->ai_addr, p->ai_addrlen) == -1) {
#ifdef WIN32
            int err = WSAGetLastError();
            gi.dprintf("RS: Error connecting to Relay server (%s): (%d)\n", ip, err);
            closesocket(vrx_relay_socket);
#else
            close(vrx_relay_socket);
#endif
            continue;
        }

        gi.dprintf("RS: Connected to relay server.\n");
        connected = true;
        break;
    }

    if (!connected) {
        gi.dprintf("RS: Failed to connect to relay server.\n");
        vrx_relay_socket = -1;
    } else {
#ifndef WIN32
        fcntl(vrx_relay_socket, F_SETFL, O_NONBLOCK);  // set to non-blocking
#else
        u_long iMode = 1;
        ioctlsocket(vrx_relay_socket, FIONBIO, &iMode);
#endif
    }

    freeaddrinfo(servinfo);
}

void vrx_relay_disconnect() {
    if (vrx_relay_socket != -1) {
#ifdef WIN32
        closesocket(vrx_relay_socket);
#else
        close(vrx_relay_socket);
#endif
        vrx_relay_socket = -1;
    }
}

void send_sbuffer(SOCKET s, msgpack_sbuffer* sbuf) {
    int sent = 0;
    size_t size = sizeof (uint32_t) * 2 + sbuf->size;

    if (sbuf->size > UINT_MAX) {
        gi.dprintf("RS: Message size is too large.\n");
        return;
    }

    // az: allocate memory for the size and the data
    char* data = calloc(size, 1);
    if (data == NULL) {
        gi.dprintf("RS: Failed to allocate memory for message.\n");
        return;
    }

    memcpy(data, &MAGIC, sizeof (uint32_t));
    memcpy(data + sizeof(MAGIC), &sbuf->size, sizeof (uint32_t));
    memcpy(data + HEADER_SIZE, sbuf->data, sbuf->size);

    while (sent < sbuf->size) {
        int n = send(s, data + sent, size - sent, 0);
        if (n == -1) {
#ifdef WIN32
            int err = WSAGetLastError();
            if (err == EWOULDBLOCK) {
                continue;
            }
#else
            int err = errno;
            if (errno == EWOULDBLOCK) {
                continue;
            }
#endif
            gi.dprintf("RS: Failed to send message to relay server. (%d)\n", err);
            break;
        }

        sent += n;
    }

    free(data);
}

void vrx_relay_message(const char* message) {
    msgpack_sbuffer sbuf;
    msgpack_packer pk;

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);
    msgpack_pack_str(&pk, 5);
    msgpack_pack_str_body(&pk, COMMAND_RELAY, 5);
    msgpack_pack_str(&pk, strlen(message));
    msgpack_pack_str_body(&pk, message, strlen(message));

    send_sbuffer(vrx_relay_socket, &sbuf);

    msgpack_sbuffer_destroy(&sbuf);
}

msgpack_object_str* msgpack_get_map_string(msgpack_object_map* map, const char* key) {
    for (uint32_t i = 0; i < map->size; i++) {
        msgpack_object_kv* kv = map->ptr + i;

        if (kv->key.type != MSGPACK_OBJECT_STR) {
            continue;
        }

        if (kv->key.via.str.size == strlen(key) && memcmp(kv->key.via.str.ptr, key, strlen(key)) == 0) {
            if (kv->val.type != MSGPACK_OBJECT_STR) {
                continue;
            }

            return &kv->val.via.str;
        }
    }

    return NULL;
}

int msgpack_streq(msgpack_object_str* str, const char* cmp) {
    return str->size == strlen(cmp) && memcmp(str->ptr, cmp, str->size) == 0;
}

qboolean vrx_relay_try_message_relay(msgpack_object_str *type, msgpack_object_array *arr) {
    if (!msgpack_streq(type, COMMAND_RELAY))
        return true;

    msgpack_object_str* message = arr->ptr[1].type == MSGPACK_OBJECT_STR ? &arr->ptr[1].via.str : NULL;
    if (message == NULL) {
        gi.dprintf("RS: Received relay message does not have a valid message field.\n");
        return false;
    }

    // az: null-terminate the string
    char* msg = calloc(message->size + 1, 1);
    if (msg == NULL) {
        gi.dprintf("RS: Failed to allocate memory for relay message.\n");
        return false;
    }

    memcpy(msg, message->ptr, message->size);

    for (int j = 1; j <= game.maxclients; j++) {
        edict_t *other = &g_edicts[j];
        if (!other->inuse)
            continue;
        if (!other->client)
            continue;
        if (other->svflags & SVF_MONSTER)
            continue;

        gi.cprintf(other, PRINT_CHAT, "[relay] %s\n", msg);
    }

    // az: print to the server console
    gi.cprintf(NULL, PRINT_CHAT, "[relay] %s\n", msg);
    free(msg);

    return true;
}

typedef enum {
    RESULT_INVALID,
    RESULT_NEED_MORE_DATA,
    RESULT_CONTINUE,
    RESULT_SUCCESS
} relay_parse_result_t;

relay_parse_result_t vrx_relay_parse_message(size_t* start) {

    // az: we need at least 8 bytes to read the magic and size
    if (*start + HEADER_SIZE >= pending_buf_size) {
        return RESULT_NEED_MORE_DATA;
    }

    uint8_t* curbuf = (uint8_t*)&pending_buf[*start];
    // az: read the magic and size, little-endian
#ifdef LITTLE_ENDIAN
    uint32_t magic = curbuf[0] | curbuf[1] << 8 | curbuf[2] << 16 | curbuf[3] << 24;
    uint32_t size = curbuf[4] | curbuf[5] << 8 | curbuf[6] << 16 | curbuf[7] << 24;
#elif BIG_ENDIAN
    uint32_t magic = curbuf[3] | curbuf[2] << 8 | curbuf[1] << 16 | curbuf[0] << 24;
    uint32_t size = curbuf[7] | curbuf[6] << 8 | curbuf[5] << 16 | curbuf[4] << 24;
#endif
    size_t off = 0;

    // az: check if we have a valid message
    if (magic != MAGIC) {
        gi.dprintf("RS: Received message from relay server is not a valid message.\n");
        return RESULT_INVALID;
    }

    // az: we need at least size bytes to read the message
    if (size > pending_buf_size - *start - HEADER_SIZE) {
        return RESULT_NEED_MORE_DATA;
    }

    msgpack_unpacked result;
    msgpack_unpacked_init(&result);

    msgpack_unpack_return ret = msgpack_unpack_next(&result, curbuf + HEADER_SIZE, size, &off);
    if (ret == MSGPACK_UNPACK_CONTINUE) {
        return RESULT_NEED_MORE_DATA;
    }

    if (ret != MSGPACK_UNPACK_SUCCESS) {
        gi.dprintf("RS: Failed to unpack message from relay server.\n");
        return RESULT_INVALID;
    }

    msgpack_object obj = result.data;

    if (obj.type != MSGPACK_OBJECT_ARRAY) {
        gi.dprintf("RS: Received message from relay server is not an array.\n");
        return RESULT_INVALID;
    }

    if (obj.via.array.size <= 1) {
        gi.dprintf("RS: Received message from relay server is not a valid message.\n");
        return RESULT_INVALID;
    }

    msgpack_object_str* type = NULL;
    msgpack_object_array* arr = &obj.via.array;
    if (arr->ptr[0].type == MSGPACK_OBJECT_STR && arr->ptr[0].via.str.size > 0) {
        type = &arr->ptr[0].via.str;
    } else {
        gi.dprintf("RS: Received message from relay server does not have a valid type.\n");
        return RESULT_INVALID;
    }

    qboolean success = false;
    success |= vrx_relay_try_message_relay(type, arr);

    msgpack_unpacked_destroy(&result);
    if (success) {
        // advance the start to the next message
        *start += HEADER_SIZE + size;

        // check if we have more messages
        if (*start + HEADER_SIZE < pending_buf_size) {
            return RESULT_CONTINUE;
        }

        // we have processed all messages
        return RESULT_SUCCESS;
    } else {
        return RESULT_INVALID;
    }
}

void vrx_relay_recv() {
    if (vrx_relay_socket == -1) {
        return;
    }

    char recbuf[16 * 1024];
    int n = recv(vrx_relay_socket, recbuf, sizeof(recbuf), 0);
    if (n == -1) {
#ifndef WIN32
        if (errno == EWOULDBLOCK) {
            return;
        }

        gi.dprintf("RS: Failed to receive message from relay server: %s\n", strerror(errno));
#else
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return;
        }

        vrx_relay_disconnect();
        gi.dprintf("RS: Failed to receive message from relay server: %d\n", err);
#endif
        return;
    }

    if (n == 0) {
        gi.dprintf("RS: Connection to relay server closed.\n");
        vrx_relay_disconnect();
        return;
    }

    // copy into pending buffer
    if (pending_buf_size + n > sizeof(pending_buf)) {
        gi.dprintf("RS: Received message from relay server is too large.\n");
        vrx_relay_disconnect();
        return;
    }

    memcpy(pending_buf + pending_buf_size, recbuf, n);
    pending_buf_size += n;

    size_t start = 0;
    qboolean continue_parsing = true;
    while (continue_parsing) {
        relay_parse_result_t res = vrx_relay_parse_message(&start);

        switch (res) {
            case RESULT_INVALID:
                // az: we need to clear the buffer and disconnect
                memset(pending_buf, 0, sizeof(pending_buf));
                pending_buf_size = 0;
                continue_parsing = false;

                vrx_relay_disconnect();
            break;
            case RESULT_NEED_MORE_DATA:
            case RESULT_SUCCESS:
                continue_parsing = false;
                // az: we need to wait for more data, memmove the reminder to the beginning
                memmove(pending_buf, pending_buf + start, pending_buf_size - start);
                pending_buf_size -= start;
                break;
            case RESULT_CONTINUE:
                break;
        }
    }
}
