$size = 10

def max(a, b)
	if a < b
		b
	else
		a
	end
end

def dfs(a, sum, k, i)
	if i == size
		sum
	elsif sum + a[i] > k
		dfs(a, sum, k, i + 1)
	else
		max(dfs(a, sum, k, i + 1), dfs(a, sum + a[i], k, i + 1))
	end
end

a = Array(size)

for i = 0, i < size, i = i + 1
	k = rand() % (size * 3)
	for j = 0, j < size, j = j + 1
		a[j] = rand() % size
		output a[j], " "
	end 
	puts " sum = ", k
	if dfs(a, 0, k, 0) == k
		puts "true"
	else
		puts "false"
	end
end
