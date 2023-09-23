# Simp: A Simplistic Programming Language

![A wizard kneeling in front of a down pointing witch](./simp.png)

Simp is my attempt to design and implement a minimalist lisp language.
Garbage collection and tail-call optimization are implemented.
See `./simp.pdf` for documentation.

TODO:
* Multiple values?
* Arbitrary precision arithmetic.


## Example

```
(define ackermann
  (lambda x y
    (do
      (display "compute")
      (newline)
      (if (= y 0) 0
          (= x 0) (* 2 y)
          (= y 1) 2
          (ackermann
            (- x 1)
            (ackermann x (- y 1)))))))

(define ack-of-one
  (ackermann 1))

(define val
  (apply ackermann \(1 2)))

(redefine val
  (ack-of-one 6))
```

The first expression defines the ackermann function; the second
expression applies the ackermann function to 1 and 6.

The following can be observed:

* The `quote` syntactic sugar is expressed with a backslash rather than
  with a apostrophe like in scheme.  Apostrophes are used to represent
  literal characters like in C.

* The `lambda` form does not get its formal parameters between
  parentheses in a vector.  It gets a sequence of symbols right
  after the `lambda` keyword.  Simp's `(lambda x y (+ x y))` is
  equivalent to Scheme's `(lambda (x y) (+ x y))`.

* The `lambda` form does not get a sequence of expressions as body.  It
  gets a single expression as body.  To evaluate to a sequence, use the
  `do` form as body.  Simp's `(lambda x (do (display x) (newline)))` is
  equivalent to Scheme's `(lambda (x) (display x) (newline))`.

* There is no `cond` form. The `if` form is the same as scheme's `cond`
  but with less parentheses.

* Procedures (but not macros) can be curried.
  In the example, `ack-of-one`, defined as `(ackermann 1)` is equivalent
  to `(lambda y (ackermann 1 y))`.

* There are no pairs.  S-expressions are not implemented as singly
  linked lists of cons cells, but as tuples/vectors.

* `set!` does not redefine a variable, use `redefine` for that.
  There is a `set!` procedure, but it is a vector mutator.


## Auxiliary syntax considered harmful

I tried to avoid auxiliary syntax as much as I could.  An auxiliary
syntax is a syntactical identifier that can only be used inside another
syntactical form.  Examples in Scheme is the `else` form, which can only
be used inside a `cond`.  In Simp, I tried to minimize their use as much
as I could.  For now, the only auxiliary syntax used in Simp are:

* `unquote`:
  Available inside a quasi-quotation to evaluate an expression.
* `splice`:
  Available inside a quasi-quotation to splice a vector
* `...`:
  Available inside a `lambda` (or `defun` or `defmacro`) to create
  variadic procedures (or macros).
