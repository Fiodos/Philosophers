// 1 800 200 200 => philo should die
// 5 800 200 200 => no one should die
// 5 800 200 200 7 => no one should die. simulation stops after every philo has eaten 7 times
// 4 410 200 200 => no one should die
// 4 310 200 100 => one philo should die

#include "../lib/philo.h"

int	ft_atoi(const char *str)
{
	int	i;
	int	minus_count;
	int	num;

	i = 0;
	minus_count = 1;
	num = 0;
	while (str[i] == '\r' || str[i] == '\t' || str[i] == ' '
		|| str[i] == '\f' || str[i] == '\v' || str[i] == '\n')
		i++;
	if (str[i] == '-')
	{
		minus_count *= (-1);
		i++;
	}
	else if (str[i] == '+')
		i++;
	while (str[i] <= '9' && str[i] >= '0')
	{
		num = (str[i] - '0') + (num * 10);
		i++;
	}
	return (num * minus_count);
}

int	get_timestamp(struct timeval *start)
{
	int				time;
	struct timeval	curr;

	gettimeofday(&curr, NULL);
	time = curr.tv_sec * 1000000 + curr.tv_usec;
	time -= (start->tv_sec * 1000000 + start->tv_usec);
	return (time / 1000);
}

void	my_sleep(int time, t_philo *philo, int timestamp)
{
	int	end_stamp;

	end_stamp = timestamp + time;
	while (get_timestamp(philo->start) < end_stamp)
		usleep(200);
}

void	custom_lock(pthread_mutex_t *mutex)
{
	if (pthread_mutex_lock(mutex) != 0)
	{
		perror("mutex lock failed");
		exit(errno);
	}
}

void	custom_unlock(pthread_mutex_t *mutex)
{
	if (pthread_mutex_unlock(mutex) != 0)
	{
		perror("mutex unlock failed");
		exit(errno);
	}
}

void	locked_print(t_philo *philo, char mode)
{
	int	timestamp;

	timestamp = get_timestamp(philo->start);
	custom_lock(philo->main);
	if (mode == 'f')
		printf("%d %d has taken fork\n", timestamp, philo->id);
	else if (mode == 'e')
		printf("%d %d is eating\n", timestamp, philo->id);
	else if (mode == 's')
		printf("%d %d is sleeping\n", timestamp, philo->id);
	else if (mode == 't')
		printf("%d %d is thinking\n", timestamp, philo->id);
	custom_unlock(philo->main);
}

void	handle_philos(t_philo *philo)
{
	custom_lock(philo->l_fork);
	locked_print(philo, 'f');
	custom_lock(philo->r_fork);
	locked_print(philo, 'f');
	locked_print(philo, 'e');
	philo->last_meal = get_timestamp(philo->start);
	my_sleep(philo->time_to_eat, philo, get_timestamp(philo->start));
	custom_unlock(philo->l_fork);
	custom_unlock(philo->r_fork);
	locked_print(philo, 's');
	my_sleep(philo->time_to_sleep, philo, get_timestamp(philo->start));
	locked_print(philo, 't');
}

void	*dinner(void *arg)
{
	t_philo	*philo;

	philo = (t_philo *)arg;
	// wait until all philos are created;
	while(!*philo->created)
		usleep(100);
	// if philo is even;
	if (philo->id % 2 == 0)
		usleep(philo->time_to_eat * 1000);
	while(1)
		handle_philos(philo);
	return (NULL);
}

pthread_mutex_t	*create_forks(int amount)
{
	int	i;
	pthread_mutex_t *forks;

	i = 0;
	forks = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * amount);
	while (i < amount)
	{
		pthread_mutex_init(&forks[i], NULL);
		i++;
	}
	return (forks);
}

pthread_t	*create_threads(t_vars *vars, t_philo *philos)
{
	int			i;
	pthread_t	*threads;

	i = 0;
	threads = (pthread_t *)malloc(sizeof(pthread_t) * vars->amount);
	while (i < vars->amount)
	{
		pthread_create(&threads[i], NULL, &dinner, &philos[i]);
		i++;
	}
	vars->created = 1;
	// start global clock;
	gettimeofday(philos[i - 1].start, NULL);
	return (threads);
}

void	waiting_for_threads(t_vars *vars, pthread_t *threads)
{
	int	i;

	i = 0;
	while (i < vars->amount)
	{
		pthread_join(threads[i], NULL);
		i++;
	}
}

void	init_vars(char **argv, t_vars *vars)
{
	if (argv[5])
		vars->must_eat = ft_atoi(argv[5]);
	vars->amount = ft_atoi(argv[1]);
	vars->time_to_die = ft_atoi(argv[2]);
	vars->time_to_eat = ft_atoi(argv[3]);
	vars->time_to_sleep = ft_atoi(argv[4]);
	vars->created = 0;
	vars->start = malloc(sizeof(struct timeval *));
	vars->forks = create_forks(vars->amount);
}

// void	check_iterations(t_vars *vars)
// {
// 	int	control;
// 	int	i;

// 	while (1)
// 	{
// 		control = 0;
// 		i = 0;
// 		while (i < vars->amount)
// 		{
// 			if (vars->iterations[i] >= vars->must_eat)
// 				control++;
// 			i++;
// 		}
// 		if (control == vars->amount)
// 		{
// 			vars->reached_end = 1;
// 			return ;
// 		}
// 	}
// }

void	init_forks(t_philo *philo, t_vars *vars, int index)
{
	philo->forks = vars->forks;
	if (philo->id == 1)
	{
		philo->r_fork = &philo->forks[0];
		philo->l_fork = &philo->forks[philo->amount - 1];
	}
	else
	{
		philo->r_fork = &philo->forks[index];
		philo->l_fork = &philo->forks[index - 1];
	}
}

void	init_times(t_philo *philo, t_vars *vars)
{
	philo->time_to_die = vars->time_to_die;
	philo->time_to_eat = vars->time_to_eat;
	philo->time_to_sleep = vars->time_to_sleep;
	philo->start = vars->start;
}

t_philo	*init_philos(t_vars *vars,  pthread_mutex_t *main)
{
	int		i;
	t_philo	*philo;

	i = 0;
	philo = (t_philo *)malloc(sizeof(t_philo) * vars->amount);
	while (i < vars->amount)
	{
		if (vars->must_eat)
			philo[i].must_eat = vars->amount;
		philo[i].id = i + 1;
		philo[i].amount = vars->amount;
		init_times(&philo[i], vars);
		init_forks(&philo[i], vars, i);
		philo[i].main = main;
		philo[i].last_meal = 0;
		philo[i].iterations = 0;
		philo[i].created = &vars->created;
		i++;
	}
	return (philo);
}

void	mr_calorie_deficit(t_philo *philos, t_vars *vars, pthread_mutex_t main)
{
	int	i;
	int	timestamp;

	while (1)
	{
		i = 0;
		while (i < vars->amount)
		{
			timestamp = get_timestamp(vars->start);
			if (philos[i].last_meal + philos[i].time_to_die < timestamp)
			{
				pthread_mutex_lock(&main);
				printf("%d %d died!\n", timestamp, philos[i].id);
				pthread_mutex_unlock(&main);
				// need to close all the threads here!;
				exit (0);
			}
			i++;
		}
	}
}

int main(int argc, char **argv)
{
	t_vars				vars;
	t_philo				*philos;
	pthread_t			*threads;
	pthread_mutex_t		main_mutex;

	argc = 0;
	pthread_mutex_init(&main_mutex, NULL);
	init_vars(argv, &vars);
	philos = init_philos(&vars, &main_mutex);
	threads = create_threads(&vars, philos);
	mr_calorie_deficit(philos, &vars, main_mutex);
	waiting_for_threads(&vars, threads);
	printf("End of main\n");
	return (0);
}
