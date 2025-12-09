use std::{
    collections::BinaryHeap,
    env, fs,
    io::{self, Read},
    ops::Add,
};

use advent2025::{debug_block, debug_println, perf};
use itertools::Itertools;
use rayon::prelude::*;

#[derive(Debug, Clone, Copy)]
struct Coord {
    x: usize,
    y: usize,
}

impl Coord {
    fn new(x: usize, y: usize) -> Self {
        Coord { x, y }
    }
    fn area(&self, other: &Self) -> usize {
        self.x.abs_diff(other.x).add(1) * self.y.abs_diff(other.y).add(1)
    }
}

#[derive(Debug)]
struct Rect {
    a: Coord,
    b: Coord,
    area: usize,
}

impl Rect {
    fn points(
        &self,
    ) -> (
        std::ops::RangeInclusive<usize>,
        std::ops::RangeInclusive<usize>,
    ) {
        let max_x = self.a.x.max(self.b.x);
        let min_x = self.a.x.min(self.b.x);
        let max_y = self.a.y.max(self.b.y);
        let min_y = self.a.y.min(self.b.y);
        (min_x..=max_x, min_y..=max_y)
    }
}

impl From<(&Coord, &Coord)> for Rect {
    fn from(value: (&Coord, &Coord)) -> Self {
        Self {
            a: *value.0,
            b: *value.1,
            area: value.0.area(value.1),
        }
    }
}

impl PartialOrd for Rect {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl PartialEq for Rect {
    fn eq(&self, other: &Self) -> bool {
        self.area.eq(&other.area)
    }
}

impl Ord for Rect {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.area.cmp(&other.area)
    }
}

impl Eq for Rect {}

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

        let coords: Vec<Coord> = perf!("parse", {
            data.lines()
                .map(|line| -> io::Result<Coord> {
                    let (x, y) = line
                        .split_once(',')
                        .ok_or(io::Error::new(io::ErrorKind::InvalidData, ""))?;
                    Ok(Coord::new(
                        x.parse::<usize>().map_err(|err| {
                            io::Error::new(io::ErrorKind::InvalidData, err.to_string())
                        })?,
                        y.parse::<usize>().map_err(|err| {
                            io::Error::new(io::ErrorKind::InvalidData, err.to_string())
                        })?,
                    ))
                })
                .collect::<io::Result<Vec<Coord>>>()?
        });
        debug_println!("{coords:?}");

        let top = perf!("solve_part1", { solve_part1(&coords) });
        println!("part1 = {top:?}");

        let top = perf!("solve_part2", { solve_part2(&coords) });
        println!("part2 = {top:?}");
    });

    Ok(())
}

fn solve_part1(coords: &[Coord]) -> Rect {
    let mut squares: BinaryHeap<Rect> = perf!("squares", {
        coords
            .iter()
            .tuple_combinations::<(&Coord, &Coord)>()
            .map_into::<Rect>()
            .collect()
    });

    debug_println!("{squares:?}");
    squares.pop().expect("Should be at least one square")
}

fn solve_part2(coords: &[Coord]) -> Rect {
    let (mut max_x, mut max_y) = perf!("map_dimensions", {
        coords
            .iter()
            .fold((1, 1), |(x, y), c| (x.max(c.x), y.max(c.y)))
    });
    max_x += 3;
    max_y += 3;
    debug_println!("dimensions = ({max_x}, {max_y})");
    let mut map = vec![false; max_y * max_x];
    perf!("map_border", {
        for (c1, c2) in coords.iter().circular_tuple_windows() {
            for y in c1.y.min(c2.y)..=c1.y.max(c2.y) {
                for x in c1.x.min(c2.x)..=c1.x.max(c2.x) {
                    map[index_2d(max_x, x, y)] = true
                }
            }
        }
    });
    debug_block!({
        for y in 0..max_y {
            for x in 0..max_x {
                print!("{}", map[index_2d(max_x, x, y)] as isize);
            }
            println!();
        }
    });

    perf!("map_fill", {
        enum State {
            Unknown,
            KnownInside,
            KnownOutside,
        }
        let mut state = State::Unknown;
        for y in 0..max_y {
            for x in 0..max_x {
                let i = index_2d(max_x, x, y);
                if map[i] {
                    state = State::Unknown;
                    continue;
                }
                match state {
                    State::KnownOutside => {}
                    State::KnownInside => map[i] = true,
                    State::Unknown => {
                        state = State::KnownOutside;
                        let mut has_top_border = false;
                        for cy in 0..y {
                            if map[index_2d(max_x, x, cy)] {
                                has_top_border = true;
                                break;
                            }
                        }
                        if !has_top_border {
                            continue;
                        }
                        let mut has_bottom_border = false;
                        for cy in y + 1..max_y {
                            if map[index_2d(max_x, x, cy)] {
                                has_bottom_border = true;
                                break;
                            }
                        }
                        if !has_bottom_border {
                            continue;
                        }
                        let mut has_left_border = false;
                        for cx in 0..x {
                            if map[index_2d(max_x, cx, y)] {
                                has_left_border = true;
                                break;
                            }
                        }
                        if !has_left_border {
                            continue;
                        }
                        let mut has_right_border = false;
                        for cx in x + 1..max_x {
                            if map[index_2d(max_x, cx, y)] {
                                has_right_border = true;
                                break;
                            }
                        }
                        if !has_right_border {
                            continue;
                        }
                        map[i] = true;
                        state = State::KnownInside;
                    }
                };
            }
        }
    });
    debug_block!({
        for y in 0..max_y {
            for x in 0..max_x {
                print!("{}", map[index_2d(max_x, x, y)] as isize);
            }
            println!();
        }
    });

    // Only if i knew how to calculate polygons
    let mut squares: BinaryHeap<Rect> = perf!("squares", {
        coords
            .iter()
            .tuple_combinations::<(&Coord, &Coord)>()
            .map_into::<Rect>()
            .par_bridge()
            .filter(|r| {
                let (r_x, r_y) = r.points();
                for y in r_y {
                    for x in r_x.clone() {
                        if !map[index_2d(max_x, x, y)] {
                            return false;
                        }
                    }
                }
                true
            })
            .collect()
    });

    debug_println!("{squares:?}");
    squares.pop().expect("Should be at least one square")
}

fn index_2d(size: usize, x: usize, y: usize) -> usize {
    y * size + x
}
