
#ifndef __CPU_H
#define __CPU_H

#include <stdbool.h>
#include "arm_cpu.h"
#include "tm4c123gh6pm.h"

extern uintptr_t __stack_start;     ///< end of user stack memory space

//void lpc2387_pclk_scale(uint32_t source, uint32_t target, uint32_t *pclksel, uint32_t *prescale);
bool install_irq(int IntNumber, void (*HandlerAddr)(void), int Priority);

#endif /* __CPU_H */
