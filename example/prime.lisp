(defun gcd a b
  (if (= b 0)
    a
    (gcd b (remainder a b))))

(defun square n
  (* n n))

(defun smallest-divisor n
  (find-divisor n 2))

(defun find-divisor n test-divisor
  (if (> (square test-divisor) n) n
      (divides? test-divisor n)   test-divisor
      (find-divisor n (+ test-divisor 1))))

(defun divides? a b
  (= (remainder b a) 0))

(defun prime? n
  (= n (smallest-divisor n)))
