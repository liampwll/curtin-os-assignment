.POSIX:
CC      = clang
CFLAGS  = -std=c11 -Wall -g -pthread -fsanitize=thread
LDFLAGS = -pthread -fsanitize=thread
LDLIBS  =

OBJS = build/cpu.o build/error.o build/log.o build/main.o build/task.o \
       build/tsqueue.o

scheduler: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

build/cpu.o: src/cpu.c src/cpu.h src/tsqueue.h src/job.h src/log.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/error.o: src/error.c src/error.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/log.o: src/log.c src/log.h src/job.h src/config.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/main.o: src/main.c src/config.h src/cpu.h src/tsqueue.h src/task.h \
              src/error.h src/job.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/task.o: src/task.c src/task.h src/tsqueue.h src/job.h src/log.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/tsqueue.o: src/tsqueue.c src/tsqueue.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build scheduler
