#
# Makefile for busy-wait IO tests
#
XCC     = gcc
AS		= as
LD      = ld
CFLAGS  = -O2 -DDEBUG -c -fPIC -Wall -I. -I./include -mcpu=arm920t -msoft-float -nostdlib
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -init main -Map kernel.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2

TASK_OBJS = tasks/first_task.o \
			tasks/main_task.o \
			tasks/name_server.o \
			tasks/idle_task.o \
			tasks/clock_server.o \
			tasks/event_notifier.o \
			tasks/uart_server.o \
			tasks/train_command.o \
			tasks/term_server.o \
			tasks/cli.o \
			tasks/sensor_server.o \
			tasks/switch_server.o \
			tasks/cab_service.o \
			tasks/train_controller.o \
			tasks/train.o \
			tasks/routing.o \
			tasks/track_util.o \
			tasks/track_server.o \
			calib/stopping_distance.o \
			calib/velocity.o \
			calib/acceleration.o
			
OBJS = 	track_data/track_data.o \
		trains/train_profile.o \
		trains/train41_profile.o \
		trains/train35_profile.o \
		trains/train36_profile.o \
 		main.o \
		kernel.o \
		io.o \
		interrupts.o \
		bwio.o \
		string.o \
		timer.o \
		cbuf.o \
		cs.o \
		task.o \
		scheduler.o \
		syscall.o \
		syscall_kern.o \
		task_queue.o \
		ordered_list.o \
		list.o \
		clist.o \
		heap.o \
		debug.o \
		uart.o \
		location.o \
		${TASK_OBJS}

all: kernel.elf

com2_off: CFLAGS += -DBW_COM2
com2_off: all

com1_off: CFLAGS += -DBW_COM1
com1_off: all

main.s: main.c
	$(XCC) -S $(CFLAGS) main.c
main.o: main.s
	$(AS) $(ASFLAGS) -o main.o main.s

kernel.s: kernel.c
	$(XCC) -S $(CFLAGS) kernel.c
kernel.o: kernel.s
	$(AS) $(ASFLAGS) -o kernel.o kernel.s

io.s: io.c
	$(XCC) -S $(CFLAGS) io.c
io.o: io.s
	$(AS) $(ASFLAGS) -o io.o io.s

interrupts.s: interrupts.c
	$(XCC) -S $(CFLAGS) interrupts.c
interrupts.o: interrupts.s
	$(AS) $(ASFLAGS) -o interrupts.o interrupts.s

string.s: string.c
	$(XCC) -S $(CFLAGS) string.c
string.o: string.s
	$(AS) $(ASFLAGS) -o string.o string.s

debug.s: debug.c
	$(XCC) -S $(CFLAGS) debug.c
debug.o: debug.s
	$(AS) $(ASFLAGS) -o debug.o debug.s

bwio.s: bwio.c
	$(XCC) -S $(CFLAGS) bwio.c
bwio.o: bwio.s
	$(AS) $(ASFLAGS) -o bwio.o bwio.s

timer.s: timer.c
	$(XCC) -S $(CFLAGS) timer.c
timer.o: timer.s
	$(AS) $(ASFLAGS) -o timer.o timer.s

uart.s: uart.c
	$(XCC) -S $(CFLAGS) uart.c
uart.o: uart.s
	$(AS) $(ASFLAGS) -o uart.o uart.s

cbuf.s: cbuf.c
	$(XCC) -S $(CFLAGS) cbuf.c
cbuf.o: cbuf.s
	$(AS) $(ASFLAGS) -o cbuf.o cbuf.s

cs.o: cs.asm
	$(AS) $(ASFLAGS) -o cs.o cs.asm

task.s: task.c
	$(XCC) -S $(CFLAGS) task.c
task.o: task.s
	$(AS) $(ASFLAGS) -o task.o task.s

task_queue.s: task_queue.c
	$(XCC) -S $(CFLAGS) task_queue.c
task_queue.o: task_queue.s
	$(AS) $(ASFLAGS) -o task_queue.o task_queue.s

scheduler.s: scheduler.c
	$(XCC) -S $(CFLAGS) scheduler.c
scheduler.o: scheduler.s
	$(AS) $(ASFLAGS) -o scheduler.o scheduler.s

syscall.o: syscall.asm
	$(AS) $(ASFLAGS) -o syscall.o syscall.asm

syscall_kern.s: syscall_kern.c
	$(XCC) -S $(CFLAGS) syscall_kern.c
syscall_kern.o: syscall_kern.s
	$(AS) $(ASFLAGS) -o syscall_kern.o syscall_kern.s

heap.s: heap.c
	$(XCC) -S $(CFLAGS) heap.c
heap.o: heap.s
	$(AS) $(ASFLAGS) -o heap.o heap.s
	
list.s: list.c
	$(XCC) -S $(CFLAGS) list.c
list.o: list.s
	$(AS) $(ASFLAGS) -o list.o list.s

clist.s: clist.c
	$(XCC) -S $(CFLAGS) clist.c
clist.o: clist.s
	$(AS) $(ASFLAGS) -o clist.o clist.s

ordered_list.s: ordered_list.c
	$(XCC) -S $(CFLAGS) ordered_list.c
ordered_list.o: ordered_list.s
	$(AS) $(ASFLAGS) -o ordered_list.o ordered_list.s

location.s: location.c
	$(XCC) -S $(CFLAGS) $<
location.o: location.s
	$(AS) $(ASFLAGS) -o $@ $<

first_task.s: tasks/first_task.c
	$(XCC) -S $(CFLAGS) tasks/first_task.c
tasks/first_task.o: first_task.s
	$(AS) $(ASFLAGS) -o tasks/first_task.o first_task.s

main_task.s: tasks/main_task.c
	$(XCC) -S $(CFLAGS) tasks/main_task.c
tasks/main_task.o: main_task.s
	$(AS) $(ASFLAGS) -o tasks/main_task.o main_task.s

name_server.s: tasks/name_server.c
	$(XCC) -S $(CFLAGS) tasks/name_server.c
tasks/name_server.o: name_server.s
	$(AS) $(ASFLAGS) -o tasks/name_server.o name_server.s

term_server.s: tasks/term_server.c
	$(XCC) -S $(CFLAGS) tasks/term_server.c
tasks/term_server.o: term_server.s
	$(AS) $(ASFLAGS) -o tasks/term_server.o term_server.s

cli.s: tasks/cli.c
	$(XCC) -S $(CFLAGS) tasks/cli.c
tasks/cli.o: cli.s
	$(AS) $(ASFLAGS) -o tasks/cli.o cli.s

sensor_server.s: tasks/sensor_server.c
	$(XCC) -S $(CFLAGS) tasks/sensor_server.c
tasks/sensor_server.o: sensor_server.s
	$(AS) $(ASFLAGS) -o tasks/sensor_server.o sensor_server.s

switch_server.s: tasks/switch_server.c
	$(XCC) -S $(CFLAGS) tasks/switch_server.c
tasks/switch_server.o: switch_server.s
	$(AS) $(ASFLAGS) -o tasks/switch_server.o switch_server.s

train.s: tasks/train.c
	$(XCC) -S $(CFLAGS) $<
tasks/train.o: train.s
	$(AS) $(ASFLAGS) -o $@ $<
	
track_server.s: tasks/track_server.c
	$(XCC) -S $(CFLAGS) $<
tasks/track_server.o: track_server.s
	$(AS) $(ASFLAGS) -o $@ $<
	
routing.s: tasks/routing.c
	$(XCC) -S $(CFLAGS) $<
tasks/routing.o: routing.s
	$(AS) $(ASFLAGS) -o $@ $<

track_util.s: tasks/track_util.c
	$(XCC) -S $(CFLAGS) $<
tasks/track_util.o: track_util.s
	$(AS) $(ASFLAGS) -o $@ $<

train_command.s: tasks/train_command.c
	$(XCC) -S $(CFLAGS) tasks/train_command.c
tasks/train_command.o: train_command.s
	$(AS) $(ASFLAGS) -o tasks/train_command.o train_command.s

idle_task.s: tasks/idle_task.c
	$(XCC) -S $(CFLAGS) tasks/idle_task.c
tasks/idle_task.o: idle_task.s
	$(AS) $(ASFLAGS) -o tasks/idle_task.o idle_task.s

clock_server.s: tasks/clock_server.c
	$(XCC) -S $(CFLAGS) tasks/clock_server.c
tasks/clock_server.o: clock_server.s
	$(AS) $(ASFLAGS) -o tasks/clock_server.o clock_server.s

uart_server.s: tasks/uart_server.c
	$(XCC) -S $(CFLAGS) tasks/uart_server.c
tasks/uart_server.o: uart_server.s
	$(AS) $(ASFLAGS) -o tasks/uart_server.o uart_server.s

event_notifier.s: tasks/event_notifier.c
	$(XCC) -S $(CFLAGS) tasks/event_notifier.c
tasks/event_notifier.o: event_notifier.s
	$(AS) $(ASFLAGS) -o tasks/event_notifier.o event_notifier.s

cab_service.s: tasks/cab_service.c
	$(XCC) -S $(CFLAGS) tasks/cab_service.c
tasks/cab_service.o: cab_service.s
	$(AS) $(ASFLAGS) -o tasks/cab_service.o cab_service.s
	
train_controller.s: tasks/train_controller.c
	$(XCC) -S $(CFLAGS) tasks/train_controller.c
tasks/train_controller.o: train_controller.s
	$(AS) $(ASFLAGS) -o tasks/train_controller.o train_controller.s

stopping_distance.s: calib/stopping_distance.c
	$(XCC) -S $(CFLAGS) calib/stopping_distance.c
calib/stopping_distance.o: stopping_distance.s
	$(AS) $(ASFLAGS) -o calib/stopping_distance.o stopping_distance.s
	
velocity.s: calib/velocity.c
	$(XCC) -S $(CFLAGS) calib/velocity.c
calib/velocity.o: velocity.s
	$(AS) $(ASFLAGS) -o calib/velocity.o velocity.s

acceleration.s: calib/acceleration.c
	$(XCC) -S $(CFLAGS) $<
calib/acceleration.o: acceleration.s
	$(AS) $(ASFLAGS) -o $@ $<
	
track.s: track_data/track.c
	$(XCC) -S $(CFLAGS) $<
track_data/track.o: track.s
	$(AS) $(ASFLAGS) -o $@ $<

train_profile.s: trains/train_profile.c
	$(XCC) -S $(CFLAGS) $<
trains/train_profile.o: train_profile.s
	$(AS) $(ASFLAGS) -o $@ $<

train41_profile.s: trains/train41_profile.c
	$(XCC) -S $(CFLAGS) $<
trains/train41_profile.o: train41_profile.s
	$(AS) $(ASFLAGS) -o $@ $<

train35_profile.s: trains/train35_profile.c
	$(XCC) -S $(CFLAGS) $<
trains/train35_profile.o: train35_profile.s
	$(AS) $(ASFLAGS) -o $@ $<

train36_profile.s: trains/train36_profile.c
	$(XCC) -S $(CFLAGS) $<
trains/train36_profile.o: train36_profile.s
	$(AS) $(ASFLAGS) -o $@ $<

track_data/track_data.c: track_data/parse_track track_data/tracka track_data/trackb
	./track_data/parse_track ./track_data/tracka ./track_data/trackb -C track_data/track_data.c -H include/track_data.h
track_data.s: track_data/track_data.c
	$(XCC) -S $(CFLAGS) track_data/track_data.c
track_data/track_data.o: track_data.s
	$(AS) $(ASFLAGS) -o track_data/track_data.o track_data.s

kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) -lgcc
	chmod ogu+rx $@
	objdump -d $@ > kernel.dump

clean:
	-rm -f kernel.elf kernel.map kernel.dump
	-rm -f *.s
	-rm -f *.o tasks/*.o track_data/*.o
	-rm -f track_data/track_data.c
	-rm -f include/track_data.h
