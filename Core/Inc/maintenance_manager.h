#ifndef __MAINTENANCE_MANAGER_H__
#define __MAINTENANCE_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void MaintenanceManager_Init(void);
void MaintenanceManager_Task(uint32_t tick_ms);

#ifdef __cplusplus
}
#endif

#endif /* __MAINTENANCE_MANAGER_H__ */
