$NULL = 0

# String functions

def strlen(d:string)
  for len = 0, d[len] != 0, len++; end
  len
end

def strcpy(d:string, s:string)
  i = 0
  while s[i] != 0
    d[i] = s[i++]
  end
end

def strcat(d:string, s:string)
  i = 0
  len = strlen(d)
  j = len
  while s[i] != 0
    d[j++] = s[i++]
  end
end

def strcmp(a:string, b:string)
	diff = 0
	a_len = strlen(a)
	for i = 0, i < a_len, i++
		diff = diff + a[i] - b[i]
	end
	diff
end

# utility

def isdigit(n)
  if '0' <= n; if n <= '9'
      return 1
  end end
  0
end

def isalpha(c)
	if 'A' <= c and c <= 'Z'
		return 1
	end
	if 'a' <= c and c <= 'z'
		return 1
	end
	0
end

def atoi(s:string)
  sum = 0; n = 1
  for l = 0, isdigit(s[l]) == 1, l++; n = n * 10; end
  for i = 0, i < l, i++
    n = n / 10
    sum = sum + n * (s[i] - '0')
  end
  sum
end

# Memory

def memset(mem:string, n, byte)
	for i = 0, i < byte, i++
		mem[i] = n
	end
end

def memcpy(m1:string, m2:string, byte)
	for i = 0, i < byte, i++
		m1[i] = m2[i]
	end
end

# Math

def pow(a, b)
	c = a; b--
	while b-- > 0
		a = a * c
	end
	a
end

def abs(a)
	if a < 0; 0 - a else a end
end

def fact(n)
	for ret = n--, n > 0, n--
		ret = ret * n
	end
	ret
end

# Secure

def SecureRandomString(str:string, len)
	stdin = fopen("/dev/urandom", "rb")
	bytes = 128
	data:string = Array(bytes)
	fgets(data, bytes, stdin)
	chars = 0
	for i = 0, i < bytes and chars < len, i++
		if isalpha(data[i]) or isdigit(data[i])
			str[chars++] = data[i]
		else
			fgets(data, bytes, stdin)
		end
	end
	str[chars] = 0
end

# I/O

def input(str:string)
	f = fopen("/dev/stdin", "w+")
	fgets(str, 100, f)
	fclose(f)
	str
end

# Main 

buf:string = Array(32)

for i = 0, i < 8, i++
	SecureRandomString(buf, 16)
	printf "%s%c", buf, 10
end
