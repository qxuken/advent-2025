use std::{cmp::Ordering, env, fs, io, io::Read, ops::RangeInclusive};

use advent2025::perf;

type Size = u64;
type Range = RangeInclusive<Size>;

/// Length of an inclusive range, saturating on overflow.
fn range_len(r: &Range) -> Size {
    let (start, end) = (*r.start(), *r.end());
    end.saturating_sub(start).saturating_add(1)
}

/// Merge overlapping ranges; result is sorted, non-overlapping.
fn merge_ranges(mut ranges: Vec<Range>) -> Vec<Range> {
    if ranges.is_empty() {
        return ranges;
    }

    ranges.sort_by_key(|r| *r.start());

    let mut merged: Vec<RangeInclusive<Size>> = Vec::with_capacity(ranges.len());
    for r in ranges {
        let (start, end) = (*r.start(), *r.end());

        if let Some(last) = merged.last_mut() {
            let (ls, le) = (*last.start(), *last.end());

            // Overlap: a.left <= b.right && b.left <= a.right
            if start <= le {
                let new_start = ls.min(start);
                let new_end = le.max(end);
                *last = new_start..=new_end;
            } else {
                merged.push(start..=end);
            }
        } else {
            merged.push(start..=end);
        }
    }

    merged
}

/// Binary search for number in sorted, non-overlapping ranges.
fn is_in_ranges(ranges: &[Range], number: Size) -> bool {
    ranges
        .binary_search_by(|r| {
            let (start, end) = (*r.start(), *r.end());
            if end < number {
                Ordering::Less
            } else if start > number {
                Ordering::Greater
            } else {
                Ordering::Equal
            }
        })
        .is_ok()
}

/// Parse all `u64` values from a string (whitespace-separated).
fn parse_all_numbers(s: &str) -> io::Result<Vec<Size>> {
    s.split_whitespace()
        .map(|tok| {
            tok.parse::<Size>()
                .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))
        })
        .collect()
}

/// Build ranges from a list of numbers: (n0, n1), (n2, n3), ...
fn build_ranges(nums: &[Size]) -> io::Result<Vec<Range>> {
    if !nums.len().is_multiple_of(2) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "odd number of values in range section",
        ));
    }

    Ok(nums
        .chunks_exact(2)
        .map(|chunk| chunk[0]..=chunk[1])
        .collect())
}

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    let Some(file_path) = args.get(1) else {
        eprintln!(
            "Usage: {} <file_input>",
            args.first().map(|s| s.as_str()).unwrap_or("program")
        );
        return Err(io::Error::other("file_input missing"));
    };

    perf!("entire", {
        // Read entire file into a string
        let data = perf!("read", {
            let mut file = fs::File::open(file_path)?;
            let mut buf = String::new();
            file.read_to_string(&mut buf)?;
            buf.replace("-", " ")
        });

        // Split into two sections at the first blank line (\n\n)
        let (ranges_part, numbers_part) = perf!("split_sections", {
            data.split_once("\n\n").ok_or_else(|| {
                io::Error::new(
                    io::ErrorKind::InvalidData,
                    "missing blank line separator between sections",
                )
            })?
        });

        // Parse all numbers in the first section, then build ranges
        let ranges = perf!("parsing_ranges", {
            let nums = parse_all_numbers(ranges_part)?;
            build_ranges(&nums)?
        });

        // Merge overlapping ranges
        let merged_ranges = perf!("optimizing_ranges", { merge_ranges(ranges) });

        // Parse all numbers in the second section
        let numbers = perf!("parsing_numbers", { parse_all_numbers(numbers_part)? });

        // Count how many numbers fall inside any of the merged ranges
        let fresh_counter: Size = perf!("fresh_counter", {
            numbers
                .iter()
                .filter(|&&n| is_in_ranges(&merged_ranges, n))
                .count() as Size
        });
        println!("{fresh_counter}");

        // Total length of all merged ranges, using rayon in parallel
        let range_counter: Size = perf!("range_counter", {
            merged_ranges.iter().map(range_len).sum()
        });
        println!("{range_counter}");

        Ok(())
    })
}
