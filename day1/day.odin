package main

import "core:bufio"
import "core:fmt"
import vmem "core:mem/virtual"
import os "core:os/os2"
import "core:strconv"

ROTOR_SIZE :: 100
ROTOR_INITIAL_POSITION :: 50

main :: proc() {
	if len(os.args) < 2 {
		fmt.eprintfln("Usage: %v <file_input>", os.args[0])
	}

	arena: vmem.Arena
	mem_err := vmem.arena_init_growing(&arena)
	if mem_err != .None || mem_err != nil {
		fmt.eprintfln("Allocation Error %v", mem_err)
		os.exit(1)
	}
	defer vmem.arena_destroy(&arena)
	context.allocator = vmem.arena_allocator(&arena)

	file, io_err := os.open(os.args[1])
	if io_err != nil {
		fmt.eprintfln("Error opening file: %v", os.error_string(io_err))
		os.exit(1)
	}
	defer os.close(file)

	rotor := ROTOR_INITIAL_POSITION
	zero_clicks := 0
	zero_stops := 0

	r: bufio.Reader
	buf: [4096]byte
	bufio.reader_init_with_buf(&r, os.to_stream(file), buf[:])
	defer bufio.reader_destroy(&r)

	for {
		line, err := bufio.reader_read_slice(&r, '\n')
		if err != nil {
			break
		}
		initial_zero := rotor == 0
		quant, _ := strconv.parse_int(string(line[1:]), 10)
		if line[0] == 'L' do quant *= -1
		rotor += quant
		zero_clicks = zero_clicks + int(rotor <= 0 && !initial_zero) + abs(rotor / ROTOR_SIZE)
		rotor = (rotor % ROTOR_SIZE + ROTOR_SIZE) % ROTOR_SIZE
		if rotor == 0 do zero_stops = zero_stops + 1
		when ODIN_DEBUG {
			fmt.print(" quant =", quant)
			fmt.print(" rotor =", rotor)
			fmt.print(" zero_stops =", zero_stops)
			fmt.print(" zero_clicks =", zero_clicks)
			fmt.print(" line =", line)
		}
	}
	fmt.println("zero_clicks = ", zero_clicks)
	fmt.println("zero_stops = ", zero_stops)
}
