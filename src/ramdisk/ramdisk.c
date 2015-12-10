/*
 * ramdisk.c
 *
 *  Created on: Dec 10, 2015
 *      Author: jcobb
 */
#include "ramdisk.h"

static struct record_t list[RAMDISK_SIZE];

static struct record_t *valid_head;

static struct record_t *deleted_head;

static struct record_t *current;


void ramdisk_init()
{
    int i;

    for(i = 0; i < RAMDISK_SIZE - 1; i++)
    {
        list[i].next = &list[i + 1];
    }

    list[i].next = NULL;

    deleted_head = &list[0];

    valid_head = NULL;

}
int ramdisk_write(struct record_t to_write)
{
	struct record_t *temp;

	if(deleted_head == NULL)
		return 0x00;

    if(valid_head == NULL)
    {
        valid_head = deleted_head;
        deleted_head = deleted_head ->next;
        valid_head->wan_msg = to_write.wan_msg;
        valid_head ->next = NULL;
        return 0xff;
    }

    else
    {
    	current = valid_head;
    	while(current ->next != NULL)
    	{
    		current = current ->next;
    	}

    	temp = deleted_head ->next;
    	current ->next = deleted_head;
    	deleted_head->wan_msg = to_write.wan_msg;
    	deleted_head ->next = NULL;
    	deleted_head = temp;
    	return 0xff;

    }

}

int ramdisk_erase(struct record_t to_remove)
{
    struct record_t *temp, *temp_2;

//    temp = ramdisk_find(to_remove.mac);
    temp = ramdisk_find(to_remove.wan_msg.tagMac);


    if(temp == NULL)
       return 0x00;

    else if(temp == valid_head)
    {
        temp ->next = deleted_head;
        deleted_head = temp;
        valid_head = NULL;
        return 0xff;
    }

    else
    {
    	current = valid_head;
    	while(current ->next != temp)
    	{
    		current = current ->next;
    	}
    	current ->next = temp ->next;

    	temp_2 = deleted_head;
        deleted_head = temp;
        deleted_head ->next = temp_2;

        return 0xff;
    }

}

void print_records()
{
    current = valid_head;
    while (current != NULL )
        {
            printf( "%" PRIu64 "\n", current->wan_msg.tagMac);
            printf( "%d\n", current->wan_msg.tagRssi);
            printf( "%d\n", current->wan_msg.tagBattery);
            current = current->next;
        }
}

struct record_t *ramdisk_find(uint64_t target)
{
    current = valid_head;

//    while(current ->mac != target && current != NULL)
//    {
//        current = current -> next;
//    }

    while(current->wan_msg.tagMac != target && current != NULL)
    {
        current = current -> next;
    }

	return current;

}
struct record_t *ramdisk_next(struct record_t *target)
{
    if(target == NULL)
    {
        return valid_head;
    }

//    ramdisk_find((*target).mac);

    ramdisk_find((*target).wan_msg.tagMac);

    if(current == NULL)
    	return NULL;

    return(current ->next);

}

