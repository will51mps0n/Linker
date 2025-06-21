	lw	0	1	AddI
	lw	0	4	End
start	jalr	4	7
    lw      0       5       Ex
five  lw      0       0       Big
	sw      0       1       AlPav
	beq	0	0	start
done	halt
six     .fill   Big
Big  	.fill	5
SubAdr  .fill   five
Ug4     .fill   -10
Undef .fill start
AlPav .fill   Ug1