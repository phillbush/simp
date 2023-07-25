(define not (lambda (x) (if x (false) (true))))

(define string-equiv?
  (lambda (s0 s1)
    (= (string-compare s0 s1) 0)))

(define quote (macro (env x) x))
