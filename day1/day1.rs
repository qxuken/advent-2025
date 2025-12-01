use std::{
    env,
    fs::File,
    io::{self, BufRead},
};

const ROTOR_SIZE: i32 = 100;
const ROTOR_INITIAL_POS: i32 = 50;

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    let Some(file_path) = args.get(1) else {
        eprintln!(
            "Usage: {} <file_input>",
            args.first().map(|s| s.as_str()).unwrap_or("day1")
        );
        return Err(io::Error::other("file_input missing"));
    };
    let file = File::open(file_path)?;
    let mut rotor = ROTOR_INITIAL_POS;
    let mut zero_clicks = 0;
    let mut zero_stops = 0;
    let reader = io::BufReader::new(file);
    for line_result in reader.lines() {
        let line = line_result?;
        let initial_zero = rotor == 0;
        let action = line.chars().next().unwrap();
        let mut quant: i32 = line[1..].parse().unwrap();
        if action == 'L' {
            quant *= -1;
        }
        rotor += quant;
        zero_clicks += (rotor <= 0 && !initial_zero) as i32 + (rotor / ROTOR_SIZE).abs();
        if rotor == 0 {
            zero_stops += 1;
        }
        rotor = (rotor % ROTOR_SIZE + ROTOR_SIZE) % ROTOR_SIZE;
    }
    println!("zero_clicks = {zero_clicks}");
    println!("zero_stops = {zero_stops}");
    Ok(())
}
