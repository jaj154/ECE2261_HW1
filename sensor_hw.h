#ifndef SENSOR_HW_H
#define SENSOR_HW_H

#include <stdint.h>

void  sensor_hw_init(void);
float sensor_hw_read_raw(uint32_t tick);

#endif /* SENSOR_HW_H */
