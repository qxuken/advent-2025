#include <stdio.h>
#include <stdlib.h>

#define ROTOR_SIZE 100
#define ROTOR_INITIAL_POS 50

#define LOGGER_ENABLED 0
#define log(...) if (LOGGER_ENABLED) printf(__VA_ARGS__)
#define log_value(val, fmt) if (LOGGER_ENABLED) printf(" %s = "fmt, #val, (val))
#define print_value(val, fmt) printf("%s = "fmt"\n", #val, (val))

int main(int argc, char *argv[]) {
	if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_input>\n", argv[0]);
		return EXIT_FAILURE;
	}
	FILE *f = fopen(argv[1], "r");
	if (f == NULL) {
		perror("Error opening file");
		return EXIT_FAILURE;
	}

	int rotor = ROTOR_INITIAL_POS;
	size_t zero_clicks = 0;
	size_t zero_stops = 0;

	char line[6];
	while (fgets(line, sizeof(line), f)) {
		int initial_zero = rotor == 0;
		char action = line[0];
		int quant = atoi(&line[1]);
		if (action == 'L') quant *= -1;
		rotor += quant;
		// case change + overflows
		zero_clicks += (rotor <= 0 && !initial_zero) + abs(rotor / ROTOR_SIZE);
		rotor = (rotor % ROTOR_SIZE + ROTOR_SIZE) % ROTOR_SIZE;
		if (rotor == 0) zero_stops++;
		log_value(quant, "%03d");
		log_value(rotor, "%02d");
		log_value(zero_stops, "%04zu");
		log_value(zero_clicks, "%04zu");
		log_value(line, "%s");
	}
	print_value(zero_clicks, "%zu");
	print_value(zero_stops, "%zu");
	fclose(f);
	return EXIT_SUCCESS;
}
