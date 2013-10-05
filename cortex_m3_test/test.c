#include <stdio.h>

#define ITM_Port8(n)    (*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n)   (*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xE0000000+4*n)))

#define DEMCR           (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          0x01000000

struct __FILE { int handle; /* Add whatever is needed */ };
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f) {
  if (DEMCR & TRCENA) {
    while (ITM_Port32(0) == 0);
    ITM_Port8(0) = ch;
  }
  return(ch);
}

typedef unsigned int u32;

void _print_regs(unsigned int* regs, u32 sp, u32 lr, u32 pc, u32 psr)
{
	int i;
	for(i = 0; i < 12; i++){
		printf("R%d %x\n",i, *(regs+i));
	}
	printf("SP %x\n", sp);
	printf("LR %x\n", lr);
	printf("PC %x\n", pc);
	printf("xPSR %x\n", psr);
}

int add(int a, int b)
{
	return a+b;
}

int main(void)
{
	int c;
	c = add(1,2);
	return 0;
}
