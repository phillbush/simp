(defun square x
  (* x x))

(defun average a b
  (/ (+ a b) 2))

(define tolerance
  0.00000001)


#                                                          PRELIMINARIES

# a guess is good enough if it change little from the previous iteration
(defun good-enough? improve guess tolerance
  (< (abs (- (improve guess) guess))
     (abs (* guess tolerance))))

# A number x is called a "fixed point" of a function f if x satisfies
# the equation f(x) = x.  For some functions f we can locate a fixed
# point by beginning with an initial guess and applying f repeatedly
# until the value does not change very much: f(x), f(f(x)), f(f(f(x))),
# and so on.  Using this idea, we can devise a procedure fixed-point
# that takes as inputs a function and an intitial guess and produces
# an approximation to a fixed point of the function.  We can apply the
# function repeatedly until we find two successive values whose
# difference is less than some prescribed tolerance
(defun fixed-point improve guess
  (if (good-enough? improve guess tolerance)
    guess
    (fixed-point improve (improve guess))))

# Notethat x=(x+f(x))/2 is a simple transformation of the equation
# x=f(x); to derive it, add x to both sides of the equation and divide
# by 2.  This transformation, called average damping often aids the
# convergence of fixed-point searches, as we will see below.
(defun average-damp f
  (lambda x
    (average x (f x))))

# Derivative is a function that transforms a function into another
# function.   For instance, the derivative of the function x↦x³ is
# the function x↦3x².  In general, if f is a function and dx is a
# small number, then the derivative of f applied to a value x can be
# approximated by D(f)(x) = (f(x+dx) - f(x)) / dx.
(defun deriv f
  (lambda x
    (/ (- (f (+ x tolerance)) (f x)) tolerance)))

# If f is adifferentiable function, then a root of f (that is, a
# solution to the equation f(x)=0) is a fixed point of the function
# x↦x-f(x)/D(f)(x), where D(f)(x) is the derivative of f evaluated
# at x.  For many functions and for sufficiently good initial guesses
# for x, Newton's method converges very rapidly to a solution of g(x)=0.
(defun newton-transform f
  (lambda x (- x (/ (f x) ((deriv f) x)))))


#                                                  COMPUTING SQUARE ROOT

# .Computing Square Root, Heron's Method.
# The Heron of Alexandria method of computing an approximation to the
# square root of a number says that to compute the square root of some
# number x requires finding a y such as y²=x.  Putting this equation
# into the equivalent form y=x/y, we recognize that we are looking for a
# fixed point of the function y↦x/y.  We could therefore try to compute
# square roots as
#
#   (defun sqrt x (fixed-point (lambda y (/ x y)) 1.0))
#
# Unfortunately, this fixed-point search does not converge.  Consider an
# initial guess y₀.  The next guess is y₁=x/y₀.  This results in an
# infinite loop in which the two guesses y₀ and y₁ repeat over and over,
# oscillating about the answer.  One way to control such oscillations is
# to preventthe guesses from changing so much.  Using the theorem of
# average damping, we can replace the function y↦x/y with (x+x/y)/2,
# that is, with the average damping of y↦x/y.
(defun sqrt-heron x
  (fixed-point
    (average-damp (lambda y (/ x y)))
    1.0))

# .Computing Square Root, Newton's Method.
# To find the square root of x, we can use Newton's method to find a
# root of the function y↦x-y².
(defun sqrt-newton x
  (fixed-point
    (newton-transform (lambda y (- x (square y))))
    1.0))


#                                                                  TESTS

(display "(sqrt-heron 25) =\t")
(display (sqrt-heron 25))
(newline)

(display "(sqrt-newton 25) =\t")
(display (sqrt-newton 25))
(newline)
