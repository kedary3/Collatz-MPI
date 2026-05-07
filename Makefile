CC      ?= gcc
MPICC   ?= mpicc
CFLAGS  ?= -O3 -std=c11 -Wall -Wextra -pedantic
BUILD   := build
SRC     := src

.PHONY: all clean

all: $(BUILD)/collatz_serial $(BUILD)/collatz_mpi

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/collatz_serial: $(SRC)/collatz_serial.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/collatz_mpi: $(SRC)/collatz_mpi.c | $(BUILD)
	$(MPICC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD)
