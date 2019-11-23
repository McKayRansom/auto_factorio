
	# the Wno-missing braces is because of the big ararys in test_route.c cause a warning
	# TODO: debug mode with -g
tests:
	gcc route.c test_route.c -o test_route -g -Wall -Wno-missing-braces
calc.c:
	gcc calc.c lib/cJSON-1.7.12/cJSON.c -Wall -o calc -g
clean:
	rm testRoute
	rm calc
