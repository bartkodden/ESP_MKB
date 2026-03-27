//#ifndef BUTTON_LABELS_H
//#define BUTTON_LABELS_H
//
//#ifdef __cplusplus
//extern "C" {
//#endif
//
//// Update button labels from current button set
//void update_button_labels(void);
//
//// Cleanup (optional)
//void cleanup_button_labels(void);
//
//#ifdef __cplusplus
//}
//#endif
//
//#endif // BUTTON_LABELS_H

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUTTON_ICON_MAX         64
#define BUTTON_NAME_MAX         48
#define BUTTON_SET_NAME_MAX     32
#define BUTTON_SET_ICON_MAX     64

typedef struct {
    char key_id;
    char icon[BUTTON_ICON_MAX];
    char name[BUTTON_NAME_MAX];
    int  cmd;
} ButtonMapping;

typedef struct {
    char set_name[BUTTON_SET_NAME_MAX];
    char set_icon[BUTTON_SET_ICON_MAX];
    ButtonMapping buttonMappings[8];
} ButtonSet;

extern ButtonSet *buttonSets;
extern int         buttonSetsCount;
extern int         activeButtonSetIndex;

void update_button_labels(void);
void cleanup_button_labels(void);

#ifdef __cplusplus
}
#endif