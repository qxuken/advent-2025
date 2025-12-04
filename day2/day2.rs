use rayon::iter::{ParallelBridge, ParallelIterator};
use std::{
    env, fs,
    io::{self, Read},
};

type Size = u64;

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    let Some(file_path) = args.get(1) else {
        eprintln!(
            "Usage: {} <file_input>",
            args.first().map(|s| s.as_str()).unwrap_or("day2")
        );
        return Err(io::Error::other("file_input missing"));
    };
    let mut file = fs::File::open(file_path)?;
    let mut data = String::new();
    file.read_to_string(&mut data)?;
    let values = data
        .lines()
        .flat_map(|l| l.split(','))
        .filter_map(|s| s.split_once('-'))
        .par_bridge()
        .map(|(l, r)| (l.parse::<Size>().unwrap(), r.parse().unwrap()))
        .flat_map(|(l, r)| std::ops::RangeInclusive::new(l, r))
        .filter(|n| contains_patters(n))
        .collect::<Vec<Size>>();
    let sum = values.iter().sum::<Size>();
    println!("sum = {sum}");
    Ok(())
}

#[allow(dead_code)]
fn is_even_palindrome(n: impl ToString) -> bool {
    let s = n.to_string();
    if !s.len().is_multiple_of(2) {
        return false;
    }
    let (left, right) = s.split_at(s.len() / 2);
    left == right
}

#[allow(dead_code)]
fn contains_patters(n: impl ToString) -> bool {
    let s = n.to_string();
    (0..(s.len() / 2)).any(|i| {
        let needle = s.get(0..=i).unwrap();
        s.split(needle).filter(|s| !s.is_empty()).count() == 0
    })
}
