#include <FreeRTOS.h>
#include <task.h>

void
vApplicationMallocFailedHook(void)
{
}

void
vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
	(void)pxTask;
	(void)pcTaskName;
}
