#include "esp_http_server.h"
#include "esp_littlefs.h"

// Function to handle file download
esp_err_t download_handler(httpd_req_t *req) {
    const char *filepath = (strcmp(req->uri, "/download/menu.json") == 0) ? "/storage/menu.json" : "/storage/buttons.json";
    FILE *file = fopen(filepath, "r");
    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    char buffer[1024];
    size_t chunksize;
    while ((chunksize = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, chunksize) != ESP_OK) {
            fclose(file);
            httpd_resp_sendstr_chunk(req, NULL);
            return ESP_FAIL;
        }
    }
    fclose(file);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

// Function to handle file upload
esp_err_t upload_handler(httpd_req_t *req) {
    char filepath[32];
    snprintf(filepath, sizeof(filepath), "/storage/%s", req->uri + 8); // Skip "/upload/"

    FILE *file = fopen(filepath, "w");
    if (!file) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char buffer[1024];
    int received;
    while ((received = httpd_req_recv(req, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, received, file);
    }
    fclose(file);
    httpd_resp_sendstr(req, "File uploaded successfully");
    return ESP_OK;
}

// Function to serve the main webpage
esp_err_t index_handler(httpd_req_t *req) {
    extern const unsigned char index_html_start[] asm("_binary_index_html_start");
    extern const unsigned char index_html_end[] asm("_binary_index_html_end");
    const size_t index_html_size = (index_html_end - index_html_start);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_size);
    return ESP_OK;
}

void setupWebServer() {
    // Initialize LittleFS
    //if (!LittleFS.begin(true)) {
    //    Serial.println("An error has occurred while mounting LittleFS");
    //    return;
    //}

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &index_uri);

        httpd_uri_t download_menu_uri = {
            .uri = "/download/menu.json",
            .method = HTTP_GET,
            .handler = download_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &download_menu_uri);

        httpd_uri_t download_buttons_uri = {
            .uri = "/download/buttons.json",
            .method = HTTP_GET,
            .handler = download_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &download_buttons_uri);

        httpd_uri_t upload_uri = {
            .uri = "/upload/*",
            .method = HTTP_POST,
            .handler = upload_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &upload_uri);
    }
}
