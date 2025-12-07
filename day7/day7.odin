package main

import "core:bytes"
import "core:fmt"
import vmem "core:mem/virtual"
import os "core:os/os2"

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

	stride := bytes.index_byte(data, '\n') + 1
	if stride <= 0 {
		fmt.eprintfln("Bad data: no newline in input file")
		os.exit(1)
	}
	when ODIN_DEBUG do fmt.println("size =", stride)


	start_pos := bytes.index_byte(data, 'S')
	if start_pos == -1 {
		fmt.eprintfln("Bad data: no start position found")
		os.exit(1)
	}
	when ODIN_DEBUG do fmt.println("start_pos =", start_pos)

	total_splits: uint
	rays := make([]uint, len(data))
	rays[start_pos] = 1
	for i in (stride + start_pos) ..< len(data) {
		if data[i] == '\n' do continue
		splits := rays[i - stride]
		if splits == 0 do continue
		switch data[i] {
		case '.', '|':
			data[i] = '|'
			rays[i] += splits
		case '^':
			{
				total_splits += 1
				if (i - 1) % stride != stride {
					data[i - 1] = '|'
					rays[i - 1] += splits
				}
				if (i + 1) % stride != 0 {
					data[i + 1] = '|'
					rays[i + 1] += splits
				}
			}
		}
	}

	when ODIN_DEBUG {
		for i in 0 ..< len(data) {
			splits := rays[i]
			if data[i] == '^' {
				fmt.print("-xx-")
				continue
			} else if data[i] == '\n' {
				fmt.println()
				continue
			} else if splits > 0 {
				fmt.printf("-%2d-", splits)
			} else {
				fmt.print("----")
			}
		}

		fmt.println(string(data))
	}

	fmt.println("total_splits =", total_splits)
	sum: uint
	for splits in rays[len(rays) - stride:] {
		sum += splits
	}
	fmt.println("sum =", sum)
}
