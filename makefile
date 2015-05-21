opt: op_commands.o op_util_scan.o op.o fl.o sub.o loop.o gen.o
	cc op_commands.o op_util_scan.o op.o fl.o sub.o loop.o gen.o -o opt

op_commands.o: 	op_commands.c
		cc -c op_commands.c

op_util_scan.o:	op_util_scan.c
		cc -c op_util_scan.c

op.o:		op.c
		cc -c op.c

fl.o:		fl.c
		cc -c fl.c

sub.o:		sub.c
		cc -c sub.c

loop.o:		loop.c
		cc -c loop.c

gen.o:		gen.c
		cc -c gen.c

