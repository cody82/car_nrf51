#ifndef __SWITCH_H
#define __SWITCH_H

typedef enum
{
    SwitchESB = 0,
    SwitchBLE
} SwitchPosition;

SwitchPosition Switch();

#endif
