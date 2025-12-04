package main

import "core:fmt"
import vmem "core:mem/virtual"
import os "core:os/os2"

Map :: struct {
	value: []byte,
	size:  int,
}

main :: proc() {
	if len(os.args) < 2 {
		fmt.eprintfln("Usage: %v <file_input>", os.args[0])
		os.exit(1)
	}

	arena: vmem.Arena
	if err := vmem.arena_init_static(&arena); err != nil {
		fmt.eprintfln("Allocation Error %v", err)
		os.exit(1)
	}
	defer vmem.arena_destroy(&arena)
	context.allocator = vmem.arena_allocator(&arena)

	data, err := os.read_entire_file(os.args[1], context.allocator)
	if err != nil {
		fmt.eprintfln("Error opening file: %v", os.error_string(err))
		os.exit(1)
	}

	m := new(Map)
	m.value = data

	for c in m.value {
		if c == '\r' {
			fmt.eprintln("Remove CR from data")
			os.exit(1)
		}
		if c == '\n' {
			break
		}
		m.size += 1
	}

	when ODIN_DEBUG do fmt.println("m.size =", m.size)

	counter := 0
	when ODIN_DEBUG do fmt.println("========")
	for y in 0 ..< m.size {
		for x in 0 ..< m.size {
			i := index(m.size, x, y)
			if m.value[i] == '@' do counter += traverse_map_rec(m, x, y)
		}
	}
	when ODIN_DEBUG {
		fmt.println(string(m.value))
	}
	fmt.println("counter =", counter)
	vmem.arena_destroy(&arena)
}

index :: #force_inline proc(size, x, y: int) -> int {
	return y * (size + 1) + x
}

traverse_map_around :: proc(m: ^Map, x, y: int) -> int {
	counter := 0
	for cy in max(y - 1, 0) ..= min(y + 1, m.size - 1) {
		for cx in max(x - 1, 0) ..= min(x + 1, m.size - 1) {
			if m.value[index(m.size, cx, cy)] == '@' {
				counter += traverse_map_rec(m, cx, cy)
			}
		}
	}
	return counter
}

traverse_map_rec :: proc(m: ^Map, x, y: int) -> int {
	counter := 0
	for cy in max(y - 1, 0) ..= min(y + 1, m.size - 1) {
		for cx in max(x - 1, 0) ..= min(x + 1, m.size - 1) {
			if cy == y && cx == x {
				continue
			}
			if m.value[index(m.size, cx, cy)] == '@' {
				counter += 1
				if counter > 3 {
					return 0
				}
			}
		}
	}

	m.value[index(m.size, x, y)] = 'x'
	return 1 + traverse_map_around(m, x, y)
}
