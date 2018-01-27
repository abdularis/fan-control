/*
 *
 * 2016, Abdul Aris R
 * Simple fan control (using pwm to control fan speed)
 *
*/

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>	
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct __config
{
	int interval;
	int curr_speed;
	float last_temp;
	int min_temp;
	int min_speed;
	int max_speed;
	int temp_step;
	int speed_step;
	char* fan_path;
	char* cpu_temp_path;

	int no_daemon:1;
};
typedef struct __config Config;

static void set_default_config(Config* c);
static float read_temperature(Config* conf);
static void set_fan_speed(Config* conf);
static void update_fan_speed(Config* conf);
static void create_child_process();
static void read_config_from_cmd_args(Config* conf, int* argc, char*** argv);

int main(int argc, char** argv)
{
	Config* c = malloc(sizeof(Config));
	if (c == NULL) {
		perror("main() -> malloc()");
		exit(1);
	}

	set_default_config(c);
	read_config_from_cmd_args(c, &argc, &argv);
	
	printf("--- simple fan control ---\n");
	printf("interval:\t%d\n", c->interval);
	printf("current speed:\t%d\n", c->curr_speed);
	printf("last temp:\t%f\n", c->last_temp);
	printf("min temp:\t%d\n", c->min_temp);
	printf("min speed:\t%d\n", c->min_speed);
	printf("max speed:\t%d\n", c->max_speed);
	printf("temp step:\t%d\n", c->temp_step);
	printf("speed step:\t%d\n", c->speed_step);
	printf("fan path:\t%s\n", c->fan_path);
	printf("cpu temp path:\t%s\n", c->cpu_temp_path);

	if (!c->no_daemon) {
		create_child_process();
	}
	else {
		printf("No daemon\n");
	}
	
	while(1) {
		update_fan_speed(c);
		sleep(c->interval);
	}

	return 0;
}

/*
 *************************************************************************************
 *************************************************************************************
 *************************************************************************************
*/
static void print_usage()
{
	printf("Usage: fan_control [options] ...\n");
	printf("Options:\n"
					"\t--help \tPrint help/usage & exit\n"
					"\t--interval \tInterval time to update speed\n"
					"\t--min-temp \tMinimum temperature to turn on the fan\n"
					"\t--max-speed \tMaximum fan speed\n"
					"\t--min-speed \tMinimum fan speed\n"
					"\t--temp-step \tTemperature steping\n"
					"\t--speed-step \tSpeed steping\n"
					"\t--fan-path \tPath to pwm fan control\n"
					"\t--cpu-temp-path \tPath to cpu temperature\n"
					"\t--no-daemon \tNo daemon\n"
			"Simple fan control, this program is using pwm to control the fan\n");
	exit(0);
}

static void read_config_from_cmd_args(Config* conf, int* argc, char*** argv)
{
	char** args = *argv;
	if ( 1 == *argc )
		print_usage();
	
	for (int i = 1; i < *argc; i++) {
	
		if ( strcmp(args[i], "--interval" ) == 0 ) {
			conf->interval = atoi(args[++i]);
		}
		else if ( strcmp(args[i], "--min-temp" ) == 0 ) {
			conf->min_temp = atoi(args[++i]);
		}
		else if ( strcmp(args[i], "--max-speed" ) == 0 ) {
			conf->max_speed = atoi(args[++i]);
		}
		else if ( strcmp(args[i], "--min-speed" ) == 0 ) {
			conf->min_speed = atoi(args[++i]);
		}
		else if ( strcmp(args[i], "--temp-step" ) == 0 ) {
			conf->temp_step = atoi(args[++i]);
		}
		else if ( strcmp(args[i], "--speed-step" ) == 0 ) {
			conf->speed_step = atoi(args[++i]);
		}
		else if ( strcmp(args[i], "--fan-path" ) == 0 ) {
			conf->fan_path = args[++i];
		}
		else if ( strcmp(args[i], "--cpu-temp-path" ) == 0 ) {
			conf->cpu_temp_path = args[++i];
		}
		else if ( strcmp(args[i], "--no-daemon" ) == 0 ) {
			conf->no_daemon = 1;
		}
		else if ( strcmp(args[i], "--help") == 0) {
			print_usage();
		}
		else {
			printf("[!] There is no %s option\n", args[i]);
			print_usage();
		}
	}


	if ( conf->interval <= 0 || conf->interval > 35 ) 
		conf->interval = 5;

	if ( conf->min_temp <= 0 ) 
		conf->min_temp = 30;

	if ( conf->min_speed >= conf->max_speed ) {
		conf->min_speed -= 30;
		conf->max_speed += 30;
	}

	if ( conf->min_speed <= 0 || conf->min_speed > 255)
		conf->min_speed = 10;

	if ( conf->max_speed <= 0 || conf->max_speed > 255)
		conf->max_speed = 80;

	if ( conf->temp_step <= 0 || conf->temp_step > 10)
		conf->temp_step = 2;

	if ( conf->speed_step <= 0 || conf->speed_step > 50 ) 
		conf->speed_step = 6;
}

static void set_default_config(Config* c)
{
	c->interval = 12;
	c->last_temp = 0.0f;
	c->curr_speed = 0;
	c->min_temp = 35;
	c->min_speed = 10;
	c->max_speed = 50;
	c->temp_step = 2;
	c->speed_step = 6;
	c->fan_path = "";
	c->cpu_temp_path = "";
	c->no_daemon = 0;
}

static float read_temperature(Config* conf)
{
	// int fd = open(conf->cpu_temp_path, O_RDONLY);
	// if (fd < 0) {
	// 	perror("read_temperature() -> open()");
	// 	exit(1);
	// }

	FILE* file = fopen(conf->cpu_temp_path, "r");
	if (file == NULL) {
		perror("read_temperature() -> fopen()");
		exit(1);
	}
	
	float ret_temp = 0.0f;
	char buff[6];
	
	// read(fd, buff, 6);
	fread(buff, 1, 6, file);

	ret_temp = atoi(buff) / 1000.0f;
	
	fclose(file);
	// close(fd);
	return ret_temp;
}

static void set_fan_speed(Config* conf)
{
	FILE* file = fopen(conf->fan_path, "w");
	if (file == NULL) {
		perror("set_fan_speed() -> fopen()");
		exit(1);
	}

	fprintf(file, "%d", conf->curr_speed);
	fclose(file);
}

static void update_fan_speed(Config* conf)
{	
	float curr_temp = read_temperature(conf);
	printf("curr temp: %f\n", curr_temp);
	
	if (curr_temp == conf->last_temp) {
		return;
	}
	
	if (curr_temp >= conf->min_temp) {
		
		int step = (int)(curr_temp - conf->min_temp) / conf->temp_step;
		
		conf->curr_speed = conf->min_speed;
		conf->curr_speed += step * conf->speed_step;
		
		if (conf->curr_speed > conf->max_speed) {
			conf->curr_speed = conf->max_speed;
		}
		
	}
	else {
		conf->curr_speed = 0;
	}
	
	conf->last_temp = curr_temp;
	set_fan_speed(conf);
}

static void create_child_process()
{
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork()");
		exit(1);
	}

	if (pid > 0) {
		exit(0);
	}

	umask(0);

	pid_t sid = setsid();
	if (sid < 0) {
		perror("setsid()");
		exit(1);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}
