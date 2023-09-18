(define flip-flop
  (let state (false)
    (lambda
      (do
        (redefine state (not state))
        state))))

(display (flip-flop))
(newline)

(display (flip-flop))
(newline)

(display (flip-flop))
(newline)

(display (flip-flop))
(newline)
