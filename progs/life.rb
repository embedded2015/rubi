$X = 50
$Y = 40

$map = 0
$data = 0

def init
	map = Array((X+1)*(Y+1))
	data = Array((X+1)*(Y+1))
	for y = 0, y <= Y, y++
		for x = 0, x <= X, x++
			map[y * Y + x] = data[y * Y + x] = 0
		end
	end
	map[25 + Y * 1] = 1
	map[23 + Y * 2] = 1
	map[25 + Y * 2] = 1
	map[13 + Y * 3] = 1
	map[14 + Y * 3] = 1
	map[21 + Y * 3] = 1
	map[22 + Y * 3] = 1
	map[35 + Y * 3] = 1
	map[36 + Y * 3] = 1
	map[12 + Y * 4] = 1
	map[16 + Y * 4] = 1
	map[21 + Y * 4] = 1
	map[22 + Y * 4] = 1
	map[35 + Y * 4] = 1
	map[36 + Y * 4] = 1
	map[1  + Y * 5] = 1
	map[2  + Y * 5] = 1
	map[11 + Y * 5] = 1
	map[17 + Y * 5] = 1
	map[21 + Y * 5] = 1
	map[22 + Y * 5] = 1
	map[1  + Y * 6] = 1
	map[2  + Y * 6] = 1
	map[11 + Y * 6] = 1
	map[15 + Y * 6] = 1
	map[17 + Y * 6] = 1
	map[18 + Y * 6] = 1
	map[23 + Y * 6] = 1
	map[25 + Y * 6] = 1
	map[11 + Y * 7] = 1
	map[17 + Y * 7] = 1
	map[25 + Y * 7] = 1
	map[12 + Y * 8] = 1
	map[16 + Y * 8] = 1
	map[13 + Y * 9] = 1
	map[14 + Y * 9] = 1
end

def show 
	for y = 1, y < Y, y++
		for x = 1, x < X, x++
			if map[y * Y + x] == 0
				output "_"
			else
				output "#"
			end
		end
		puts ""
	end
end

def gen
	for y = 1, y < Y, y++
		for x = 1, x < X, x++
			data[y * Y + x] = map[y * Y + x]
		end
	end
	
	for y = 1, y < Y, y++
		for x = 1, x < X, x++
			if data[y * Y + x] == 0 
				b = data[y * Y + (x-1)] + data[y * Y + (x+1)] + data[(y-1) * Y + x] + data[(y+1) * Y + x]
				b = b + data[(y-1) * Y + (x-1)] + data[(y+1) * Y + (x-1)] + data[(y-1) * Y + (x+1)] + data[(y+1) * Y + (x+1)]
				if b == 3; map[y * Y + x] = 1; end
			elsif map[y * Y + x] == 1
				d = data[y * Y + (x-1)] + data[y * Y + (x+1)] + data[(y-1) * Y + x] + data[(y+1) * Y + x]
				d = d + data[(y-1) * Y + (x-1)] + data[(y+1) * Y + (x-1)] + data[(y-1) * Y + (x+1)] + data[(y+1) * Y + (x+1)]
				if d < 2; map[y * Y + x] = 0; end
				if d > 3; map[y * Y + x] = 0; end
			end
		end
	end
end

# Main

init()

for i = 0, i < 20000, i++
	gen()
	show()
	sleep(100)
end
