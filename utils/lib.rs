#[macro_export]
macro_rules! debug_print {
    ($($arg:tt)*) => (#[cfg(debug_assertions)] print!($($arg)*));
}

#[macro_export]
macro_rules! debug_println {
    ($($arg:tt)*) => (#[cfg(debug_assertions)] println!($($arg)*));
}

#[macro_export]
macro_rules! debug_block {
    ($body:block) => {{
        if cfg!(debug_assertions) {
            $body
        }
    }};
}

#[macro_export]
macro_rules! perf {
    ($label:expr, $body:block) => {{
        if cfg!(feature = "perf") {
            use std::time::Instant;

            let __label: &str = $label;
            let __start = Instant::now();
            let __result = { $body };
            let __elapsed = __start.elapsed();
            eprintln!("[PERF] {__label}: {__elapsed:?}");
            __result
        } else {
            $body
        }
    }};
}
