
# create the function factory using lambda
#
# defines user function, 'fun' which in turn creates a
# user-space function when supplied with two quoted expressions
# representing the signature of the method
# and a second the body

def {fun}
  (lambda {args body}
          {
            def (head args) (lambda (tail args) body)
          })

def {fun} (lambda {args body} { def (head args) (lambda (tail args) body) })

# currying

fun {unpack f xs} {eval (join (quote f) xs)}
fun {pack f & xs} {f xs}

# aliases

def {uncurry} pack
def {curry} unpack
