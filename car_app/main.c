#include "switch.h"
#include "main_esb.h"
#include "main_ble.h"

int main()
{
    if(Switch() == SwitchESB)
    {
        main_esb();
    }
	else
	{
        main_ble();
	}
    return 0;
}
