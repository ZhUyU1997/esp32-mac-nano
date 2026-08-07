#include "msg.h"

#include "macplus.h"
#include <string.h>
#include <asm/mutex.h>

#define MAC_MSG_Q_DEPTH 8
#define MAC_MSG_HANDLERS_MAX 16

typedef struct {
	char *msg; /* internal copy (injected allocator); freed after dispatch */
	char *val;
} mac_msg_slot_t;

static mac_msg_slot_t q[MAC_MSG_Q_DEPTH];
static volatile uint16_t q_head;
static volatile uint16_t q_tail;
static asm_mutex_t mac_msg_mutex = ASM_MUTEX_INITIALIZER;

typedef struct {
	const char *msg;
	mac_msg_set_fn fn;
} mac_msg_list_t;

static mac_msg_list_t handlers[MAC_MSG_HANDLERS_MAX];
static int handler_count;
static macplus_t *g_sim;
static void *(*g_alloc)(size_t);
static void (*g_dealloc)(void *);

void mac_msg_init(macplus_t *sim, void *(*alloc)(size_t), void (*dealloc)(void *))
{
	g_sim = sim;
	g_alloc = alloc;
	g_dealloc = dealloc;
	asm_mutex_init(&mac_msg_mutex);
}

void mac_msg_register(const char *msg, mac_msg_set_fn fn)
{
	if (handler_count < MAC_MSG_HANDLERS_MAX) {
		handlers[handler_count].msg = msg;
		handlers[handler_count].fn = fn;
		handler_count += 1;
	}
}

/* Prefix matching, same as PCE lib/msg.c msg_is_message():
 * "floppy.insert" matches "floppy.insert" and "floppy.insert.xxx". */
static int mac_msg_is_message(const char *m, const char *val)
{
	while (*m != 0) {
		if (strcmp(m, val) == 0)
			return 1;
		while ((*m != 0) && (*m != '.'))
			m += 1;
		if (*m == '.')
			m += 1;
	}
	return 0;
}

void mac_msg_submit(const char *msg, const char *val)
{
	char *msg_copy = NULL;
	char *val_copy = NULL;

	if (msg == NULL || g_alloc == NULL || g_dealloc == NULL)
		return;
	if (val == NULL)
		val = "";

	msg_copy = g_alloc(strlen(msg) + 1);
	val_copy = g_alloc(strlen(val) + 1);
	if (msg_copy == NULL || val_copy == NULL) {
		if (msg_copy != NULL)
			g_dealloc(msg_copy);
		if (val_copy != NULL)
			g_dealloc(val_copy);
		return;
	}
	strcpy(msg_copy, msg);
	strcpy(val_copy, val);

	asm_mutex_lock(&mac_msg_mutex);
	uint16_t next = (uint16_t)((q_tail + 1u) % MAC_MSG_Q_DEPTH);
	if (next == q_head) {
		/* full: drop the oldest slot (and its copies) */
		if (q[q_head].msg != NULL)
			g_dealloc(q[q_head].msg);
		if (q[q_head].val != NULL)
			g_dealloc(q[q_head].val);
		q_head = (uint16_t)((q_head + 1u) % MAC_MSG_Q_DEPTH);
	}
	q[q_tail].msg = msg_copy;
	q[q_tail].val = val_copy;
	q_tail = next;
	asm_mutex_unlock(&mac_msg_mutex);
}

int mac_msg_dispatch(void)
{
	mac_msg_slot_t slot;

	/* Fast path: unlocked when empty (volatile head/tail — single
	 * producer appends, single consumer pops). */
	if (q_head == q_tail)
		return 0;

	asm_mutex_lock(&mac_msg_mutex);
	if (q_head == q_tail) {
		asm_mutex_unlock(&mac_msg_mutex);
		return 0;
	}
	slot = q[q_head];
	q_head = (uint16_t)((q_head + 1u) % MAC_MSG_Q_DEPTH);
	asm_mutex_unlock(&mac_msg_mutex);

	for (int i = 0; i < handler_count; i++) {
		if (mac_msg_is_message(handlers[i].msg, slot.msg)) {
			handlers[i].fn(g_sim, slot.msg, slot.val);
			break;
		}
	}

	g_dealloc(slot.msg);
	g_dealloc(slot.val);
	return 1;
}
