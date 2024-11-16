#include "server.h"
#include "server_game.h"
#define TICKRATE 64.0

static i32
server_init_tcp(server_t* server)
{
	i32 ret;

	ret = ssp_tcp_server(&server->tcp_sock, SSP_IPv4, server->port);

	return ret;
}

static void
server_set_tickrate(server_t* server, f64 tickrate)
{
	server->tickrate = tickrate;
	server->interval = 1.0 / tickrate;
	server->interval_ns = (1.0 / tickrate) * 1e9;
}

static i32
server_init_udp(server_t* server)
{
	if ((server->udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket UDP");
		return -1;
	}

	server->tcp_sock.addr.sockaddr.in.sin_port = htons(server->udp_port);
	if (bind(server->udp_fd, (struct sockaddr*)&server->tcp_sock.addr.sockaddr, server->tcp_sock.addr.addr_len) == -1)
	{
		perror("bind UDP");
		return -1;
	}

	return 0;
}

static i32 
server_init_timerfd(server_t* server)
{
	struct itimerspec timer;

	server->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
	if (server->timerfd == -1)
	{
		perror("timerfd_create");
		return -1;
	}

	timer.it_value.tv_sec = server->routine_time;
	timer.it_value.tv_nsec = 0;
	timer.it_interval = timer.it_value;

	if (timerfd_settime(server->timerfd, 0, &timer, NULL) == -1)
		perror("timerfd_settime");

	server_add_event(server, server->timerfd, NULL, server_timerfd_timeout, NULL);

	return 0;
}

static i32
server_init_signalfd(server_t* server)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	server->signalfd = signalfd(-1, &mask, 0);
	if (server->signalfd == -1)
	{
		perror("signalfd");
		return -1;
	}

	server_add_event(server, server->signalfd, NULL, signalfd_read, event_signalfd_close);

	return 0;
}

static void
server_print_help(const char* exe_path)
{
	printf(
		"Usage:\n\t%s [options]\n\n"\
		"  -m, --map=FILE\t\t.cgmap file path.\n"
		"  -p, --port=PORT\t\tPort number for TCP and UDP + 1. (Default 49420 TCP, 49421 UDP)\n"
		"  --tcp-port=PORT\t\tTCP Port. (Default 49420)\n"
		"  --udp-port=PORT\t\tUDP Port. (Default 49421)\n"
		"  -t, --tickrate=TICKRATE\tTickrate. (Default 64)\n"
		"  -r, --routine-time=SECONDS\tRoutine checks in seconds. (Default 20s)\n"
		"  -c, --client-timeout=SECONDS\tTime in seconds before a client is disconnected due to inactivity (no packets received). (Default 15s)\n"
		"  -h, --help\t\t\tPrint this message\n\n"
	, exe_path);
}

static i32
server_set_port(u16* dest_port, const char* str)
{
	i32 port = atoi(str);
	if (port <= 0 || port > UINT16_MAX)
	{
		fprintf(stderr, "Invalid port number.\n");
		return -1;
	}

	*dest_port = (u16)port;
	return 0;
}

static i32
server_argv(server_t* server, i32 argc, char* const* argv)
{
	i32 opt;

	struct option long_options[] = {
		{"port",		required_argument,	0, 'p'},
		{"map",			required_argument,	0, 'm'},
		{"udp-port",	required_argument,	0,  0 },
		{"tcp-port",	required_argument,	0,  0 },
		{"tickrate",	required_argument,	0, 't'},
		{"routine-time",	required_argument,	0, 'r'},
		{"client-timeout",	required_argument,	0, 'c'},
		{"help",		no_argument,		0, 'h'},
		{0, 0, 0, 0}
	};
	char* endptr;
	i32 opt_idx;

	while ((opt = getopt_long(argc, argv, "p:m:t:h:r:c:", long_options, &opt_idx)) != -1)
	{
		switch (opt) 
		{
			case 'p':
			{
				if (server_set_port(&server->port, optarg) == -1)
					return -1;
				server->udp_port = server->port + 1;
				break;
			}
			case 'm':
				server->cgmap_path = optarg;
				break;
			case 't':
			{
				i16 tickrate = strtol(optarg, &endptr, 10);
				if (endptr == optarg || *endptr != 0x00 || tickrate >= INT16_MAX || tickrate <= 0)
				{
					fprintf(stderr, "Invalid tickrate.\n");
					return -1;
				}
				server_set_tickrate(server, (f64)tickrate);
				break;
			}
			case 'h':
				server_print_help(argv[0]);
				return -1;
			case 0:
			{
				if (strcmp(long_options[opt_idx].name, "udp-port") == 0)
					server_set_port(&server->udp_port, optarg);
				else if (strcmp(long_options[opt_idx].name, "tcp-port") == 0)
					server_set_port(&server->port, optarg);
				break;
			}
			case 'r':
			{
				i32 time = strtoll(optarg, &endptr, 10);
				if (endptr == optarg || *endptr != 0x00 || time >= INT32_MAX || time <= 0)
				{
					fprintf(stderr, "Invalid routine-time.\n");
					return -1;
				}
				server->routine_time = (f64)time;
				break;
			}
			case 'c':
			{
				i32 time = strtoll(optarg, &endptr, 10);
				if (endptr == optarg || *endptr != 0x00 || time >= INT32_MAX || time <= 0)
				{
					fprintf(stderr, "Invalid client timeout time.\n");
					return -1;
				}
				server->client_timeout_threshold = (f64)time;
				break;
			}
			default:
				return -1;
				break;
		}
	}

	return 0;
}

static void
server_init_netdef(server_t* server)
{
	ssp_segmap_callback_t callbacks[NET_SEGTYPES_LEN] = {0};
	callbacks[NET_TCP_CONNECT] = (ssp_segmap_callback_t)client_tcp_connect;
	callbacks[NET_TCP_WANT_SERVER_STATS] = (ssp_segmap_callback_t)want_server_stats;
	callbacks[NET_UDP_PLAYER_DIR] = (ssp_segmap_callback_t)player_dir;
	callbacks[NET_UDP_PLAYER_CURSOR] = (ssp_segmap_callback_t)player_cursor;
	callbacks[NET_UDP_PLAYER_SHOOT] = (ssp_segmap_callback_t)player_shoot;
	callbacks[NET_UDP_PING] = (ssp_segmap_callback_t)udp_ping;
	callbacks[NET_UDP_PLAYER_PING] = (ssp_segmap_callback_t)player_ping;
	callbacks[NET_TCP_CHAT_MSG] = (ssp_segmap_callback_t)chat_msg;
	callbacks[NET_UDP_PLAYER_GUN_ID] = (ssp_segmap_callback_t)player_gun_id;

	netdef_init(&server->netdef, NULL, callbacks);
	server->netdef.ssp_state.user_data = server;
	server->netdef.ssp_state.verify_session = (ssp_session_verify_callback_t)server_verify_session;
}

static void
server_init_coregame_gun_specs(server_t* server)
{
	const cg_gun_spec_t small_gun_spec = {
		.id = CG_GUN_ID_SMALL,
		.bps = 5.0,
		.dmg = 7.0,
		.knockback_force = 0.0,
		.bullet_speed = 7000,
		.autocharge = true,
		.initial_charge_time = 0,
	};

	const cg_gun_spec_t big_gun_spec = {
		.id = CG_GUN_ID_BIG,
		.bps = 0.75,
		.dmg = 95.0,
		.knockback_force = 10000.0,
		.bullet_speed = 10000,
		.autocharge = false,
		.initial_charge_time = 0,
	};

	const cg_gun_spec_t mini_gun_spec = {
		.id = CG_GUN_ID_MINI_GUN,
		.bps = 100.0,
		.dmg = 0.75,
		.knockback_force = 200.0,
		.bullet_speed = 9000,
		.autocharge = false,
		.initial_charge_time = 1.0,
	};

	coregame_add_gun_spec(&server->game, &small_gun_spec);
	coregame_add_gun_spec(&server->game, &big_gun_spec);
	coregame_add_gun_spec(&server->game, &mini_gun_spec);
}

static i32 
server_init_coregame(server_t* server)
{
	cg_runtime_map_t* map = cg_map_load(server->cgmap_path, &server->disk_map, &server->disk_map_size);
	if (map == NULL)
	{
		fprintf(stderr, "Failed to load map: %s\n", server->cgmap_path);
		return -1;
	}

	coregame_init(&server->game, false, map);
	server->game.user_data = server;
	server->game.player_changed = (cg_player_changed_callback_t)on_player_changed;
	server->game.player_damaged = (cg_player_damaged_callback_t)on_player_damaged;

	array_init(&server->spawn_points, sizeof(cg_runtime_cell_t**), 10);
	cg_runtime_cell_t* cell;

	for (u16 x = 0; x < map->w; x++)
	{
		for (u16 y = 0; y < map->h; y++)
		{
			cell = cg_runtime_map_at(map, x, y);
			if (cell->type == CG_CELL_SPAWN)
				array_add_voidp(&server->spawn_points, cell);
		}
	}

	server_init_coregame_gun_specs(server);

	return 0;
}

static i32
server_init_epoll(server_t* server)
{
	if ((server->epfd = epoll_create1(EPOLL_CLOEXEC)) == -1)
	{
		perror("epoll_create1");
		return -1;
	}

	server_add_event(server, server->tcp_sock.sockfd, NULL, server_handle_new_connection, NULL);
	server_add_event(server, server->udp_fd, NULL, server_read_udp_packet, NULL);

	return 0;
}

i32 
server_init(server_t* server, i32 argc, char* const* argv)
{
	server->port = DEFAULT_PORT;
	server->udp_port = DEFAULT_PORT + 1;
	server->routine_time = 20.0;
	server->client_timeout_threshold = 15.0;
	server_set_tickrate(server, TICKRATE);

	if (server_argv(server, argc, argv) == -1)
		return -1;

	ght_init(&server->clients, 10, free);

	if (server_init_tcp(server) == -1)
		goto err;
	if (server_init_udp(server) == -1)
		goto err;
	if (server_init_epoll(server) == -1)
		goto err;
	if (server_init_signalfd(server) == -1)
		goto err;
	if (server_init_coregame(server) == -1)
		goto err;
	if (server_init_timerfd(server) == -1)
		goto err;
	server_init_netdef(server);
	mmframes_init(&server->mmf);
	ssp_segbuff_init(&server->segbuf, 4, 0);

	server->running = true;

	return 0;
err:
	server_cleanup(server);
	return -1;
}
