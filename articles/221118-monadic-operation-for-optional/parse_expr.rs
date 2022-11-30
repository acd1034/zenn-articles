// fn parse_expr(s: &str) -> Option<i32> {
//   let toks: Vec<_> = s.split_whitespace().collect();
//   let n: i32 = toks[0].parse().ok()?;
//   let m: i32 = toks[2].parse().ok()?;
//   match toks[1] {
//     "+" => Some(n + m),
//     "-" => Some(n - m),
//     "*" => Some(n * m),
//     "/" => Some(n / m),
//     _ => None,
//   }
// }

fn parse_expr(s: &str) -> Option<i32> {
  let toks: Vec<_> = s.split_whitespace().collect();
  toks[0].parse().ok().and_then(|n: i32| {
    toks[2].parse().ok().and_then(|m: i32| match toks[1] {
      "+" => Some(n + m),
      "-" => Some(n - m),
      "*" => Some(n * m),
      "/" => Some(n / m),
      _ => None,
    })
  })
}

fn main() {
  assert!(parse_expr("1 + 2") == Some(1 + 2));
  assert!(parse_expr("478 - 234") == Some(478 - 234));
  assert!(parse_expr("15 * 56") == Some(15 * 56));
  assert!(parse_expr("98 / 12") == Some(98 / 12));
}
