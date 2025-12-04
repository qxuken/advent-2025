use std::{
    env, fs,
    io::{self, Read},
};

const N: usize = 12;

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    let Some(file_path) = args.get(1) else {
        eprintln!(
            "Usage: {} <file_input>",
            args.first().map(|s| s.as_str()).unwrap_or("day3")
        );
        return Err(io::Error::other("file_input missing"));
    };
    let mut file = fs::File::open(file_path)?;
    let mut data = String::new();
    file.read_to_string(&mut data)?;
    let value = data
        .lines()
        .map(|l| {
            l.chars()
                .filter_map(|c| c.to_digit(10))
                // NOTE: Could be copies of acc each iteration, who knows
                .fold([0u32; N], |mut acc, c| {
                    for i in 0..N {
                        if i < N - 1 && acc[i] < acc[i + 1] {
                            for j in i..(N - 1) {
                                acc[j] = acc[j + 1];
                            }
                            acc[N - 1] = c;
                            break;
                        } else if i == N - 1 && acc[i] < c {
                            acc[N - 1] = c
                        }
                    }
                    // println!("{acc:?} {c}");
                    acc
                })
                .iter()
                .rev()
                .enumerate()
                .fold(0u64, |acc, (i, v)| {
                    // NOTE: Speed here can be better if buffer will be reversed
                    acc + (*v as u64 * 10u64.pow(i.try_into().unwrap()))
                })
        })
        .collect::<Vec<u64>>();
    // println!("{:?}", value);
    let sum = value.into_iter().sum::<u64>();
    println!("sum = {sum}");
    Ok(())
}
