use advent2025::{debug_println, perf};
use memchr::{memchr, memchr2};
use memmap2::Mmap;
use std::env;
use std::fs::File;
use std::io::{self, Write};

#[inline]
fn is_digit(b: u8) -> bool {
    b.is_ascii_digit()
}

// Parse the first vertical run of digits in a given column (top-to-bottom).
// Returns Some(number) if found, or None if no digits in the column.
#[inline]
fn parse_col_number(data: &[u8], col: usize, row_size: usize) -> Option<u64> {
    let len = data.len();
    let mut idx = col;
    // Skip non-digits in this column
    while idx < len && !is_digit(data[idx]) {
        idx = idx.saturating_add(row_size);
    }
    if idx >= len {
        return None;
    }
    // Find extent of digit run
    let mut end = idx;
    while end < len && is_digit(data[end]) {
        end = end.saturating_add(row_size);
    }
    // Accumulate as wrapping (to match C unsigned overflow semantics)
    let mut n: u64 = 0;
    let mut i = idx;
    while i < end {
        n = n.wrapping_mul(10).wrapping_add((data[i] - b'0') as u64);
        i = i.saturating_add(row_size);
    }
    Some(n)
}

fn main() -> io::Result<()> {
    let mut stdout = io::stdout();
    let mut stderr = io::stderr();

    perf!("entire", {
        // Args
        let args: Vec<String> = env::args().collect();
        if args.len() < 2 {
            let _ = writeln!(stderr, "Usage: {} <file_input>", args[0]);
            std::process::exit(1);
        }

        // Map file into memory (zero-copy)
        let map = perf!("read", {
            let file = File::open(&args[1]).unwrap_or_else(|e| {
                eprintln!("Error opening file: {e}");
                std::process::exit(1);
            });
            unsafe {
                Mmap::map(&file).unwrap_or_else(|e| {
                    eprintln!("Error mmapping file: {e}");
                    std::process::exit(1);
                })
            }
        });
        let data: &[u8] = &map;

        if data.is_empty() {
            let _ = writeln!(stdout, "0");
            return Err(io::Error::other("No data"));
        }

        // Determine row size (bytes per row including newline if present)
        let row_size = perf!("row-size", {
            match memchr(b'\n', data) {
                Some(pos) => pos + 1, // include newline
                None => data.len(),   // single line
            }
        });

        let columns = if row_size > 0 && data.get(row_size - 1) == Some(&b'\n') {
            row_size - 1
        } else {
            row_size
        };

        debug_println!("Read bytes {}", data.len());
        debug_println!("Row size {}", row_size);
        debug_println!("Columns {}", columns);

        // Prepare per-column vectors
        let mut numbers: Vec<Vec<u64>> = Vec::with_capacity(columns);

        // Parse one vertical number per column
        perf!("parse", {
            let mut i = 0;
            for col in 0..columns {
                match parse_col_number(data, col, row_size) {
                    Some(n) => {
                        if i >= numbers.len() {
                            numbers.push(vec![]);
                        }
                        numbers[i].push(n)
                    }
                    None => i += 1,
                }
            }
        });

        // Debug-dump numbers when in debug builds
        #[cfg(debug_assertions)]
        {
            for (i, col) in numbers.iter().enumerate() {
                use advent2025::{debug_print, debug_println};

                debug_print!("col {}: ", i);
                for &v in col {
                    use advent2025::debug_print;

                    debug_print!("{} ", v);
                }
                debug_println!("");
            }
        }

        // Advance to first operator '*' or '+'
        let start_ops = perf!("advance", {
            memchr2(b'*', b'+', data).unwrap_or(data.len())
        });

        // Sum according to operators stream
        let mut sum: u64 = 0;
        perf!("sum", {
            let mut idx = start_ops;
            let mut i = 0usize;
            while idx < data.len() {
                match data[idx] {
                    b'*' => {
                        let mut acc: u64 = 1;
                        if i < numbers.len() {
                            for &n in &numbers[i] {
                                acc = acc.wrapping_mul(n);
                            }
                        }
                        sum = sum.wrapping_add(acc);
                        idx += 1;
                        while idx < data.len() && data[idx] == b' ' {
                            idx += 1;
                        }
                        i += 1;
                    }
                    b'+' => {
                        let mut acc: u64 = 0;
                        if i < numbers.len() {
                            for &n in &numbers[i] {
                                acc = acc.wrapping_add(n);
                            }
                        }
                        sum = sum.wrapping_add(acc);
                        idx += 1;
                        while idx < data.len() && data[idx] == b' ' {
                            idx += 1;
                        }
                        i += 1;
                    }
                    b'\n' | b' ' => {
                        // Skip stray spaces between operators (robustness)
                        idx += 1;
                    }
                    other => {
                        eprintln!("Unexpected data at {}({})", idx, other);
                        std::process::exit(1);
                    }
                }
            }
        });

        let _ = writeln!(stdout, "{sum}");
    });

    Ok(())
}
