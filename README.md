# Simp: A Simplistic Programming Language

![A wizard kneeling in front of a down pointing witch](./simp.png)

Simp is my attempt to design and implement a minimalist lisp language.
Garbage collection and tail-call optimization are implemented.
See `./simp.1` for documentation.

TODO:
* Drop the need to apply every procedure?
* Add quasi-quote and unquote.
* Display the line number and file of error.
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

(apply display
  (apply ackermann 1 6))
```

The first expression defines the ackermann function; the second
expression computes the ackermann function applied to 1 and 6.

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

* There are no pairs.  S-expressions are not implemented as singly
  linked lists of cons cells, but as tuples/vectors.
