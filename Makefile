all:
	gcc Sorter.c -O3 -o sorter

clean:
	rm sorter

run:
	cat movie_metadata.csv | ./sorter > results.csv -c director_name

debug:
	gcc Sorter.c -Wall -Werror -fsanitize=address -g -o sorter