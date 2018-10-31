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

rbtree-obj-y :=
rbtree-obj-y += rbtree.o
obj-y += $(addprefix rbtree/, $(rbtree-obj-y))

fifobuf-obj-y :=
fifobuf-obj-y += fifobuf.o
obj-y += $(addprefix fifobuf/, $(fifobuf-obj-y))

ctrie-obj-y :=
ctrie-obj-y += ctrie.o
obj-y += $(addprefix ctrie/, $(ctrie-obj-y))

#;
