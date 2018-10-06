
#include "keyboard_map.h"
#define LINES 25
#define COLUMNS_IN_LINE 80
#define SCREENSIZE 2 * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C
extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

unsigned int currentScreenPos = 0;
unsigned int running = 1;
char *vidptr = (char*)0xb8000;

//input--------------------------------------------
char currentLineInput[10];
int currentInPos;
//input end ----------------------------------------
struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];


void interupt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);
	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);
	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void keyboard_init(void)
{
	write_port(0x21 , 0xFD);
}

void print(const char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[currentScreenPos++] = str[i++];
		vidptr[currentScreenPos++] = 0x17;
	}
}

void println(void)
{
	unsigned int line_size = 2 * COLUMNS_IN_LINE;
	currentScreenPos = currentScreenPos + (line_size - currentScreenPos % (line_size));
}

void clearS(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0x17;
	}
	currentScreenPos = 0;
}
void resetArray(char *var) {
    int i = 0;
    while(var[i] != '\0') {
        var[i] = '\0';
        i++;
    }
	currentInPos = 0;
}
void commands(char *var)
{
	if(var[0] == 'c' && var[1] == 'l' && var[2] == '\0')
	{
		clearS();
	}
	else if(var[0] == 'e' && var[1] == 'x' && var[2] == '\0')
	{
		running = 0;
	}
	else
	{
		print("Syntax error");
		println();
	}		
}




void keyboard_handler_main(void)
{
	unsigned char status;
	char keycode;
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;

		if(keycode == ENTER_KEY_CODE) {
			println();
			commands(currentLineInput);
			resetArray(currentLineInput);
			return;
		}

		vidptr[currentScreenPos++] = keyboard_map[(unsigned char) keycode];
		currentLineInput[currentInPos] = keyboard_map[(unsigned char) keycode];
		currentInPos++;
		vidptr[currentScreenPos++] = 0x17;
	}
}







void kmain(void)
{
	clearS();
	println();
	interupt_init();
	keyboard_init();
	while(running);
}