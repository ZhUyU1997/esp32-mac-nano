/*****************************************************************************
 * IWM stub — behaviour notes: upstream pce/src/arch/macplus/iwm.c
 *****************************************************************************/

#include <string.h>

#include "iwm.h"

#define MAC_IWM_CA0 0x01
#define MAC_IWM_CA1 0x02
#define MAC_IWM_CA2 0x04
#define MAC_IWM_LSTRB 0x08
#define MAC_IWM_ENABLE 0x10
#define MAC_IWM_SELECT 0x20
#define MAC_IWM_Q6 0x40
#define MAC_IWM_Q7 0x80

static int mac_iwm_get_drive_status(mac_iwm_t *iwm)
{
	unsigned reg;
	int val;
	mac_iwm_drive_t *drv;

	drv = iwm->curdrv;

	if (drv == NULL) {
		return (1);
	}

	reg = (unsigned)(iwm->lines & 7);

	if (iwm->head_sel) {
		reg |= 8;
	}

	switch (reg) {
	case 0:
		val = (0 == 0);
		break;
	case 1:
		val = (0 == 0);
		break;
	case 2:
		val = ((drv->motor_on != 0) == 0);
		break;
	case 3:
		val = (drv->disk_switched != 0);
		break;
	case 4:
		val = (1 == 0);
		break;
	case 5:
		val = 0;
		break;
	case 6:
		val = 0;
		break;
	case 7:
		val = (1 == 0);
		break;
	case 8:
		val = (drv->disk_inserted == 0);
		break;
	case 9:
		val = (0 == 0);
		break;
	case 10:
		val = ((drv->cur_cyl == 0) == 0);
		break;
	case 11:
		val = (0 == 0);
		break;
	case 12:
		val = (1 == 0);
		break;
	case 13:
		val = 1;
		break;
	case 14:
		val = (1 == 0);
		break;
	case 15:
		val = 1;
		break;
	default:
		val = 1;
		break;
	}

	return (val);
}

static void mac_iwm_access_uint8(mac_iwm_t *iwm, unsigned reg)
{
	switch (reg & 0x0f) {
	case 0x00:
		iwm->lines &= ~MAC_IWM_CA0;
		break;
	case 0x01:
		iwm->lines |= MAC_IWM_CA0;
		break;
	case 0x02:
		iwm->lines &= ~MAC_IWM_CA1;
		break;
	case 0x03:
		iwm->lines |= MAC_IWM_CA1;
		break;
	case 0x04:
		iwm->lines &= ~MAC_IWM_CA2;
		break;
	case 0x05:
		iwm->lines |= MAC_IWM_CA2;
		/* Drive control on CA2 (PCE set cntrl): reg 3 = eject
		 * (disk removed + disk-changed event), reg 4 = CLRSWITCHED
		 * (guest acknowledges the disk-change event). */
		if (iwm->curdrv != NULL) {
			unsigned ctrl_reg = (iwm->lines & 3) | (iwm->head_sel ? 4 : 0);
			if (ctrl_reg == 3) {
				iwm->curdrv->disk_inserted = 0;
				iwm->curdrv->disk_switched = 1;
			} else if (ctrl_reg == 4) {
				iwm->curdrv->disk_switched = 0;
			}
		}
		break;
	case 0x06:
		iwm->lines &= ~MAC_IWM_LSTRB;
		break;
	case 0x07:
		iwm->lines |= MAC_IWM_LSTRB;
		break;
	case 0x08:
		iwm->lines &= ~MAC_IWM_ENABLE;
		break;
	case 0x09:
		iwm->lines |= MAC_IWM_ENABLE;
		break;
	case 0x0a:
		iwm->lines &= ~MAC_IWM_SELECT;
		break;
	case 0x0b:
		iwm->lines |= MAC_IWM_SELECT;
		break;
	case 0x0c:
		iwm->lines &= ~MAC_IWM_Q6;
		break;
	case 0x0d:
		iwm->lines |= MAC_IWM_Q6;
		break;
	case 0x0e:
		iwm->lines &= ~MAC_IWM_Q7;
		break;
	case 0x0f:
		iwm->lines |= MAC_IWM_Q7;
		break;
	}
}

void mac_iwm_init(mac_iwm_t *iwm)
{
	memset(iwm, 0, sizeof(*iwm));
	iwm->handshake = 0x7f;
	iwm->curdrv = &iwm->drv[0];
}

void mac_iwm_set_head_sel(mac_iwm_t *iwm, unsigned char val)
{
	val = (unsigned char)(val != 0);

	if (iwm->head_sel == val) {
		return;
	}

	iwm->head_sel = val;
}

void mac_iwm_set_uint8(mac_iwm_t *iwm, unsigned long addr, unsigned char val)
{
	unsigned reg;

	if ((addr & 1) == 0) {
		return;
	}

	reg = (unsigned)((addr >> 9) & 0x0f);

	mac_iwm_access_uint8(iwm, reg);

	if ((iwm->lines & (MAC_IWM_Q6 | MAC_IWM_Q7)) == (MAC_IWM_Q6 | MAC_IWM_Q7)) {
		if (iwm->lines & MAC_IWM_ENABLE) {
		} else {
			iwm->mode = val;
		}
	}
}

unsigned char mac_iwm_get_uint8(mac_iwm_t *iwm, unsigned long addr)
{
	unsigned reg;
	unsigned val;

	if ((addr & 1) == 0) {
		return (0);
	}

	reg = (unsigned)((addr >> 9) & 0x0f);

	mac_iwm_access_uint8(iwm, reg);

	val = 0;

	switch (iwm->lines & (MAC_IWM_Q6 | MAC_IWM_Q7)) {
	case 0x00:
		if (iwm->lines & MAC_IWM_ENABLE) {
			val = 0;
		} else {
			val = 0xff;
		}
		break;

	case MAC_IWM_Q6:
		val = (unsigned)(iwm->mode & 0x1f);
		if (iwm->lines & MAC_IWM_ENABLE) {
			val |= 0x20;
		}
		if (mac_iwm_get_drive_status(iwm)) {
			val |= 0x80;
		}
		break;

	case MAC_IWM_Q7:
		val = (unsigned)(iwm->handshake & 0x7f);
		val |= 0x80;
		break;

	case (MAC_IWM_Q6 | MAC_IWM_Q7):
		break;

	default:
		break;
	}

	return ((unsigned char)val);
}


/* Insert: disk present + disk-changed event (idempotent on inserted). */
void mac_iwm_insert(mac_iwm_t *iwm, unsigned drive)
{
	if (iwm != NULL && drive < MAC_IWM_DRIVES) {
		if (iwm->drv[drive].disk_inserted)
			return;
		iwm->drv[drive].disk_inserted = 1;
		iwm->drv[drive].disk_switched = 1;
	}
}

