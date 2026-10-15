// Copyright (2025) Beijing Volcano Engine Technology Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "volc_http.h"

#ifdef ESP_PLATFORM

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <esp_http_client.h>

#include "tls_certificate.h"
#include "util/volc_log.h"
#include "volc_osal.h"

typedef struct {
    const char* uri;
    const char* post_data;
    int data_len;
    char* response;
    SemaphoreHandle_t completed;
} esp_http_context_t;

char* esp_http_post_perform(const char* uri, const char* post_data, int data_len);

static void esp_http_post_task(void* arg) {
    esp_http_context_t* context = (esp_http_context_t*)arg;
    context->response = esp_http_post_perform(context->uri, context->post_data, context->data_len);
    xSemaphoreGive(context->completed);
    vTaskDelete(NULL);
}

char* volc_http_post(const char* uri, const char* post_data, int data_len) {
    if (uri == NULL || post_data == NULL || data_len < 0) {
        return NULL;
    }

    SemaphoreHandle_t completed = xSemaphoreCreateBinary();
    if (completed == NULL) {
        return NULL;
    }
    esp_http_context_t context = {
        .uri = uri,
        .post_data = post_data,
        .data_len = data_len,
        .response = NULL,
        .completed = completed,
    };
    if (xTaskCreate(esp_http_post_task, "volc_https", 12288, &context, 4, NULL) != pdPASS) {
        vSemaphoreDelete(completed);
        return NULL;
    }
    xSemaphoreTake(completed, portMAX_DELAY);
    vSemaphoreDelete(completed);
    return context.response;
}

char* esp_http_post_perform(const char* uri, const char* post_data, int data_len) {
    if (uri == NULL || post_data == NULL || data_len < 0) {
        return NULL;
    }

    esp_http_client_config_t config = {};
    config.url = uri;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 10000;
    config.cert_pem = GLOBAL_ROOT_CERT;
    config.cert_len = GLOBAL_ROOT_CERT_LEN - 1;
    config.keep_alive_enable = true;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        LOGE("Failed to create the ESP HTTP client");
        return NULL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, data_len);
    if (esp_http_client_perform(client) != ESP_OK) {
        LOGE("Failed to perform the ESP HTTP request");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return NULL;
    }

    const int content_length = esp_http_client_fetch_headers(client);
    size_t capacity = content_length > 0 ? (size_t)content_length + 1 : 2048;
    char* buffer = (char*)volc_osal_calloc(capacity, sizeof(char));
    size_t used = 0;
    while (buffer != NULL && used + 1 < capacity) {
        const int read_len = esp_http_client_read(client, buffer + used, capacity - used - 1);
        if (read_len <= 0) {
            break;
        }
        used += (size_t)read_len;
        if (used + 1 == capacity) {
            const size_t new_capacity = capacity * 2;
            char* new_buffer = (char*)volc_osal_realloc(buffer, new_capacity);
            if (new_buffer == NULL) {
                break;
            }
            buffer = new_buffer;
            memset(buffer + capacity, 0, new_capacity - capacity);
            capacity = new_capacity;
        }
    }

    const int status = esp_http_client_get_status_code(client);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (buffer == NULL) {
        LOGE("Failed to allocate the ESP HTTP response");
        return NULL;
    }
    if (status < 200 || status >= 300) {
        LOGE("ESP HTTP POST failed with status %d", status);
        volc_osal_free(buffer);
        return NULL;
    }
    buffer[used] = '\0';
    return buffer;
}

#else

#include "tls_certificate.h"
#include "util/volc_list.h"
#include "util/volc_log.h"
#include "webclient.h"

char* volc_http_post(const char* uri, const char* post_data, int data_len) {
    struct webclient_session* session = NULL;
    char* buffer = NULL;
    int resp_status;
    size_t res_len = 0;

    /* create webclient session and set header response size */
    session = webclient_session_create(2048, GLOBAL_ROOT_CERT, GLOBAL_ROOT_CERT_LEN);
    if (session == NULL) {
        goto err_out_label;
    }

    webclient_header_fields_add(session, "Content-Type: application/json\r\n");
    webclient_header_fields_add(session, "Content-Length: %d\r\n", strlen(post_data));

    /* send POST request by default header */
    if ((resp_status = webclient_post(session, uri, post_data, data_len)) != 200) {
        LOGE("webclient POST request failed, response(%d) error.\n", resp_status);
    }

    webclient_response(session, (void**)&buffer, &res_len);
    LOGD("url: %s, request: %s, response: %s", uri, post_data, buffer);
err_out_label:
    if (session) {
        webclient_close(session);
    }

    return buffer;
}

#endif
