# Simp: A Simplistic Programming Language

<p align="center">
  <img src="./simp.png", title="A wizard kneeling in front of a down pointing witch"/>
</p>

Simp is my attempt to design and implement a minimalist lisp language.
Garbage collection and tail-call optimization are implemented.

Documentation:
* The language is documented in `./simp.7`.
* The interpreter is documented in `./simp.1`.
* The C API is documented in `./simp.3`.

TODO:
* Multiple values.
* Arbitrary precision arithmetic.

Example:

```
(define ackermann
  (lambda x y
    (do
      (apply display "compute")
      (apply newline)
      (if (apply = y 0) 0
          (apply = x 0) (apply * 2 y)
          (apply = y 1) 2
          (apply ackermann
            (apply - x 1)
            (apply ackermann x (apply - y 1)))))))

(apply ackermann
  (apply vector-ref (vector 0 1 2 3 4) 1)
  (apply vector-ref (vector 5 6 7 8 9) 3))
```

The first expression defines the ackermann function; the second
expression computes the ackermann function applied to 1 and 8.

The following can be observed:

* There is no `defun` form or a `define` form with embedded lambda.
  Defining a procedure must be done with both `define` and `lambda`
  forms.

* The `lambda` form does not get a list of symbols and then one or more
  expressions as body.  It gets zero or more symbols and a single
  expression as body.  To evaluate to a sequence, use the `do` form
  as body.

* There is no `cond` form. The `if` form is the same as scheme's `cond`
  but with less parentheses.

* To avoid confusion between syntactic forms and procedure applications
  (in `(and foo bar)`, `and` is a procedure? Can I pass it to map?),
  all applications are performed with the `apply` syntactic form.
  You can replace `apply` with `!` as a syntactic-sugar `(apply + 1 2)`
  is the same as `(!+ 1 2)`.
