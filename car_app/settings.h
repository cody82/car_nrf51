#ifndef __SETTINGS_H
#define __SETTINGS_H


#include <stdint.h>
#include "fds.h"

typedef struct
{
    uint8_t MaxForwardSpeed;
    uint8_t MaxBackwardSpeed;
    uint8_t MaxLeft;
    uint8_t MaxRight;
} Settings;

extern Settings SettingsData;

ret_code_t SettingsInit(void);
ret_code_t SettingsSave(void);
ret_code_t SettingsLoad(void);
ret_code_t SettingsDefault(void);

ret_code_t fds_read(void);
ret_code_t fds_test_write(void);

extern volatile bool init_ok;
extern volatile bool write_ok;

#endif
