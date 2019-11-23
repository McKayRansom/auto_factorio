
	# the Wno-missing braces is because of the big ararys in test_route.c cause a warning
	# TODO: debug mode with -g
tests:
	gcc route.c test_route.c -o test_route -O3 -Wall -Wno-missing-braces
clean:
	rm testRoute
