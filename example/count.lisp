(define count
  (lambda (n)
    (if
      (= n 0) 0
        (count (- n 1)))))

(display (count 10))
(newline)
(display (count 100000))
(newline)
