	lw $s1, $zero, $imm, 0x100			# load upper left corner
	lw $s0, $zero, $imm, 0x101			# load edge length
	and $s2, $s1, $imm, 0xFF			# set s2 to corner column index (8 least significant bits)
	srl $s1, $s1, $imm, 8				# set s0 to corner row index (8 most significant bits)

	add $t1, $zero, $imm, 256			# set constant 256 to compare against
	add $t0, $s1, $s0, 0				# add edge length to corner row index to find last row
	bge $imm, $t0, $t1, END 			# last row out of screen - fail
	add $t0, $s2, $s0, 0 				# add edge length to corner column index to find last column
	bge $imm, $t0, $t1, END 			# last column out of screen - fail

	add $a2, $zero, $imm, 255 			# a2 is 255 (white) in all calls to WRITE_PIXEL
	add $t0, $zero, $zero, 0 			# init row loop index (i)
ROW_LOOP:
	add $t1, $zero, $zero, 0 			# init column loop index (j)
COLUMN_LOOP:
	add $a0, $s1, $t0, 0 				# set row argument to corner row index + i 
	add $a1, $s2, $t1, 0 				# set column argument to corner column index + j
	jal $ra, $imm, $zero, WRITE_PIXEL 	# call WRITE_PIXEL with the set arguments
	add $t1, $t1, $imm, 1 				# j++
	blt $imm, $t1, $s0, COLUMN_LOOP 	# loop back in column loop if j < edge length
	add $t2, $t2, $imm, 1 				# i++
	blt $imm, $t2, $s0, ROW_LOOP 		# loop back in row loop if i < edge length

END:
	halt $zero, $zero, $zero, 0			# halt

WRITE_PIXEL: 							# function to write the value $a2 to pixel ($a0, $a1)
	sll $t0, $a0, $imm, 8 				# multiply row number by 256
	add $t1, $t0, $a1, 0 				# calculate 256 * row + column which is the pixel index
	out $a2, $zero, $imm, 21 			# set monitordata to $a2 to write that value
	add $t2, $zero, $imm, 1
	out $t2, $zero, $imm, 22 			# set monitorcmd to 1 in order to write pixel to the monitor
	beq $ra, $zero, $zero, 0 			# return

	.word 0x100 100
	.word 0x101 100
