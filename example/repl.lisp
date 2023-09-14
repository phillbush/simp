(define env (environment))

(define repl
  (lambda
    (let input (read)
      (if (and (true? input) (not (eof? input)))
        (do
          (write (eval input env))
          (newline)
          (repl))))))

(repl)

# should that be less parentized like this?
# (define (repl)
#   (write (eval (read)))
#   (repl))
