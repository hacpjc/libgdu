obj-y :=

fio-obj-y :=
fio-obj-y += fio_easyrw.o
fio-obj-y += fio_lock.o
obj-y += $(addprefix fio/, $(fio-obj-y))

#;