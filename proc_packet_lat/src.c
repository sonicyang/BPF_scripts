#include <uapi/linux/ptrace.h>
#include <linux/irq.h>
#include <linux/netdevice.h>
#include <linux/irqdesc.h>
#include <linux/interrupt.h>

#define KBUILD_MODNAME "net_tracer"
#include "ixgbe.h"

struct event_t {
    u64 ts;
    char rxtx[4];
};
BPF_PERF_OUTPUT(events);

static bool has_rx(struct ixgbe_q_vector *q_vector) {
    union ixgbe_adv_rx_desc *rx_desc;
    unsigned int size;

    rx_desc = (&(((union ixgbe_adv_rx_desc *)((q_vector->rx.ring)->desc))[q_vector->rx.ring->next_to_clean]));
    size = rx_desc->wb.upper.length;

    return size > 0;
}

static bool has_tx(struct ixgbe_q_vector *q_vector) {
    struct ixgbe_ring *tx_ring = q_vector->tx.ring;
    unsigned int i = tx_ring->next_to_clean;
    struct ixgbe_tx_buffer *tx_buffer = &tx_ring->tx_buffer_info[i];
    union ixgbe_adv_tx_desc *eop_desc = tx_buffer->next_to_watch;

    return eop_desc != NULL;
}

int ixgbe_msix_interrupt_probe(struct pt_regs *ctx, int irq, void *data) {
    struct ixgbe_q_vector *q_vector = data;
    bool tx, rx;
    rx = has_rx(q_vector);
    tx = has_tx(q_vector);

    struct event_t event = {};

    event.ts = bpf_ktime_get_ns();

    if(tx) {
        event.rxtx[0] = 't';
        event.rxtx[1] = 'x';
        event.rxtx[2] = '\0';
    }

    if(rx) {
        event.rxtx[0] = 'r';
        event.rxtx[1] = 'x';
        event.rxtx[2] = '\0';
    }

    if(tx || rx) {
        events.perf_submit(ctx, &event, sizeof(event));
    }

    return 0;
}

int ixgbe_poll_probe(struct pt_regs *ctx, struct napi_struct *napi, int budget) {
    bpf_trace_printk("ixgbe poll %s %d \\n", napi->dev->name, budget);
    return 0;
}

int ixgbe_poll_retprobe(struct pt_regs *ctx, struct napi_struct *napi, int budget) {
    int ret = PT_REGS_RC(ctx);
    bpf_trace_printk("ixgbe poll %s %d\\n", napi->dev->name, ret);
    return 0;
}

