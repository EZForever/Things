// Inspired from unlimited@tctf2020_finals

/*
Basic principles:
	1. A "number" in iteration arithmetics is a lambda that, when evaluated,
	   iterate (`x = f(x)`) its arguments for certain times.
	2. The identities in iteration arithmetics are `f_0` (`x`) and `f_1` (`f(x)`).
	   All other positive integers are recursively constructed from the identities.
	   Any number that is not in N_* is not defined in iteration arithmetics.
	3. The only well-defined operations in iteration arithmetics are addition
	   and multiplication.
	4. In order to convert iteration arithmetics "numbers" into actual numbers,
	   evaluate the lambda with normal numbers and normal math operation lambdas.
	5. Naming convention: `f_n` is a iteration arithmetics "number" equivalent
	   to `n`, so `f_n(f)(x)` is equivalent to `f^n(x)`.
*/

// f_n(f)(x) = f^n(x)
const f_0 = f => x => x;
const f_1 = f => x => f(x);
//const f_2 = f_inc(f_1); // == f => x => f(f(x))
// ...

// f_inc(f_n) == f_{n+1}
const f_inc = f_n => f => x => f(f_n(f)(x));

// f_add(f_n1)(f_n2) == f_{n1+n2}
const f_add = f_n1 => f_n2 => f => x => f_n1(f)(f_n2(f)(x));

// f_mul(f_n1)(f_n2) == f_{n1*n2}
const f_mul = f_n1 => f_n2 => f_n1(f_add(f_n2))(f_0);

// Use these to evaluate the lambda and get the actual number
const f_plus_1 = x => x + 1;
const f_sub_1 = x => x - 1;

// The iteration arithmetics version of number 9
var f_magic = f_add(f_add(f_1)(f_add(f_1)(f_1)))(f_add(f_add(f_inc(f_1))(f_add(f_1)(f_1)))(f_add(f_1)(f_1)));

// The actual number
var magic = f_magic(f_plus_1)(0);

// Or substract that number from another number
var magic2 = f_magic(f_sub_1)(100);

// Some law in normal maths
const f_mul_3_mod_7 = x => x * 3 % 7;
f_magic(f_mul_3_mod_7)(1) == Math.pow(3, magic) % 7;

