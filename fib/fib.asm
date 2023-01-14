	add $s0, $imm, $zero, 0x102 # start generating fibo numbers after the first two
LOOP:
	add $t0, $s0, $imm, -2 # get the memory location for the fibo number 2 before the current (f[n-2])
	lw $t1, $t0, $zero, 0 # load f[n-2] into t1
	add $t0, $s0, $imm, -1 # get the memory location for the fibo number 1 before the current (f[n-1])
	lw $t2, $t0, $zero, 0 # load f[n-1] into t2
	add $t0, $t1, $t2, 0 # calculate f[n] = f[n-2] + f[n-1]
	blt $imm, $t0, $zero, END # test for overflow in the f[n] result by checking if it's negative (should be positive if no OF), end loop if so
	sw $t0, $s0, $zero, 0 # store f[n] in the next memory line
	add $s0, $s0, $imm, 1 # increment current memory line
	add $t0, $imm, $zero, 4096 # set $t0 to maximum memory line
	blt $imm, $s0, $t0, LOOP # loop back if $s0 didn't overflow the memory (it's less than 4096)
END:
	halt $zero, $zero, $zero, 0
	.word 0x100 1
	.word 0x101 1
