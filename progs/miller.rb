def modPow(b, p, m)
	res = 1
	while p > 0
		if p % 2 == 1
			res = (res * b) % m
		end
		b = (b * b) % m
		p = p / 2
	end
	res
end

def prime(n)
	if n < 2
		return 0
	elsif n == 2
		return 1
	elsif n % 2 == 0
		return 0
	end

	d = n - 1
	while d % 2 == 0; d = d / 2; end

	for q = 0, q < 30, q++
		a = (rand() % (n - 2)) + 1
		t = d
		y = modPow(a, t, n)
		while t != n - 1 & y != 1 & y != n - 1
			y = (y * y) % n
			t = t * 2
		end
		if y != n - 1 & t % 2 == 0
			return 0
		end
	end
	1
end

isp = 0
while isp < 100
	r = rand() % 65536
	if prime(r)
		puts r, " is prime"
		isp++
	end
end
