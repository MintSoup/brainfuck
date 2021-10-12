#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint8_t *code;
uint8_t *codeptr;
uint16_t loopCounter = 0;

void read_file(char *filename) {
	FILE *f = fopen(filename, "rb");
	fseek(f, 0, SEEK_END);
	int size = ftell(f);

	code = (uint8_t *) malloc(size + 1);
	fseek(f, 0, SEEK_SET);
	int read = fread(code, 1, size, f);
	code[read] = 0;
	codeptr = code;
	fclose(f);
}

void compile();

int16_t arithmetic(uint8_t emit) {
	int16_t finalChange = 0;
	while (*codeptr == '-' || *codeptr == '+') {
		if (*codeptr == '-')
			finalChange--;
		if (*codeptr == '+')
			finalChange++;
		codeptr++;
	}

	if (!emit)
		return finalChange;

	switch (finalChange) {
		case 0:
			return 0;
		case 1:
			puts("\tinc byte[rsi]");
			break;
		case -1:
			puts("\tdec byte[rsi]");
			break;
		default: {
			printf(finalChange > 0 ? "\tadd " : "\tsub ");
			printf("byte[rsi], %u\n", abs(finalChange));
			break;
		}
	}
	return finalChange;
}

void slide() {
	int16_t finalChange = 0;
	while (*codeptr == '>' || *codeptr == '<') {
		if (*codeptr == '<')
			finalChange--;
		if (*codeptr == '>')
			finalChange++;
		codeptr++;
	}

	switch (finalChange) {
		case 0:
			return;
		case 1:
			puts("\tinc rsi");
			break;
		case -1:
			puts("\tdec rsi");
			break;
		default: {
			printf("\tlea rsi, [rsi");
			printf(finalChange > 0 ? " + " : " - ");
			printf("%u]\n", abs(finalChange));
			break;
		}
	}
}

void print() {
	puts("\tmov rax, 1");
	puts("\tsyscall");
}

void loop() {

	codeptr++;

	loopCounter++;

	if (*codeptr == '-') {
		if (*(codeptr + 1) == ']') {
			codeptr += 2;
			if (*codeptr == '+' || *codeptr == '-') {
				int16_t set = arithmetic(0);
				if (set == 0) {
					puts("\tmov byte[rsi], 0");
					return;
				}
				uint8_t final = 0;
				final += set;
				printf("\tmov byte[rsi], %u\n", final);
				return;
			}
			puts("\tmov byte[rsi], 0");
			return;
		}
	}

	uint16_t mylp = loopCounter;

	printf("loopa%u:\n", mylp);
	puts("\tmov r11b, byte[rsi]");
	puts("\ttest r11b, r11b");
	printf("\tjz loopb%u\n", mylp);

	compile();

	codeptr++;

	printf("\tjmp loopa%u\n", mylp);
	printf("loopb%u:\n", mylp);
}

void compile() {
	while (*codeptr != 0) {
		switch (*codeptr) {
			case '+':
			case '-':
				arithmetic(1);
				break;
			case '>':
			case '<':
				slide();
				break;
			case '.':
				print();
				codeptr++;
				break;
			case '[':
				loop();
				break;
			case ']':
				return;
			default:
				codeptr++;
				break;
		}
	}
	puts("\tmov rax, 60\n\tmov rdi, 0\n\tsyscall");
}

int main(int argc, char *argv[]) {

	if (argc != 2) {
		puts("Usage: bfc <filename>");
		return 1;
	}

	read_file(argv[1]);

	puts("\
section .data\n\
cells: resb 32678\n\
section .text\n\
global _start\n\
_start:\n\
	mov rsi, cells								    \n\
	mov rdi, 1					; RDI = 1 --> stdout\n\
	mov rdx, 1					; RDX = 1 --> size = 1\n");
	compile();
	free(code);
	return 0;
}
