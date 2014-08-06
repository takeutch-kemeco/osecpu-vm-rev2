	ORG 0x100
	MOV AH,0x09
	MOV DX,msg
	INT 0x21
    RET
msg:
	DB  'hello, world',10,'$'
