	lw $s0, $zero, $imm, 0x103				# load first sector number
	lw $s1, $zero, $imm, 0x104				# load second sector number
	add $t0, $zero, $imm, READ_DONE			# set $t0 to address of READ_DONE handler
	out $t0, $zero, $imm, 6					# set irqhandler to READ_DONE
	out $imm, $zero, $imm, 1				# enable irq1 handler
	add $s2, $zero, $imm, 0x100				# set $s2 to 0x100. This register value is used by the ISR to know where to write the sum result.

	add $sp, $zero, $imm, 0x800				# set $sp = 0x800 = 2048

	out $s0, $zero, $imm, 15				# set disksector to read the first sector
	add $t0, $zero, $imm, 0x200				# set $t0 to address of first read buffer
	out $t0, $zero, $imm, 16				# set diskbuffer to address of read buffer
	add $t0, $zero, $imm, 1					# set $t0 to 1 respresenting a read command
	out $t0, $zero, $imm, 14				# send read command to disk to read the first sector

BUSY_WAIT1: 								# busy wait for first read to end because we have nothing better to do now
	in $t0, $zero, $imm, 17					# read diskstatus
	bne $imm, $t0, $zero, BUSY_WAIT1		# continue waiting for disk to become ready if not ready

	out $s1, $zero, $imm, 15				# set disksector to read the second sector
	add $t0, $zero, $imm, 0x300				# set $t0 to address of second read buffer
	out $t0, $zero, $imm, 16				# set diskbuffer to address of read buffer
	add $t0, $zero, $imm, 1					# set $t0 to 1 respresenting a read command
	out $t0, $zero, $imm, 14				# send read command to disk to read the second sector

BUSY_WAIT2: 								# busy wait for second read to end (and the interrupt to happen)
	in $t0, $zero, $imm, 17					# read diskstatus
	bne $imm, $t0, $zero, BUSY_WAIT2		# continue waiting for disk to become ready if not ready

	lw $t0, $zero, $imm, 0x100				# load first sum
	lw $t1, $zero, $imm, 0x101				# load second sum
	bgt $imm, $t1, $t0, SECOND_SUM_BIGGER	# compare the sums
	sw $t0, $zero, $imm, 0x102				# first sum bigger, store it at 0x102
	halt $zero, $zero, $zero, 0
SECOND_SUM_BIGGER:
	sw $t1, $zero, $imm, 0x102				# second sum bigger, store it at 0x102
	halt $zero, $zero, $zero, 0

SUM_8_WORDS:								# function to sum the first 8 words starting at the address given by $a0
	add $sp, $sp, $imm, -3					# adjust stack for 2 items
	sw $s0, $sp, $imm, 2					# save $s0
	sw $s1, $sp, $imm, 1					# save $s1
	sw $s2, $sp, $imm, 0					# save $s2
	add $s0, $zero, $zero, 0				# init loop counter (i=0)
	add $v0, $zero, $zero, 0				# init sum
	add $s1, $zero, $imm, 8 				# set $s1 as 8 (loop iteration count)
LOOP:
	lw $s2, $a0, $s0, 0						# load buffer[i] into $s2
	add $v0, $v0, $s2, 0					# increment sum by buffer[i]
	add $s0, $s0, $imm, 1					# i++
	blt $imm, $s0, $s1, LOOP 				# loop back in row loop if i < 8
	lw $s2, $sp, $imm, 0					# restore $s2
	lw $s1, $sp, $imm, 1					# restore $s1
	lw $s0, $sp, $imm, 2					# restore $s0
	add $sp, $sp, $imm, 3					# pop 3 items from stack
	beq $ra, $zero, $zero, 0 				# return

READ_DONE:									# interrupt handler for when a read is done
	add $sp, $sp, $imm, -3					# adjust stack for 3 items (saved in case a previous function call is still processed)
	sw $a0, $sp, $imm, 2					# save $a0
	sw $v0, $sp, $imm, 1					# save $v0
	sw $ra, $sp, $imm, 0					# save return address
	in $a0, $zero, $imm, 16					# set $a0 to address of read buffer
	jal $ra, $imm, $zero, SUM_8_WORDS 		# call SUM_8_WORDS to sum the first 8 words of the buffer
	sw $v0, $zero, $s2, 0					# save the sum in 0x100 or 0x101
	add $s2, $s2, $imm, 1					# increment $s0 so that the second read is stored in 0x102
	lw $ra, $sp, $imm, 0					# restore return address
	lw $v0, $sp, $imm, 1					# restore $v0
	lw $a0, $sp, $imm, 2					# restore $a0
	add $sp, $sp, $imm, 3					# pop 3 items from stack
	out $zero, $zero, $imm, 4				# clear irq1 status
	reti $zero, $zero, $zero, 0				# return from interrupt

	.word 0x103 0
	.word 0x104 1
