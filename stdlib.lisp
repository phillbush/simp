(define not (lambda (x) (if x (false) (true))))

(define quote (macro (env x) x))
