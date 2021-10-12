CC := gcc
name := bf

src := src
inc := inc
obj := obj

srcf := $(wildcard $(src)/*.c)
objf := $(srcf:$(src)/%.c=$(obj)/%.o)

executable := $(name)

flags := -O3 -Wall

debugflags := -g -Og -Wall -DDebug

all: compiler build

$(obj)/%.o: $(src)/%.c $(obj)
	$(CC) $(flags) -I$(inc) -c -o $@ $<

$(executable): $(objf)
	$(CC) $(objf) -o $(executable)


compiler:
	gcc -O3 -Wall -g ./compiler.c -o ./bfc

build: $(executable)

debug: flags = $(debugflags)
debug: $(executable)

run: $(executable)
	st sh -c './$(executable) mandel.b; pause'

$(obj):
	mkdir -p $(obj)

clean:
	rm -rf $(objf) $(executable)
