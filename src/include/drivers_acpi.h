#ifndef ___DRIVERS_ACPI
#define ___DRIVERS_ACPI

#include "base.h"

void acpi_init_new(const byte_t *multi_boot_info, usz_t len);
void acpi_init_old(const byte_t *multi_boot_info, usz_t len);

#endif
