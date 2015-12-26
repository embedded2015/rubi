Rubi
====
Rubi is a high-level, high-performance script programming language, with syntax that is familiar to users of Ruby. Rubi is implemented via just-in-time compilation.

Syntax
======
Anyone who is familiar to Ruby programming language should learn Rubi quickly.
Sample programs can be found in the directory `progs`.

- Syntax
```ruby
# Statement => write in the function.

# output the string and number
# puts is output and \n
# output is output only.
puts "Hello world!!" # => Hello world!!\n
puts "number = ", 65536 # => number = 65536\n
output "not new line" # => not new line

# declaration is not required
# support only integer
i = 10
i = i + 43
i = i % 5
i = i - 32
sum = i * (12 / 5)
12 / 5 # => 2

# loop and if
for i = 1, i < 100, i = i + 1
    if i % 15 == 0
        puts "fizzbuzz"
    elsif i % 5 == 0
        puts "buzz"
    elsif i % 3 == 0
        puts "fizz"
    else
        puts i
    end
end

# create function
# return is not support...
def sum(n)
    sm = 0
    for i = 1, i <= n, i = i + 1
        sm = sm + i
    end
    sm
end
```

- Implement Fibonacci in Rubi:

```ruby
def fib(n)
    if n < 2
        n
    else
        fib(n - 1) + fib(n - 2)
    end
end

puts fib(39)
```

Licensing
=========

Rubi is freely redistributable under the two-clause BSD License.
Use of this source code is governed by a BSD-style license that can be found
in the `LICENSE` file.

Portability
===========
The compiler and JIT is highly dependant on the Intel x86 Instruction Set Architecture (ISA) and Linux style calling convention.


Quick Start
===========
* Install development packages for Ubuntu Linux Ubuntu 14.04 LTS:
```
sudo apt-get install gcc-multilib libc6-dev-i386
```
* Build and run
```
$ make
$ ./rubi [filename]
```
