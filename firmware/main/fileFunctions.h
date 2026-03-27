#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H

#include "init.h"
#include "cJSON.h"
#include <sys/stat.h>
#include <dirent.h>
#include "esp_littlefs.h"

// Embedded file declarations
extern const char _binary_menu_json_start[];
extern const char _binary_menu_json_end[];
extern const char _binary_buttons_json_start[];
extern const char _binary_buttons_json_end[];

// Function declarations
void copy_embedded_json_to_littlefs();
esp_err_t setup_littlefs(void);
void flush_littlefs();
void list_files();
char *read_file_from_littlefs(const char *path);
void write_file_to_littlefs(const char* path, const char* content);
esp_err_t write_json_to_littlefs(const char *path, cJSON *json);
cJSON* parse_json_from_file(const char* path);
cJSON *parse_json_menu(const char *path);
extern ButtonSet* buttonSets;
extern int buttonSetsCount;
extern int activeButtonSetIndex;

void loadButtonMappings();
void selectButtonSet(const char *set_name);
void nextButtonSet();
void previousButtonSet();
ButtonMapping* getActiveButtonMappings();
const char* getActiveButtonSetName();
int getButtonSetsCount();
void freeButtonMappings();

#endif // FILEFUNCTIONS_H