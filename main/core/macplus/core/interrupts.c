#include "macplus.h"

#include "m68k.h"
#include "m68kconf.h"

int m68k_int_ack(int int_level)
{
	(void)int_level;
	return M68K_INT_ACK_AUTOVECTOR;
}

void mac_irq_reset(macplus_t *s)
{
	int i;
	(void)s;
	/* PCE: sim->intr = 0 — clear all pending interrupts */
	for (i = 7; i >= 1; i--)
		m68k_set_virq((unsigned)i, 0);
}

/* RESET instruction callback.  Full peripheral reset matching PCE. */
void m68k_reset_instr_callback(void)
{
	macplus_t *s = macplus_instance();
	mac_reset(s);
}

void mac_interrupt_scsi(void *ext, unsigned char val)
{
	macplus_t *s = ext;

	if (s->via_port_b & 0x40) {
		val = 0;
	} else {
		val = (val != 0);
	}

	if (val) {
		s->intr_scsi_via |= 2;
	} else {
		s->intr_scsi_via &= ~2;
	}
	m68k_set_virq(1, s->intr_scsi_via != 0);
}

void mac_interrupt_via(void *ext, unsigned char val)
{
	macplus_t *s = ext;

	if (val) {
		s->intr_scsi_via |= 1;
	} else {
		s->intr_scsi_via &= ~1;
	}
	m68k_set_virq(1, s->intr_scsi_via != 0);
}

void mac_set_via_port_a(void *ext, unsigned char val)
{
	macplus_t *s = ext;
	unsigned char old;

	if (s->via_port_a == val) {
		return;
	}

	old = s->via_port_a;
	s->via_port_a = val;

	if ((old ^ val) & 0x10) {
		mac_set_overlay(s, (val & 0x10) != 0);
	}

	if ((old ^ val) & 0x20) {
		mac_iwm_set_head_sel(&s->iwm, (unsigned char)(val & (1 << 5)));
	}

	if ((old ^ val) & 0x40) {
		mac_set_vbuf(s, (val & 0x40) ? s->vbuf1 : s->vbuf2);
	}

	if ((old ^ val) & 0x08) {
		mac_sound_set_sbuf(&s->sound, (val & 0x08) ? s->sbuf1 : s->sbuf2);
	}

	if ((old ^ val) & 0x07) {
		mac_sound_set_volume(&s->sound, val & 7);
	}
}

static void mac_interrupt_sony_check(macplus_t *sim_local)
{
	unsigned int sr;
	unsigned int a7;
	unsigned int pc;

	if (sim_local->sony.check_addr == 0) {
		return;
	}

	sr = m68k_get_reg(NULL, M68K_REG_SR);
	if (((sr >> 8) & 7u) == 7u) {
		return;
	}

	a7 = m68k_get_reg(NULL, M68K_REG_A7);
	pc = m68k_get_reg(NULL, M68K_REG_PC);
	m68k_write_memory_32(a7 - 4, pc);
	m68k_set_reg(M68K_REG_A7, a7 - 4u);
	m68k_set_reg(M68K_REG_PC, (unsigned int)sim_local->sony.check_addr);
}

void mac_interrupt_osi(void *ext, unsigned char val)
{
	macplus_t *sim_local = ext;

	if (val) {
		if (mac_sony_check(&sim_local->sony)) {
			mac_interrupt_sony_check(sim_local);
		}
		e6522_set_ca2_inp(&sim_local->via, 0);
		e6522_set_ca2_inp(&sim_local->via, 1);
	}
}

void mac_set_rtc_data(void *ext, unsigned char v)
{
	macplus_t *s = ext;

	if (v) {
		s->via_port_b |= 0x01;
	} else {
		s->via_port_b &= ~0x01;
	}
	e6522_set_irb_inp(&s->via, s->via_port_b);
}

void mac_set_via_port_b(void *ext, unsigned char val)
{
	macplus_t *s = ext;
	unsigned char old;

	if (s->via_port_b == val) {
		return;
	}

	old = s->via_port_b;
	s->via_port_b = val;

	mac_rtc_set_uint8(&s->rtc, val);

	if ((old ^ val) & 0x80) {
		mac_sound_set_enable(&s->sound, (val & 0x80) == 0);
	}
}

void mac_interrupt_scc(void *ext, unsigned char val)
{
	(void)ext;
	m68k_set_virq(2, val != 0);
}
