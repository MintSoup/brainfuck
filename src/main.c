#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint8_t *code;
uint8_t *codeptr;

typedef enum {
	OP_QUIT,
	OP_INC,
	OP_DEC,
	OP_ATM,
	OP_RIGHT,
	OP_LEFT,
	OP_SLIDE,
	OP_OUT,
	OP_JZ,
	OP_JMP,
	OP_ZERO,
	OP_SET
} OPCODE;

uint8_t *bytecode;
uint32_t bytecode_count = 0;
uint32_t bytecode_capacity = 0;

uint8_t memcells[9000];
uint32_t pointer = 0;

uint32_t emitCode(OPCODE code) {
	if (bytecode_capacity < ++bytecode_count) {
		bytecode_capacity = bytecode_capacity == 0 ? 2 : bytecode_capacity * 2;
		bytecode = realloc(bytecode, bytecode_capacity);
	}
	bytecode[bytecode_count - 1] = code;
	return bytecode_count - 1;
}

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

uint32_t READ32(uint8_t *i) {
	return *(uint32_t *) i;
}

void disassemble() {
	int i = 0;
	while (i < bytecode_count) {
		printf("%.6d ", i);
		switch (bytecode[i]) {
			case OP_INC:
				puts("inc");
				i++;
				break;
			case OP_DEC:
				puts("dec");
				i++;
				break;
			case OP_LEFT:
				puts("left");
				i++;
				break;
			case OP_RIGHT:
				puts("right");
				i++;
				break;
			case OP_OUT:
				puts("out");
				i++;
				break;
			case OP_ATM: {
				i++;
				uint8_t top = bytecode[i];
				i++;
				uint8_t bottom = bytecode[i];
				int16_t value = (int16_t) (bottom | (top << 8));
				i++;
				printf("atm %d\n", value);
				break;
			}
			case OP_SLIDE: {
				i++;
				uint8_t top = bytecode[i];
				i++;
				uint8_t bottom = bytecode[i];
				int16_t value = (int16_t) (bottom | (top << 8));
				i++;
				printf("slide %d\n", value);
				break;
			}
			case OP_JZ: {
				i++;
				uint32_t to = READ32(bytecode + i);
				i += 4;
				printf("jz %d\n", to);
				break;
			}
			case OP_JMP: {
				i++;
				uint32_t to = READ32(bytecode + i);
				i += 4;
				printf("jmp %d\n", to);
				break;
			}
			case OP_ZERO:
				i++;
				puts("zero");
				break;

			case OP_QUIT:
				i++;
				puts("quit");
				break;

			case OP_SET: {
				i++;
				printf("set %d\n", bytecode[i]);
				i++;
				break;
			}
		}
	}
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
			emitCode(OP_INC);
			break;
		case -1:
			emitCode(OP_DEC);
			break;
		default: {
			uint8_t top = finalChange >> 8;
			uint8_t bottom = finalChange & 0xff;
			emitCode(OP_ATM);
			emitCode(top);
			emitCode(bottom);
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
			emitCode(OP_RIGHT);
			break;
		case -1:
			emitCode(OP_LEFT);
			break;
		default: {
			uint8_t top = finalChange >> 8;
			uint8_t bottom = finalChange & 0xff;
			emitCode(OP_SLIDE);
			emitCode(top);
			emitCode(bottom);
			break;
		}
	}
}

void loop() {

	codeptr++;
	if (*codeptr == '-') {
		if (*(codeptr + 1) == ']') {
			codeptr += 2;
			if (*codeptr == '+' || *codeptr == '-') {
				int16_t set = arithmetic(0);
				if (set == 0) {
					emitCode(OP_ZERO);
					return;
				}
				uint8_t final = 0;
				final += set;
				emitCode(OP_SET);
				emitCode(final);
				return;
			}
			emitCode(OP_ZERO);
			return;
		}
	}

	uint32_t jumpback = emitCode(OP_JZ);

	emitCode(0);
	emitCode(0);
	emitCode(0);
	emitCode(0);

	compile();
	codeptr++;

	uint32_t jumpto = emitCode(OP_JMP) + 5;

	emitCode(jumpback & 0xff);
	emitCode((jumpback >> 8) & 0xFF);
	emitCode((jumpback >> 16) & 0xFF);
	emitCode(jumpback >> 24);

	bytecode[jumpback + 4] = jumpto >> 24;
	bytecode[jumpback + 3] = (jumpto >> 16) & 0xFF;
	bytecode[jumpback + 2] = (jumpto >> 8) & 0xFF;
	bytecode[jumpback + 1] = jumpto & (0xFF);
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
				emitCode(OP_OUT);
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
	emitCode(OP_QUIT);
}

void run() {
	uint8_t *ip = bytecode;
	while (1) {
		switch (*ip) {
			case OP_OUT:
				printf("%c", memcells[pointer]);
				ip++;
				break;
			case OP_QUIT:
				return;
			case OP_RIGHT:
				ip++;
				pointer++;
				break;
			case OP_LEFT:
				ip++;
				pointer--;
				break;
			case OP_INC:
				ip++;
				memcells[pointer]++;
				break;
			case OP_DEC:
				ip++;
				memcells[pointer]--;
				break;
			case OP_ATM: {
				ip++;
				int16_t add = 0;
				add |= *ip << 8;
				ip++;
				add |= *ip;
				memcells[pointer] += add;
				ip++;
				break;
			}
			case OP_SLIDE: {
				ip++;
				int16_t add = 0;
				add |= *ip << 8;
				ip++;
				add |= *ip;
				pointer += add;
				ip++;
				break;
			}
			case OP_ZERO:
				ip++;
				memcells[pointer] = 0;
				break;
			case OP_SET:
				ip++;
				memcells[pointer] = *ip;
				ip++;
				break;
			case OP_JZ: {
				if (memcells[pointer] != 0) {
					ip += 5;
					break;
				}
				ip++;
				uint32_t address = READ32(ip);
				ip = (bytecode + address);
				break;
			}
			case OP_JMP: {
				ip++;
				uint32_t address = READ32(ip);
				ip = (bytecode + address);
				break;
			}
		}
	}
}

int main(int argc, char *argv[]) {

	if (argc != 2) {
		puts("Usage: bf <filename>");
		return 1;
	}

	read_file(argv[1]);
	compile();

	/* disassemble(); */

	run();

	free(code);
	free(bytecode);
	return 0;
}
