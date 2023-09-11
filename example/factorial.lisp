(define fact
  (lambda x
    (if
      (= x 0) 1
      (* x (fact (- x 1))))))

(define bignum (fact 10))

(display "10! = ")
(display bignum)
(newline)
