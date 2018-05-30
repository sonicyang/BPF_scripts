#!/usr/bin/env python
import os
import ctypes as ct
from bcc import BPF

DIR = os.path.dirname(os.path.realpath(__file__))

IXGBE_SRC = "/home/sonicyang/workspace/ixgbe-5.3.6"
BPF_SRC = DIR + "/src.c"

class Data(ct.Structure):
    _fields_ = [("ts", ct.c_ulonglong), ("rxtx", ct.c_char * 4)]

def print_event(cpu, data, size):
    event = ct.cast(data, ct.POINTER(Data)).contents
    print("%-18.9f\t%s" % (event.ts , event.rxtx))

def __main__():

    src = ""
    with open(BPF_SRC) as fil:
        src = fil.read()

    # load BPF program
    b = BPF(text=src, cflags=["-I" + IXGBE_SRC + "/src"])

    b.attach_kprobe(event="ixgbe_msix_clean_rings", fn_name="ixgbe_msix_interrupt_probe")
    #b.attach_kprobe(event="ixgbe_poll", fn_name="ixgbe_poll_probe")
    #b.attach_kretprobe(event="ixgbe_poll", fn_name="ixgbe_poll_retprobe")
    print("Tracing ... Ctrl-C to end")

    b["events"].open_perf_buffer(print_event)
    while 1:
        b.kprobe_poll()

__main__()
