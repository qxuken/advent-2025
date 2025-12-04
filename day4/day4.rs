use std::{
    env, fs,
    io::{self, Read},
};

use advent2025::{debug_print, debug_println};

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    let Some(file_path) = args.get(1) else {
        eprintln!(
            "Usage: {} <file_input>",
            args.first().map(|s| s.as_str()).unwrap_or("day4")
        );
        return Err(io::Error::other("file_input missing"));
    };
    let mut file = fs::File::open(file_path)?;
    let mut data = String::new();
    file.read_to_string(&mut data)?;
    let mut map = data
        .lines()
        .map(|l| l.chars().collect::<Vec<_>>())
        .collect::<Vec<_>>();
    let mut counter = 0;
    let mut prev_counter = -1;
    while counter != prev_counter {
        debug_println!("========");
        prev_counter = counter;
        for y in 0..map.len() {
            for x in 0..map[y].len() {
                if map[y][x] == '@' && less_than_4_arount(&map, x, y) {
                    map[y][x] = '.';
                    counter += 1;
                    debug_print!("x");
                } else {
                    debug_print!("{}", map[y][x]);
                }
            }
            debug_println!();
        }
        debug_println!("counter = {counter}");
    }
    println!("counter = {counter}");
    Ok(())
}

fn less_than_4_arount(map: &[Vec<char>], x: usize, y: usize) -> bool {
    let mut counter = 0;
    for cy in y.checked_sub(1).unwrap_or_default()..=(y + 1).min(map.len() - 1) {
        for cx in x.checked_sub(1).unwrap_or_default()..=(x + 1).min(map[y].len() - 1) {
            if cy == y && cx == x {
                continue;
            }
            if map[cy][cx] == '@' {
                counter += 1;
                if counter > 3 {
                    return false;
                }
            }
        }
    }
    true
}
