#include "settings.h"

#include <stdlib.h>
#include <string.h>

#define FILE_ID     0x1111
#define REC_KEY     0x2222

volatile bool init_ok = false;
volatile bool write_ok = false;

Settings SettingsData;

ret_code_t fds_read(void);
ret_code_t fds_test_write(void);

static void my_fds_evt_handler(fds_evt_t const * const p_fds_evt)
{
    ret_code_t ret;
    switch (p_fds_evt->id)
    {
        case FDS_EVT_INIT:
            if (p_fds_evt->result != FDS_SUCCESS)
            {
                // Initialization failed.
            }
            else
            {
				if(SettingsLoad() != FDS_SUCCESS)
				{
					SettingsDefault();
				}
                init_ok = true;
            }
            break;
		case FDS_EVT_WRITE:
			if (p_fds_evt->result == FDS_SUCCESS)
			{
				//write_flag=1;
			}
			write_ok = true;
			break;
		case FDS_EVT_UPDATE:
			if (p_fds_evt->result == FDS_SUCCESS)
			{
				write_ok = true;
			}
			break;
		case FDS_EVT_GC:
			break;
        default:
            break;
    }
}

ret_code_t fds_test_write(void)
{
	//static uint32_t const m_deadbeef[2] = {0xDEADBEEF,0xBAADF00D};
	fds_record_t        record;
	fds_record_desc_t   record_desc;
	fds_record_chunk_t  record_chunk;
	// Set up data.
	record_chunk.p_data         = &SettingsData;
	record_chunk.length_words   = 1;

	// Set up record.
	record.file_id              = FILE_ID;
	record.key              		= REC_KEY;
	record.data.p_chunks       = &record_chunk;
	record.data.num_chunks   = 1;
			
    write_ok = false;
	ret_code_t ret = fds_record_write(&record_desc, &record);
	if (ret != FDS_SUCCESS)
	{
		return ret;
	}
	//NRF_LOG_PRINTF("Writing Record ID = %d \r\n",record_desc.record_id);
	return NRF_SUCCESS;
}

ret_code_t fds_read(void)
{
	fds_flash_record_t  flash_record;
	fds_record_desc_t   record_desc;
	fds_find_token_t    ftok ={0};//Important, make sure you zero init the ftok token
	uint32_t err_code;
	Settings *data;
	
	//NRF_LOG_PRINTF("Start searching... \r\n");
	// Loop until all records with the given key and file ID have been found.
	while (fds_record_find(FILE_ID, REC_KEY, &record_desc, &ftok) == FDS_SUCCESS)
	{
		err_code = fds_record_open(&record_desc, &flash_record);
		if ( err_code != FDS_SUCCESS)
		{
			return err_code;		
		}
		
		//NRF_LOG_PRINTF("Found Record ID = %d\r\n",record_desc.record_id);
		//NRF_LOG_PRINTF("Data = ");
		data = (Settings *) flash_record.p_data;
		memcpy((void*)&SettingsData, (void*)data, flash_record.p_header->tl.length_words * 4);
		//for (uint8_t i=0;i<flash_record.p_header->tl.length_words;i++)
		//{
			//NRF_LOG_PRINTF("0x%8x ",data[i]);
		//}
		//NRF_LOG_PRINTF("\r\n");
		// Access the record through the flash_record structure.
		// Close the record when done.
		err_code = fds_record_close(&record_desc);
		if (err_code != FDS_SUCCESS)
		{
			return err_code;	
		}
	}
	return NRF_SUCCESS;
		
}

static ret_code_t fds_test_find_and_delete (void)
{
	fds_record_desc_t   record_desc;
	fds_find_token_t    ftok;
	ret_code_t ret;
	fds_stat_t stat = {0};

	ftok.page=0;
	ftok.p_addr=NULL;
	// Loop and find records with same ID and rec key and mark them as deleted. 
	while (fds_record_find(FILE_ID, REC_KEY, &record_desc, &ftok) == FDS_SUCCESS)
	{
		fds_record_delete(&record_desc);
		//NRF_LOG_PRINTF("Deleted record ID: %d \r\n",record_desc.record_id);
	}
	// call the garbage collector to empty them, don't need to do this all the time, this is just for demonstration
	
	/*if(fds_stat(&stat) == NRF_SUCCESS)
	{
		if(stat.freeable_words > 10 && stat.)
		{
			ret = fds_gc();
			if (ret != FDS_SUCCESS)
			{
				return ret;
			}
		}
	}*/
	return NRF_SUCCESS;
}

ret_code_t SettingsInit(void)
{
    ret_code_t ret = fds_register(my_fds_evt_handler);
    if (ret != FDS_SUCCESS)
    {
        return ret;    
    }

    ret = fds_init();
    if (ret != FDS_SUCCESS)
    {
        return ret;
    }

    return NRF_SUCCESS;
}

ret_code_t SettingsSave(void)
{
	ret_code_t ret;
	//fds_test_find_and_delete();
	ret = fds_test_write();

	if(ret == FDS_ERR_NO_SPACE_IN_FLASH)
	{
		ret = fds_gc();
		if (ret != FDS_SUCCESS)
		{
			return ret;
		}
		ret = fds_test_write();
	}

    return ret;
}


ret_code_t SettingsLoad(void)
{
	fds_flash_record_t  flash_record;
	fds_record_desc_t   record_desc;
	fds_find_token_t    ftok ={0};//Important, make sure you zero init the ftok token
	fds_find_token_t    ftok2 ={0};//Important, make sure you zero init the ftok token
	uint32_t err_code;
	Settings *data;
	int32_t i = 0;
	
	while (fds_record_find(FILE_ID, REC_KEY, &record_desc, &ftok) == FDS_SUCCESS)
	{
		i++;
	}

	if (i > i)
	{
		return FDS_ERR_INTERNAL;
	}

	while (fds_record_find(FILE_ID, REC_KEY, &record_desc, &ftok2) == FDS_SUCCESS)
	{
		err_code = fds_record_open(&record_desc, &flash_record);
		if ( err_code != FDS_SUCCESS)
		{
			return err_code;		
		}
		
		data = (Settings *) flash_record.p_data;
		memcpy((void*)&SettingsData, (void*)data, (flash_record.p_header->tl.length_words * 4 <= sizeof(SettingsData)) ? (flash_record.p_header->tl.length_words * 4) : sizeof(SettingsData));
		
		err_code = fds_record_close(&record_desc);
		if (err_code != FDS_SUCCESS)
		{
			return err_code;	
		}
	}

    return NRF_SUCCESS;
}

ret_code_t SettingsDefault(void)
{
    SettingsData.MaxForwardSpeed = 255;
    SettingsData.MaxBackwardSpeed = 255;
    SettingsData.MaxLeft = 255;
    SettingsData.MaxRight = 255;

	fds_file_delete(FILE_ID);

	return SettingsSave();
}
