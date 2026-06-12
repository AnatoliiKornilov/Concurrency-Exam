all:
  nasm -f elf64 context.asm -o context.o
  g++ coro.cpp context.o
clean:
  rm -rf a.out
  rm -rf context.o