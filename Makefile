
all:
	gcc -o ft ft.c ft_common.c ft_common.h ft_udp.c ft_udp.h

	gcc -o ftd ft_daemon.c ft_common.c ft_common.h ft_udp.c ft_udp.h

clean:
	rm -f  *.o *.swp ft ftd
