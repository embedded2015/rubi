# The calculate of pi
# Reference: http://xn--w6q13e505b.jp/program/spigot.html

def pi
  N = 14*5000
  NM = N - 14 # PI LONG
  a = Array(N)
  d = 0; e = 0; g = 0; h = 0
  f = 10000; cnt = 1
  for c = NM, c, c = c - 14
    d = d % f
    e = d
    for b = c - 1, b > 0, b = b - 1
      g = 2 * b - 1
      if c != NM
        d = d * b + f * a[b]
      else
        d = d * b + f * (f / 5)
      end
      a[b] = d % g
      d = d / g
    end
    printf "%04d", e + d / f

    if cnt % 16 == 0 puts "" end; cnt = cnt + 1
  end
end

puts pi()
