(defun ackermann x y
  (do
    (display "compute")
    (newline)
    (if (= y 0) 0
        (= x 0) (* 2 y)
        (= y 1) 2
        (ackermann
          (- x 1)
          (ackermann x (- y 1))))))

(define ack-of-one
  (ackermann 1))

(display
  (ack-of-one 6))
