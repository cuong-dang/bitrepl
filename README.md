# bitrepl - Bitwise REPL (Read-Evaluate-Print-Loop)

bitrepl is a REPL / interpreter that aims to help students working with "bit
twiddling hacks." Most C operators are supported (arithmetic and bitwise) as
well as variable assignments. Numbers could be entered in decimal or
hexadecimal.

## Installation
```
$ git clone https://github.com/cuong-dang/bitrepl.git
$ cd bitrepl
$ make
```

## Supported operators
- Unary: `+, -, ~`
- Arithmetic: `+, -, *, /, %`
- Bitwise: `|, ^, &, <<, >>, >>>`
- Assignment: `=, |=, ^=, &=, <<=, >>=, >>>=, +=, -=, *=, /=, %=`
- Comment: `//`

## Quirks
- Unary `+` and `-` are invalid for hexadecimal numbers.
- Left shift `<<` by more than the current bit width returns 0.
- `>>>` is adopted from Java. It shifts right without sign extension.
- Variable names must consist of only letters and have at most 15 characters.
- Variables do not lose their set bits when the bit width is reduced; they
  will then be `overflowed`.

## Option commands
- `set dm [bin|hex|dec]`: Change output display mode to binary, hexadecimal, or
  decimal
- `set bw x`: Set bit width to `x` that must be positive, maximum 32, and
  divisible by 4
- `help`: Display help

## Example session
Counting the number of bits set in `v = 0xab`
```
b> // an example bitrepl session counting the set bits of 0xab
b> 0xab
0000 0000 1010 1011
b> set bw 8
b> v = 0xab
1010 1011
b> v = (v & 0x55) + ((v >> 1) & 0x55)
0101 0110
b> v = (v & 0x33) + ((v >> 2) & 0x33)
0010 0011
b> v = (v & 0x0f) + ((v >> 4) & 0x0f)
0000 0101
b> set dm dec
d> v
5
d>
```

## Useful bit hack resources
- (page) https://graphics.stanford.edu/~seander/bithacks.html
- (book) Hacker's Delight by Henry S. Warren
