#ifndef MAC_TRAP_LOG_H
#define MAC_TRAP_LOG_H

#include <stdint.h>

extern const uint16_t mac_trap_words[];
extern const char *const mac_trap_names[];
extern const unsigned mac_trap_count;

/** Snow systrap_history: mask then table lookup. Returns NULL if unknown. */
const char *mac_trap_name_lookup(uint16_t trap_word);

/** Called from Musashi m68ki_exception_1010 (REG_IR / fault PC). */
void mac_trap_on_1010_exception(void);

#endif
