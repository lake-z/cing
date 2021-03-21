#ifndef ___DRIVERS_ACPI
#define ___DRIVERS_ACPI

#include "base.h"

void acpi_bootstrap_64(const byte_t *multi_boot_info, usz_t len);
void acpi_bootstrap_32(const byte_t *multi_boot_info, usz_t len);

#endif
