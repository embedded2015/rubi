def file_write
	fp = fopen("text", "w")
	if fp == 0; return 1; end

	fprintf(fp, "I love to do my homework.")

	fclose(fp)
end

def file_read(text:string, max)
	fp = fopen("text", "r")
	if fp == 0; return 1; end

	fgets(text, max, fp)
	fclose(fp)
end

text:string = Array(100)

file_write()
file_read(text, 100)

printf "%s\n", text
