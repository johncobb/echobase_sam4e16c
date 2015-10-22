// ramdisk.c

#include "ramdisk.h"
#include <stdlib.h>
#include <string.h>

static tag_msg_t records[SIZE_OF];

static tag_msg_t *valid_head;

static tag_msg_t *deleted_head;

void ramdisk_init()
{
	int i;

	for (i = 0; i < SIZE_OF - 1; i++)
	{
		records[i].next = &records[i + 1];
	}

	records[i].next = NULL;

	deleted_head = &records[0];

	valid_head = NULL;

}

uint8_t ramdisk_write(tag_msg_t to_write)
{
	tag_msg_t *temp_1;

	//Check for out of Memory
	if (deleted_head == NULL )
		return 0x00;

	//Check for no records
	if (valid_head == NULL )
	{
		temp_1 = deleted_head->next;
		valid_head = deleted_head;
		deleted_head = temp_1;


		// the long way
		valid_head->messageType = to_write.messageType;
		valid_head->routerMac = to_write.routerMac;
		valid_head->routerShort = to_write.routerShort;
		valid_head->tagMac = to_write.tagMac;
		valid_head->tagConfigSet = to_write.tagConfigSet;
		valid_head->tagSerial = to_write.tagSerial;
		valid_head->tagStatus = to_write.tagStatus;
		valid_head->tagLqi = to_write.tagLqi;
		valid_head->tagRssi = to_write.tagRssi;
		valid_head->tagBattery = to_write.tagBattery;
		valid_head->tagTemperature = to_write.tagTemperature;
		valid_head->temp = to_write.temp;
		valid_head->next = NULL;

		// the short way
//		memcpy(valid_head, &to_write, sizeof(tag_msg_t));
	}

	else
	{

		// the long way
		deleted_head->messageType = to_write.messageType;
		deleted_head->routerMac = to_write.routerMac;
		deleted_head->routerShort = to_write.routerShort;
		deleted_head->tagMac = to_write.tagMac;
		deleted_head->tagConfigSet = to_write.tagConfigSet;
		deleted_head->tagSerial = to_write.tagSerial;
		deleted_head->tagStatus = to_write.tagStatus;
		deleted_head->tagLqi = to_write.tagLqi;
		deleted_head->tagRssi = to_write.tagRssi;
		deleted_head->tagBattery = to_write.tagBattery;
		deleted_head->tagTemperature = to_write.tagTemperature;
		deleted_head->temp = to_write.temp;
		temp_1 = valid_head;
		valid_head = deleted_head;
		deleted_head = deleted_head->next;
		valid_head->next = temp_1;

		// the short way
//		memcpy(deleted_head, &to_write, sizeof(tag_msg_t));

	}

	return 0xff;

}

uint8_t ramdisk_erase(tag_msg_t to_remove)
{
	tag_msg_t *temp_1 = valid_head;
	//Move temp to target(to_remove);
	while (temp_1->tagMac != to_remove.tagMac && temp_1 != NULL )
	{
		temp_1 = temp_1->next;
	}

	//Handle Erase cases
	if (temp_1 == NULL )
		return 0x00;

	else if (temp_1 == valid_head)
	{
		valid_head = valid_head->next;
		temp_1->next = deleted_head;
		deleted_head = temp_1;
	}

	else
	{
		tag_msg_t *temp_2 = valid_head;
		while (temp_2->next != temp_1)
		{
			temp_2 = temp_2->next;
		}

		temp_2->next = temp_1->next;
		temp_2 = deleted_head;
		deleted_head = temp_1;
		deleted_head->next = temp_2;
	}

	return 0xff;
}

//tag_msg_t * ramdisk_find(uint64_t target)
//{
//	tag_msg_t *temp_1 = valid_head;
//
//	while (temp_1->tagMac != target && temp_1 != NULL )
//	{
//		temp_1 = temp_1->next;
//	}
//
//	return temp_1;
//}

uint8_t ramdisk_find(uint64_t target, tag_msg_t * result)
{
	result = valid_head;

	while (result->tagMac != target && result != NULL )
	{
		result = result->next;
	}

	return 0xff;
}

//tag_msg_t * ramdisk_next(tag_msg_t * target)
//{
//	if (target == NULL )
//		return valid_head;
//	else
//		return (target->next);
//}

uint8_t ramdisk_next(tag_msg_t * target, tag_msg_t * result)
{
	if (target == NULL )
		result = valid_head;
	else
		result = (target->next);

	return 0xff;
}
