                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   500
__initial_sp
	
                ;PRESERVE8
                ;THUMB


; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     MemManage_Handler         ; MPU Fault Handler
                DCD     BusFault_Handler          ; Bus Fault Handler
                DCD     UsageFault_Handler        ; Usage Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     DebugMon_Handler          ; Debug Monitor Handler
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; PendSV Handler
                DCD     SysTick_Handler           ; SysTick Handler

                ; External Interrupts
                DCD     WDT_IRQHandler            ; 0:  Watchdog Timer
                DCD     RTC_IRQHandler            ; 1:  Real Time Clock
                DCD     TIM0_IRQHandler           ; 2:  Timer0 / Timer1
                DCD     TIM2_IRQHandler           ; 3:  Timer2 / Timer3
                DCD     MCIA_IRQHandler           ; 4:  MCIa
                DCD     MCIB_IRQHandler           ; 5:  MCIb
                DCD     UART0_IRQHandler          ; 6:  UART0 - DUT FPGA
                DCD     UART1_IRQHandler          ; 7:  UART1 - DUT	FPGA
                DCD     UART2_IRQHandler          ; 8:  UART2 - DUT	FPGA
                DCD     UART4_IRQHandler          ; 9:  UART4 - not connected
                DCD     AACI_IRQHandler           ; 10: AACI / AC97
                DCD     CLCD_IRQHandler           ; 11: CLCD Combined Interrupt
                DCD     ENET_IRQHandler           ; 12: Ethernet
                DCD     USBDC_IRQHandler          ; 13: USB Device
                DCD     USBHC_IRQHandler          ; 14: USB Host Controller
                DCD     CHLCD_IRQHandler          ; 15: Character LCD
                DCD     FLEXRAY_IRQHandler        ; 16: Flexray
                DCD     CAN_IRQHandler            ; 17: CAN
                DCD     LIN_IRQHandler            ; 18: LIN
                DCD     I2C_IRQHandler            ; 19: I2C ADC/DAC
                DCD     0                         ; 20: Reserved
                DCD     0                         ; 21: Reserved
                DCD     0                         ; 22: Reserved
                DCD     0                         ; 23: Reserved
                DCD     0                         ; 24: Reserved
                DCD     0                         ; 25: Reserved
                DCD     0                         ; 26: Reserved
                DCD     0                         ; 27: Reserved
                DCD     CPU_CLCD_IRQHandler       ; 28: Reserved - CPU FPGA CLCD
                DCD     0                         ; 29: Reserved - CPU FPGA
                DCD     UART3_IRQHandler          ; 30: UART3    - CPU FPGA
                DCD     SPI_IRQHandler            ; 31: SPI Touchscreen - CPU FPGA


                AREA    |.text|, CODE, READONLY



; Dummy Exception Handlers (infinite loops which can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler         [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler          [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler        [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
                BX	LR
				BX 	LR
				BX	LR
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler          [WEAK]
                B       .
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler            [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler           [WEAK]
                B       .
                ENDP

Default_Handler PROC

                EXPORT  WDT_IRQHandler            [WEAK]
                EXPORT  RTC_IRQHandler            [WEAK]
                EXPORT  TIM0_IRQHandler           [WEAK]
                EXPORT  TIM2_IRQHandler           [WEAK]
                EXPORT  MCIA_IRQHandler           [WEAK]
                EXPORT  MCIB_IRQHandler           [WEAK]
                EXPORT  UART0_IRQHandler          [WEAK]
                EXPORT  UART1_IRQHandler          [WEAK]
                EXPORT  UART2_IRQHandler          [WEAK]
                EXPORT  UART3_IRQHandler          [WEAK]
                EXPORT  UART4_IRQHandler          [WEAK]
                EXPORT  AACI_IRQHandler           [WEAK]
                EXPORT  CLCD_IRQHandler           [WEAK]
                EXPORT  ENET_IRQHandler           [WEAK]
                EXPORT  USBDC_IRQHandler          [WEAK]
                EXPORT  USBHC_IRQHandler          [WEAK]
                EXPORT  CHLCD_IRQHandler          [WEAK]
                EXPORT  FLEXRAY_IRQHandler        [WEAK]
                EXPORT  CAN_IRQHandler            [WEAK]
                EXPORT  LIN_IRQHandler            [WEAK]
                EXPORT  I2C_IRQHandler            [WEAK]
				EXPORT  CPU_CLCD_IRQHandler       [WEAK]
                EXPORT  SPI_IRQHandler            [WEAK]

WDT_IRQHandler
RTC_IRQHandler
TIM0_IRQHandler
TIM2_IRQHandler
MCIA_IRQHandler
MCIB_IRQHandler
UART0_IRQHandler
UART1_IRQHandler
UART2_IRQHandler
UART3_IRQHandler
UART4_IRQHandler
AACI_IRQHandler
CLCD_IRQHandler
ENET_IRQHandler
USBDC_IRQHandler
USBHC_IRQHandler
CHLCD_IRQHandler
FLEXRAY_IRQHandler
CAN_IRQHandler
LIN_IRQHandler
I2C_IRQHandler
CPU_CLCD_IRQHandler
SPI_IRQHandler
                B       .

                ENDP


Reset_Handler	
	;test for SBC
	;THUMB
	;LDR R1,=MYDATA
	;CBNZ R6, Label2
	;ITTEE EQ
	;ADDEQ R3,LR
	;LSLEQ R3,#3
	;MOVNE R4,LR
	;LSLNE R4,#3
Label
	MOV R1,LR
	MOV R2,LR
	MOV R6,SP
	;SUB R1,#1
	STMIA R6!,{R1,R2}
	LDMDB R6!,{R3,R4}
	;POP.w {R3,R4}
	;LDMDB SP!,{R3,R4}
	MOV R3,LR
	LDR R2,=DATA
	STR R3,[R2]
	LDR R1,[R2]
	SVC #1
	;B Label2
	MOV R3,#0x08
	MOV R1,#1
	PUSH {R1}
	POP {R5} 
	MOV R2,#2
	MOV R4,R3
	STM R3!,{R1,R2}
	LDM R4!,{R6,R7}
	LDR R5,[R4]
	PUSH {R1,R2}
	POP {R3,R4}
	LSR R1,#5
	UXTB R2,R1
	SUB SP,SP,#8
	;BKPT #234
	;ADR R1, Label
Label2	
	;MOV R5,#0xC
	;CBZ R6, Label
	LDR R1,=__initial_sp
	MOV SP, R1
	PUSH {R1,R2}
	POP {R3,R4}
	LDR R2, [R2, #0]
	STR R2, [SP, #0]
	LDR R3,[SP, #0]
	ADD R1,PC
	BLX R1	
	MOV R2,R1
	MOV R1,R2
	MOV PC,LR
	ADD SP,R1
	MOV R2,#0
	ROR R1,R2
	MVN R2,R1
	BIC R1,R2
	
	MUL R1,R2
	
	ADD R1,R2
	ORR R2,R1
	
	MOV R2,LR
	LSR R2,#1
	MOV R3,R2
	CMN R2,R3
	
	MOV R2,LR
	LSR R2,#1
	MOV R3,LR
	ASR R3,#1
	CMP R2,R3
	
	TST R2,R3
	
	LSL R3,#5
	ROR R3,R2
	
	SUB R3,#2
	SBC R2,R3
	
	MOV R2,#1
	MOV R3,#2
	SBC R2,R3

	;test for ADC
	MOV R2,LR
	MOV R2,#1
	MOV R3,#1
	ADC R2,R3
	
	MOV R2,LR
	MOV R3,#1
	LSR R2,#1
	ADC R2,R3
	
	MOV R2,#1
	LSL R2,#31
	MOV R3,#1
	ADC R2,R3
	
	MOV R2,LR
	MOV R3,#1
	ADC R2,R3
	
	MOV R2,LR
	MOV R3,LR
	ADC R2,R3
	
	MOV R2,#1
	LSL R2,#31
	ADD R2,#1
	MOV R3,R2
	ADC R2,R3
	
	EOR R2,R3
	
	
	SUB R3,#20
	ADD R2,#20
	ADD R4,#0
	CMP R4,#1
	CMP R3,#1
	CMP R3,#0
	CMP R3,#21
	CMP r5,#0
	ASR R1,R2,#1
	LSL R3,R2,#1
	LSR R1,R2,#1
	LSR R2,R2,#1
	MUL R1,R2
	ASR R3,R2,#2
	ADD R1,R1,R2
	BX	R1

	MOV R1,R2
	
	AREA    MYDATA, READWRITE
DATA	
	DCD		0xABABABAB
	END
	
