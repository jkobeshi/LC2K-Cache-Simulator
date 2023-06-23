		lw	0	1	data1	$1= mem[data1]
		add	0	1	2
		lw	0	1	data1
		beq	1	1	done
done	lw	0	1	data1
		add 0	0	2
		lw	0	1	data1
		beq	0	0	done1
done1	lw	0	1	data1
		nor	1	1	2
		lw	0	1	data1
		nor	0	0	2
		halt
data1	.fill	12345
