obj-y :=

fio-obj-y :=
fio-obj-y += fio_easyrw.o
fio-obj-y += fio_lock.o
obj-y += $(addprefix fio/, $(fio-obj-y))

hexdump-obj-y :=
hexdump-obj-y += hexdump.o
obj-y += $(addprefix hexdump/, $(hexdump-obj-y))

semlock-obj-y :=
semlock-obj-y += semlock.o
obj-y += $(addprefix semlock/, $(semlock-obj-y))

dirtrav-obj-y :=
dirtrav-obj-y += dirtrav.o
obj-y += $(addprefix dirtrav/, $(dirtrav-obj-y))

#;
