# Simp: A Simplistic Programming Language

<p align="center">
  <img src="https://user-images.githubusercontent.com/63266536/181057400-9c4a447b-755b-4c72-806a-4a28f968be2d.png", title="A wizard kneeling in front of a down pointing witch"/>
</p>

Simp is my attempt to design and implement a minimalist lisp language.
For now, it only contains a REPL, whose evaluator only passes what is
read to be print (so it is actually a RPL).

## Documentation

The Simp Programming Language syntax and semantics are specified in
the mandoc manual page at `man/simp.7`.

The Simp interpreter command and its invocation is described in the
mandoc manual page at `man/simp.1`.

The Simp C API is described in the mandoc manual page at `man/simp.3`.

The Simp implementation is described by the code itself.
The code is the manual.
Although the code is sparsely commented, descriptive blocks of
comments are used to annotate nontrivial algorithms, intricate
data structures, and nonobvious globals.  The choice for a given
construct or technique in favor of another is also described in
comments.


## Differences from other LISPs

One of the features of Simp that makes it different from other lisps
is that it does not have pairs as a proper type.  It is the vector the
basic mean of combination of data.

A pair is just a vector with two elements.
A box is just a vector with one element.
And nil is a vector with no element.

Since pairs are vectors, we need a notation to express both data
structures at the same time.

Lists are still expressed with parentheses:

	> (a b c d)
	(a b c d)

Cons pairs (two-element vectors) are expressed with a variation of the
dot notation:

	> (a . b .)
	(a . b .)

The dot notation is expanded to express vectors with more than one
element.  For example, the following is a vector with four elements:

	> (a . b . c . d .)
	(a . b . c . d .)

To make it easy to write vectors, they can be typed in with square
braces. They are still printed using dot notation:

	> [a b c d]
	(a . b . c . d .)

A list can also be typed in with square braces:

	> [a [b [c [d []]]]]
	(a b c d)

Which is the same as this:

	> (a . (b . (c . (d . () . ) . ) . ) . )
	(a b c d)

The logic behind the augmented dot notation is that, when we are between
parentheses, a list is constructed.  When a list is being constructed, a
new vector is created after an object.   So, for instance, when we write
the list `(a b c)`, we create a new vector after `a`, a new vector after
`b`, and a new (empty) vector after `c`.  Thus, `(a b c)` is the same as
`[a [b [c []]]]`:

	> [a [b [c []]]]
	(a b c)

But, when we place a dot after an object inside a list, we inhibit the
creation of a new vector, and instead, we place the next element right
after the current one.  In the list `(a . b c)`, the element `b` occurs
right after `a` in the same vector.  Therefore, `(a . b c)` is the same
as `[a b [c []]]`:

	> [a b [c []]]
	(a . b c)

To inhibit the creation of the last, empty vector, we place a dot after
the last element in a list.  `(a b . c .)` is the same as `[a [b c]]`.
Notice that, in regular LISPs like scheme, this improper list would be
constructed slightly differently, with no dot after the last element, as
in `(a b . c)`.

	> [a [b c]]
	(a b . c .)

Here is a interesting structure, a list built on triples rather than on
pairs:

	> [a b [c d []]]
	(a . b c . d)

Which has this box-and-pointer notation:

	┌───┬───┬───┐    ┌───┬───┬───┐
	│ ╷ │ ╷ │ ╶─┼───>│ ╷ │ ╷ │ ╱ │
	└─┼─┴─┼─┴───┘    └─┼─┴─┼─┴───┘
	  V   V            V   V
	  a   b            c   d

Here's a list with a vector in it:

	> (a (b . c . d .) e)
	(a (b . c . d .) e)

In square bracket notation:

	> [a [[b c d] [e []]]]
	(a (b . c . d .) e)

In mixed parentheses/braces notation:

	> (a [b c d] e)
	(a (b . c . d .) e)

And in box-and-pointer notation:

	┌───┬───┐    ┌───┬───┐    ┌───┬───┐
	│ ╷ │ ╶─┼───>│ ╷ │ ╶─┼───>│ ╷ │ ╱ │
	└─┼─┴───┘    └─┼─┴───┘    └─┼─┴───┘
	  V            │            V
	  a            V            e
	         ┌───┬───┬───┐
	         │ ╷ │ ╷ │ ╷ │
	         └─┼─┴─┼─┴─┼─┘
	           V   V   V
	           b   c   d
