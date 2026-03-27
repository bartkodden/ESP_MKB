#include "filefunctions.h"
#include "esp_littlefs.h"
#include "ui/button_labels.h"


static const char *TAG = "FILE";

struct EmbeddedFile {
    const char *start;
    const char *end;
    const char *path;
};

void copy_embedded_json_to_littlefs() {
    EmbeddedFile files[] = {
        { _binary_menu_json_start, _binary_menu_json_end, "/storage/menu.json" },
        { _binary_buttons_json_start, _binary_buttons_json_end, "/storage/buttons.json" }
    };

    for (const auto &file : files) {
        size_t size = file.end - file.start;
        
        ESP_LOGI(TAG, "Writing %s (%d bytes)...", file.path, size);
        
        FILE *f = fopen(file.path, "w");
        if (f) {
            fwrite(file.start, 1, size, f);
            fclose(f);
            ESP_LOGI(TAG, "✅ %s written and closed", file.path);
        } else {
            ESP_LOGE(TAG, "❌ Failed to open %s", file.path);
        }
    }
}

esp_err_t setup_littlefs(void) {
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = "storage",
        .partition = NULL,
        .format_if_mount_failed = true,
        .read_only = false,
        .dont_mount = false,
        .grow_on_mount = false
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "LittleFS mounted successfully");
    return ESP_OK;
}

void flush_littlefs() {
    ESP_LOGI(TAG, "Remounting LittleFS to flush cache...");
    
    // Unmount
    esp_err_t ret = esp_vfs_littlefs_unregister("/storage");
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Unmount warning: %s", esp_err_to_name(ret));
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Remount
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false
    };

    ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Remount failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "✅ LittleFS remounted");
    }
}

void list_files() {
    DIR *dir = opendir("/storage");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            printf("File: %s\n", entry->d_name);
        }
        closedir(dir);
    } else {
        ESP_LOGE(TAG, "Failed to open directory");
    }
}

char *read_file_from_littlefs(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Memory allocation failed");
        fclose(f);
        return NULL;
    }

    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    return buffer;
}

void write_file_to_littlefs(const char* path, const char* content) {
    FILE* file = fopen(path, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        return;
    }

    fwrite(content, 1, strlen(content), file);
    fclose(file);

    ESP_LOGI(TAG, "Successfully wrote to file: %s", path);
}

esp_err_t write_json_to_littlefs(const char *path, cJSON *json) {
    char *json_str = cJSON_PrintUnformatted(json);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to convert JSON to string");
        return ESP_FAIL;
    }

    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        free(json_str);
        return ESP_FAIL;
    }

    fwrite(json_str, 1, strlen(json_str), f);
    fclose(f);
    free(json_str);

    ESP_LOGI(TAG, "JSON written to: %s", path);
    return ESP_OK;
}

cJSON* parse_json_from_file(const char* path) {
    char* file_content = read_file_from_littlefs(path);
    if (!file_content) {
        ESP_LOGE(TAG, "Failed to read file: %s", path);
        return NULL;
    }

    cJSON* json = cJSON_Parse(file_content);
    free(file_content);

    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON from file: %s", path);
        return NULL;
    }

    ESP_LOGI(TAG, "Successfully parsed JSON from: %s", path);
    return json;
}

cJSON *parse_json_menu(const char *path) {
    char *file_content = read_file_from_littlefs(path);
    if (!file_content) {
        return NULL;
    }
    
    cJSON *json = cJSON_Parse(file_content);
    free(file_content);
    if (!json) {
        return NULL;
    }
    return json;
}

void loadButtonMappings() {
    cJSON *json = parse_json_from_file("/storage/buttons.json");
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse buttons.json");
        return;
    }

    cJSON *sets = cJSON_GetObjectItemCaseSensitive(json, "buttons");
    if (!cJSON_IsArray(sets)) {
        ESP_LOGE(TAG, "'buttons' is not an array");
        cJSON_Delete(json);
        return;
    }

    const int setCount = cJSON_GetArraySize(sets);
    ESP_LOGI(TAG, "Found %d button sets", setCount);

    free(buttonSets);
    buttonSets = (ButtonSet *)calloc(setCount, sizeof(ButtonSet));
    if (!buttonSets) {
        ESP_LOGE(TAG, "Failed to allocate ButtonSet array");
        buttonSetsCount = 0;
        cJSON_Delete(json);
        return;
    }
    buttonSetsCount = setCount;

    int setIndex = 0;
    cJSON *setObj = NULL;
    cJSON_ArrayForEach(setObj, sets) {
        if (setIndex >= buttonSetsCount) break;

        ButtonSet &set = buttonSets[setIndex];

        cJSON *set_name = cJSON_GetObjectItemCaseSensitive(setObj, "set_name");
        cJSON *set_icon = cJSON_GetObjectItemCaseSensitive(setObj, "set_icon");
        cJSON *content  = cJSON_GetObjectItemCaseSensitive(setObj, "content");

        if (!cJSON_IsString(set_name) || !cJSON_IsArray(content)) {
            ESP_LOGW(TAG, "Skipping malformed set at index %d", setIndex);
            continue;
        }

        strlcpy(set.set_name, set_name->valuestring, sizeof(set.set_name));
        if (cJSON_IsString(set_icon)) {
            strlcpy(set.set_icon, set_icon->valuestring, sizeof(set.set_icon));
        } else {
            set.set_icon[0] = '\0';
        }

        int btnIndex = 0;
        cJSON *buttonObj = NULL;
        cJSON_ArrayForEach(buttonObj, content) {
            if (btnIndex >= 8) break;

            cJSON *key_id = cJSON_GetObjectItemCaseSensitive(buttonObj, "key_id");
            cJSON *icon   = cJSON_GetObjectItemCaseSensitive(buttonObj, "icon");
            cJSON *name   = cJSON_GetObjectItemCaseSensitive(buttonObj, "name");
            cJSON *cmd    = cJSON_GetObjectItemCaseSensitive(buttonObj, "cmd");

            if (!cJSON_IsString(key_id) || strlen(key_id->valuestring) == 0 ||
                !cJSON_IsString(icon)   ||
                !cJSON_IsString(name)   ||
                !cJSON_IsNumber(cmd)) {
                ESP_LOGW(TAG, "Skipping malformed button entry in set '%s'", set.set_name);
                continue;
            }

            ButtonMapping &entry = set.buttonMappings[btnIndex];
            entry.key_id = key_id->valuestring[0];
            strlcpy(entry.icon, icon->valuestring, sizeof(entry.icon));
            strlcpy(entry.name, name->valuestring, sizeof(entry.name));
            entry.cmd = cmd->valueint;

            btnIndex++;
        }

        for (int i = btnIndex; i < 8; ++i) {
            set.buttonMappings[i].key_id = '\0';
            set.buttonMappings[i].icon[0] = '\0';
            set.buttonMappings[i].name[0] = '\0';
            set.buttonMappings[i].cmd = 0;
        }

        ++setIndex;
    }

    cJSON_Delete(json);

    if (activeButtonSetIndex >= buttonSetsCount) {
        activeButtonSetIndex = 0;
    }

    ESP_LOGI(TAG, "Loaded %d button sets", buttonSetsCount);
}

void nextButtonSet() {
    if (buttonSets == NULL || buttonSetsCount == 0) {
        ESP_LOGE(TAG, "No button sets loaded");
        return;
    }

    activeButtonSetIndex = (activeButtonSetIndex + 1) % buttonSetsCount;
    ESP_LOGI(TAG, "Active button set: %s (index %d/%d)", buttonSets[activeButtonSetIndex].set_name, activeButtonSetIndex + 1, buttonSetsCount);

    update_button_labels();
}

void previousButtonSet() {
    if (buttonSets == NULL || buttonSetsCount == 0) {
        ESP_LOGE(TAG, "No button sets loaded");
        return;
    }

    activeButtonSetIndex = (activeButtonSetIndex - 1 + buttonSetsCount) % buttonSetsCount;
    ESP_LOGI(TAG, "Active button set: %s (index %d/%d)",  buttonSets[activeButtonSetIndex].set_name, activeButtonSetIndex + 1, buttonSetsCount);
    
    update_button_labels();
}

void selectButtonSet(const char *set_name) {
    if (buttonSets == NULL || buttonSetsCount == 0) {
        ESP_LOGE(TAG, "No button sets loaded");
        return;
    }

    for (int i = 0; i < buttonSetsCount; i++) {
        if (strcmp(buttonSets[i].set_name, set_name) == 0) {
            activeButtonSetIndex = i;
            ESP_LOGI(TAG, "Active button set: %s (index %d)", set_name, i);
            
            // Update button labels
            update_button_labels();
            return;
        }
    }
    ESP_LOGE(TAG, "Button set not found: %s", set_name);
}

ButtonMapping* getActiveButtonMappings() {
    if (buttonSets == NULL || buttonSetsCount == 0) {
        ESP_LOGE(TAG, "No button sets loaded");
        return NULL;
    }

    if (activeButtonSetIndex < 0 || activeButtonSetIndex >= buttonSetsCount) {
        ESP_LOGE(TAG, "Invalid active button set index: %d", activeButtonSetIndex);
        return NULL;
    }

    return buttonSets[activeButtonSetIndex].buttonMappings;
}

const char* getActiveButtonSetName() {
    if (buttonSets == NULL || buttonSetsCount == 0) {
        return "None";
    }
    
    if (activeButtonSetIndex < 0 || activeButtonSetIndex >= buttonSetsCount) {
        return "Invalid";
    }
    
    return buttonSets[activeButtonSetIndex].set_name;
}

int getButtonSetsCount() {
    return buttonSetsCount;
}
void freeButtonMappings() {
    if (buttonSets != NULL) {
        free(buttonSets);
        buttonSets = NULL;
    }
    buttonSetsCount = 0;
    activeButtonSetIndex = 0;
    ESP_LOGI(TAG, "Button mappings freed");
}