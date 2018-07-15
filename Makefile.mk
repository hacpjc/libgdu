obj-y :=

fio-obj-y :=
fio-obj-y += fio_easyrw.o
fio-obj-y += fio_lock.o
obj-y += $(addprefix fio/, $(fio-obj-y))

hexdump-obj-y :=
hexdump-obj-y += hexdump.o
obj-y += $(addprefix hexdump/, $(hexdump-obj-y))

#;