#include "game_ui.h"
#include "app.h"
#include "gui/gui.h"
#include "cutils.h"

static void
game_ui_kills_window(client_game_t* game, struct nk_context* ctx)
{
	array_t* player_deaths = &game->player_deaths;
	u32 delete_count = 0;

	if (player_deaths->count == 0)
		return;

	player_kill_t* kills = (player_kill_t*)player_deaths->buf;
	player_kill_t* kill;

    static struct nk_rect kill_window_rect = {
        .w = 500,
        .h = 200
    };
	kill_window_rect.x = (game->ren->viewport.x - kill_window_rect.w);
	static nk_flags flags = NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_BACKGROUND | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(ctx, "kills", kill_window_rect, flags))
	{
		nk_layout_row_dynamic(ctx, 30, 1);

		for (u32 i = 0; i < player_deaths->count; i++)
		{
			kill = kills + i;
			char label[256];
			snprintf(label, 256, "%s -> %s", kill->attacker_name, kill->target_name); 
			nk_label(ctx, label, NK_TEXT_RIGHT);

			f64 elapsed_time = game->app->timer.start_time_s - kill->timestamp;

			if (elapsed_time >= game->death_kill_time)
				delete_count++;
		}
		if (nk_window_is_hovered(ctx))
			game->app->on_ui = false;
	}
	nk_end(ctx);

	for (u32 i = 0; i < delete_count; i++)
		array_erase(player_deaths, 0);
}

static void
game_ui_score_window(client_game_t* game, struct nk_context* ctx)
{
	static struct nk_rect score_window_rect = {
		.w = 200,
		.h = 50,
		.y = 0
	};
	score_window_rect.x = (game->ren->viewport.x / 2 - (score_window_rect.w / 2));

	if (nk_begin(ctx, "score", score_window_rect, NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT | NK_WINDOW_BACKGROUND))
	{
		nk_layout_row_dynamic(ctx, score_window_rect.h, 1);
		char label[64];
		snprintf(label, 64, "Kills: %u", game->player->core->stats.kills);
		nk_label(ctx, label, NK_TEXT_CENTERED);
		if (nk_window_is_hovered(ctx))
			game->app->on_ui = false;
	}
	nk_end(ctx);
}

static void 
game_ui_tab_display_player(struct nk_context* ctx, cg_player_t* player)
{
	char player_stats[64];
	nk_label(ctx, player->username, NK_TEXT_CENTERED);

	snprintf(player_stats, 64, "%u", player->stats.kills);
	nk_label(ctx, player_stats, NK_TEXT_CENTERED);

	snprintf(player_stats, 64, "%u", player->stats.deaths);
	nk_label(ctx, player_stats, NK_TEXT_CENTERED);

	snprintf(player_stats, 64, "%.2f", player->stats.ping);
	nk_label(ctx, player_stats, NK_TEXT_CENTERED);
}

static void
game_ui_tab_window(client_game_t* game, struct nk_context* ctx)
{
	wa_state_t* state = wa_window_get_state(game->app->window);
	if (state->key_map[WA_KEY_TAB] == 0)
		return;

	struct nk_rect rect = {
		.x = percent(20, game->ren->viewport.x),
		.y = percent(20, game->ren->viewport.y),
	};
	rect.w = percent(70, game->ren->viewport.x) - rect.x / 2;
	rect.h = percent(70, game->ren->viewport.y) - rect.y / 2;
	const ght_t* players = &game->cg.players;

	if (nk_begin(ctx, "Scoreboard", rect, NK_WINDOW_TITLE))
	{
		nk_layout_row_dynamic(ctx, 50, 4);
		
		nk_label(ctx, "PLAYERS", NK_TEXT_CENTERED);
		nk_label(ctx, "KILLS", NK_TEXT_CENTERED);
		nk_label(ctx, "DEATHS", NK_TEXT_CENTERED);
		nk_label(ctx, "PING", NK_TEXT_CENTERED);

		nk_layout_row_dynamic(ctx, 20, 4);

		game_ui_tab_display_player(ctx, game->player->core);

		GHT_FOREACH(cg_player_t* player, players, {
			if (player->id != game->player->core->id)
				game_ui_tab_display_player(ctx, player);
		});
		game->app->on_ui = true;
	}
	nk_end(ctx);
}

static void 
format_ns(char* buf, u64 max, i64 ns)
{
	if (ns > 1e6)
		snprintf(buf, max, "%.3f ms", (f64)ns / 1e6);
	else if (ns > 1000)
		snprintf(buf, max, "%zu us", ns / 1000);
	else
		snprintf(buf, max, "%zu ns", ns);
}

static void
game_ui_server_stats(client_game_t* game, struct nk_context* ctx)
{
	char* label = game->ui_label;
	nk_flags col0 = NK_TEXT_CENTERED;
	nk_flags col1 = NK_TEXT_CENTERED;

	nk_layout_row_dynamic(ctx, 300, 1);
	if (nk_group_begin(ctx, "Server Stats", NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_INPUT))
	{
		const server_stats_t* stats = &game->net->server_stats;
		nk_layout_row_dynamic(ctx, 20, 2);

		nk_label(ctx, "Tick time:", col0);
		format_ns(label, UI_LABEL_SIZE, stats->tick_time);
		nk_label(ctx, label, col1);

		nk_label(ctx, "Tick time avg:", col0);
		format_ns(label, UI_LABEL_SIZE, stats->tick_time_avg);
		nk_label(ctx, label, col1);
		
		nk_label(ctx, "Tick time (high):", col0);
		format_ns(label, UI_LABEL_SIZE, stats->tick_time_highest);
		nk_label(ctx, label, col1);

		nk_label(ctx, "UDP PPS in:", col0);
		snprintf(label, UI_LABEL_SIZE, "%u", stats->udp_pps_in);
		nk_label(ctx, label, col1);

		nk_label(ctx, "UDP PPS in:", col0);
		snprintf(label, UI_LABEL_SIZE, "%u bytes", stats->udp_pps_in_bytes);
		nk_label(ctx, label, col1);

		nk_label(ctx, "UDP PPS in (high):", col0);
		snprintf(label, UI_LABEL_SIZE, "%u bytes", stats->udp_pps_in_bytes_highest);
		nk_label(ctx, label, col1);

		nk_label(ctx, "UDP PPS out:", col0);
		snprintf(label, UI_LABEL_SIZE, "%u", stats->udp_pps_out);
		nk_label(ctx, label, col1);

		nk_label(ctx, "UDP PPS out:", col0);
		snprintf(label, UI_LABEL_SIZE, "%u bytes", stats->udp_pps_out_bytes);
		nk_label(ctx, label, col1);

		nk_label(ctx, "UDP PPS out (high):", col0);
		snprintf(label, UI_LABEL_SIZE, "%u bytes", stats->udp_pps_out_bytes_highest);
		nk_label(ctx, label, col1);

		nk_group_end(ctx);
	}
	nk_layout_row_dynamic(ctx, 30, 1);
}

static void
game_send_bot_mode(client_game_t* game)
{
	net_tcp_bot_mode_t mode = {.is_bot = game->bot};

	ssp_segbuf_add(&game->net->tcp.buf, NET_TCP_BOT_MODE, sizeof(net_tcp_bot_mode_t), &mode);
	ssp_tcp_send_segbuf(&game->net->tcp.sock, &game->net->tcp.buf);
}

static void
game_ui_stats_window(client_game_t* game, struct nk_context* ctx)
{
    wa_state_t* state = wa_window_get_state(game->app->window);
	waapp_t* app = game->app;
	char* label = game->ui_label;

    static nk_flags win_flags = 
        NK_WINDOW_BORDER | 
         NK_WINDOW_TITLE |
        NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE;
    const char* window_name = state->window.title;
    // static struct nk_vec2 window_offset = {10, 10};
    static struct nk_rect window_rect = {
        .x = 0,
        .y = 0,
        .w = 340,
        .h = 840
    };

    if (nk_begin(ctx, window_name, window_rect, win_flags))
                 
    {
        if (win_flags & NK_WINDOW_MINIMIZED)
            win_flags ^= NK_WINDOW_MINIMIZED;

		snprintf(label, UI_LABEL_SIZE, "Server: %s:%u (%.1f Hz)",
				game->net->udp.ipaddr, game->net->udp.port, game->net->udp.tickrate);
        nk_layout_row_dynamic(ctx, 20, 1);
		nk_label(ctx, label, NK_TEXT_LEFT);

		if (game->net->udp.latency < 1.0)
			snprintf(label, UI_LABEL_SIZE, "RTT: %.4f ms", game->net->udp.latency);
		else
			snprintf(label, UI_LABEL_SIZE, "RTT: %.2f ms", game->net->udp.latency);
		nk_label(ctx, label, NK_TEXT_LEFT);

		if (game->net->udp.jitter < 1.0)
			snprintf(label, UI_LABEL_SIZE, "Jitter: %.4f ms", game->net->udp.jitter);
		else
			snprintf(label, UI_LABEL_SIZE, "Jitter: %.2f ms", game->net->udp.jitter);
		nk_label(ctx, label, NK_TEXT_LEFT);

		snprintf(label, UI_LABEL_SIZE, "FPS: %u (%.2f ms)", app->fps, app->frame_time);
		nk_label(ctx, label, NK_TEXT_LEFT);

		wa_state_t* state = wa_window_get_state(app->window);
		nk_bool vsync = !state->window.vsync;
		if (nk_checkbox_label(ctx, "VSync", &vsync))
		{
			app->update_vync = true;
			app->tmp_vsync = !vsync;
		}

        nk_layout_row_dynamic(ctx, 20, 1);
		if (vsync)
		{
			nk_bool limit_fps = !app->fps_limit;
			if (nk_checkbox_label(ctx, "Limit FPS", &limit_fps))
				app->fps_limit = !limit_fps;

			if (app->fps_limit)
			{
				snprintf(label, UI_LABEL_SIZE, "FPS LIMIT: %u", (u32)app->max_fps);
				nk_label(ctx, label, NK_TEXT_LEFT);
				if (nk_slider_float(ctx, 10.0, &app->max_fps, 500.0, 1.0))
					waapp_set_max_fps(app, app->max_fps);
			}
		}
        const char* fullscreen_str = (state->window.state & WA_STATE_FULLSCREEN) ? "Windowed" : "Fullscreen";
        if (nk_button_label(ctx, fullscreen_str))
            wa_window_set_fullscreen(app->window, !(state->window.state & WA_STATE_FULLSCREEN));

		nk_layout_row_dynamic(ctx, 160, 1);
		if (nk_group_begin(ctx, "SSP RX", 
					 NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
		{
			nk_layout_row_dynamic(ctx, 20, 2);

			nk_label(ctx, "PPS", NK_TEXT_CENTERED);
			snprintf(label, UI_LABEL_SIZE, "%u (%zu bytes)", 
					game->net->udp.in.last_count, game->net->udp.in.last_bytes);
			nk_label(ctx, label, NK_TEXT_LEFT);

			nk_label(ctx, "TOTAL:", NK_TEXT_CENTERED);
			snprintf(label, UI_LABEL_SIZE, "%u", 
					game->net->udp.buf.in_total_packets);
			nk_label(ctx, label, NK_TEXT_LEFT);

			nk_label(ctx, "DROPPED:", NK_TEXT_CENTERED);
			snprintf(label, UI_LABEL_SIZE, "%u", 
					game->net->udp.buf.in_dropped_packets);
			nk_label(ctx, label, NK_TEXT_LEFT);

			nk_label(ctx, "LOST:", NK_TEXT_CENTERED);
			snprintf(label, UI_LABEL_SIZE, "%u", 
					game->net->udp.buf.sliding_window.lost_packets);
			nk_label(ctx, label, NK_TEXT_LEFT);

			nk_label(ctx, "BUFFERED:", NK_TEXT_CENTERED);
			snprintf(label, UI_LABEL_SIZE, "%u", 
					game->net->udp.buf.sliding_window.count);
			nk_label(ctx, label, NK_TEXT_LEFT);

			nk_group_end(ctx);
		}

		nk_layout_row_dynamic(ctx, 140, 1);
		if (nk_group_begin(ctx, "SSP TX", 
					 NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
		{
			nk_layout_row_dynamic(ctx, 20, 2);

			nk_label(ctx, "PPS:", NK_TEXT_CENTERED);
			snprintf(label, UI_LABEL_SIZE, "%u (%zu bytes)", 
					game->net->udp.out.last_count, game->net->udp.out.last_bytes);
			nk_label(ctx, label, NK_TEXT_LEFT);

			nk_label(ctx, "TOTAL:", NK_TEXT_CENTERED);
			snprintf(label, UI_LABEL_SIZE, "%u", 
					game->net->udp.buf.out_total_packets);
			nk_label(ctx, label, NK_TEXT_LEFT);

			nk_label(ctx, "RTO:", NK_TEXT_CENTERED);
			snprintf(label, UI_LABEL_SIZE, "%u", 
					game->net->udp.buf.rto);
			nk_label(ctx, label, NK_TEXT_LEFT);

			nk_label(ctx, "BUFFERED:", NK_TEXT_CENTERED);
			snprintf(label, UI_LABEL_SIZE, "%u", 
					game->net->udp.buf.important_packets.count);
			nk_label(ctx, label, NK_TEXT_LEFT);

			nk_group_end(ctx);
		}
        nk_layout_row_dynamic(ctx, 20, 1);

		snprintf(label, UI_LABEL_SIZE, "Local Interpolate Factor: %f", game->cg.local_interp_factor);
		nk_label(ctx, label, NK_TEXT_LEFT);

		snprintf(label, UI_LABEL_SIZE, "Remote Interpolate Factor: %f", game->cg.remote_interp_factor);
		nk_label(ctx, label, NK_TEXT_LEFT);

		nk_bool ignore_auto_interp = !game->ignore_auto_interp;
		if (nk_checkbox_label(ctx, "Custom Interpolation", &ignore_auto_interp))
		{
			game->ignore_auto_interp = !ignore_auto_interp;
		}

		if (game->ignore_auto_interp)
		{
			snprintf(label, UI_LABEL_SIZE, "Target Local Interpolate Factor: %f", game->cg.target_local_interp_factor);
			nk_label(ctx, label, NK_TEXT_LEFT);
			nk_slider_float(ctx, 0.00001, &game->cg.target_local_interp_factor, 0.2, 0.00001);

			snprintf(label, UI_LABEL_SIZE, "Target Remote Interpolate Factor: %f", game->cg.target_remote_interp_factor);
			nk_label(ctx, label, NK_TEXT_LEFT);
			nk_slider_float(ctx, 0.00001, &game->cg.target_remote_interp_factor, 0.2, 0.00001);
		}

		snprintf(label, UI_LABEL_SIZE, "Interp Threshold Dist: %f", game->cg.interp_threshold_dist);
		nk_label(ctx, label, NK_TEXT_LEFT);
		nk_slider_float(ctx, 0.0001, &game->cg.interp_threshold_dist, 50.0, 0.0001);

		nk_bool pause = !game->cg.pause;
		if (nk_checkbox_label(ctx, (pause) ? "Pause" : "Play", &pause))
		{
			game->cg.pause = !pause;
		}

		snprintf(label, UI_LABEL_SIZE, "Time Scale: %f", game->cg.time_scale);
		nk_label(ctx, label, NK_TEXT_LEFT);
		nk_slider_float(ctx, 0, &game->cg.time_scale, 10.0, 0.1);

		nk_bool game_debug = !game->game_debug;
		if (nk_checkbox_label(ctx, "Game Debug", &game_debug))
		{
			game->game_debug = !game_debug;
		}

		nk_bool game_netdebug = !game->game_netdebug;
		if (nk_checkbox_label(ctx, "Game Net Debug", &game_netdebug))
		{
			game->game_netdebug = !game_netdebug;
		}

		nk_bool bot = !game->bot;
		if (nk_checkbox_label(ctx, "Bot Mode", &bot))
		{
			game->bot = !bot;
			game_send_bot_mode(game);
		}

		if (game->bot)
		{
			snprintf(label, UI_LABEL_SIZE, "Bot interval: %.2fs", game->bot_interval);
			nk_label(ctx, label, NK_TEXT_LEFT);
			nk_slider_float(ctx, 0.1, &game->bot_interval, 5.0, 0.1);
		}

		snprintf(label, UI_LABEL_SIZE, "Draw calls: %u", game->ren->draw_calls);
		nk_label(ctx, label, NK_TEXT_LEFT);
		game->ren->draw_calls = 0;

		nk_bool get_server_stats = !app->get_server_stats;
		if (nk_checkbox_label(ctx, "Get server stats", &get_server_stats))
		{
			app->get_server_stats = !get_server_stats;

			net_tcp_want_server_stats_t want_stats = {app->get_server_stats};
			ssp_segbuf_add(&app->net.tcp.buf, NET_TCP_WANT_SERVER_STATS, sizeof(net_tcp_want_server_stats_t), &want_stats);
			ssp_tcp_send_segbuf(&app->net.tcp.sock, &app->net.tcp.buf);
		}

		if (app->get_server_stats)
			game_ui_server_stats(game, ctx);

		nk_layout_row_dynamic(ctx, 30, 1);

		if (nk_button_label(ctx, "Main Menu"))
			waapp_state_switch(app, &app->sm.states.main_menu);

		if (nk_button_label(ctx, "Quit"))
			wa_window_stop(app->window);

		if (nk_window_is_hovered(ctx))
			game->app->on_ui = true;
    }
    nk_end(ctx);
}

static void
game_ui_chat_window(client_game_t* game, struct nk_context* ctx)
{
	f64 current_time = game->app->timer.start_time_s;
	if (game->open_chat == false)
	{
		f64 elapsed_time = current_time - game->last_chatmsg;
		if (elapsed_time > 10.0)
			return;
	}
	else 
		game->last_chatmsg = current_time;

	struct nk_rect rect = {
		.w = 300,
		.h = 300,
		.x = 0,
	};
	rect.y = game->ren->viewport.y - rect.h;
	bool show = game->open_chat;
	bool fake_show = false;
	nk_flags group_flags = 0;
	struct nk_style_item og_bg = ctx->style.window.fixed_background;

	struct nk_vec2 prev_padding = ctx->style.window.padding;
	static bool should_focus = true;

	if (game->new_msg && show == false)
		show = fake_show = true;

	if (show == false)
	{
		group_flags |= NK_WINDOW_NO_SCROLLBAR;
		should_focus = true;
		ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
	}
	else if (fake_show)
	{
		ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
	}

	if (show)
		game->app->on_ui = true;

	if (nk_begin(ctx, "Chat", rect, NK_WINDOW_BACKGROUND | NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_dynamic(ctx, 250, 1);

		const char* group_name = "chat_group";

		if (nk_group_begin(ctx, group_name, group_flags))
		{
			nk_uint scroll_x, scroll_y = 0;
			nk_group_get_scroll(ctx, group_name, &scroll_x, &scroll_y);

			u32 fheight = (u32)ctx->style.font->height;

			const chatmsg_t* msgs = (const chatmsg_t*)game->chat_msgs.buf;

			for (u32 i = 0; i < game->chat_msgs.count; i++)
			{
				const chatmsg_t* msg = msgs + i;
				u32 len = msg->name_len;
				const char* name = msg->name;

				struct nk_color name_color;

				if (*name == 0x00)
				{
					name = "SERVER:";
					name_color = nk_rgb_f(1.0, 0.0, 0);
					len = strlen(name);
				}
				else
					name_color = nk_rgb_f(0.0, 1.0, 0);

				u32 width = fheight / 2 * len;
				u32 msg_width = fheight / 2 * msg->msg_len;
				
				u32 x = nk_window_get_width(ctx);

				u32 h = fheight + 1;
				bool too_long = false;
				u32 total = width + msg_width + h;
				if (total >= x - h)
					too_long = true;

				if (too_long)
				{
					u32 cols = msg_width / (x - width) + 1;

					nk_layout_row_dynamic(ctx, h, 1);
					nk_label_colored(ctx, name, NK_TEXT_LEFT, name_color);

					nk_layout_row_dynamic(ctx, h * cols, 1);
					nk_label_colored_wrap(ctx, msg->msg, nk_rgb(255, 255, 255));
				}
				else
				{
					nk_layout_row_template_begin(ctx, h);
					nk_layout_row_template_push_static(ctx, width);
					nk_layout_row_template_push_dynamic(ctx);
					nk_layout_row_template_end(ctx);
					
					nk_label_colored(ctx, name, NK_TEXT_LEFT, name_color);
					nk_label_colored_wrap(ctx, msg->msg, nk_rgb(255, 255, 255));
				}
			}

			if (game->new_msg)
			{
				nk_group_set_scroll(ctx, group_name, scroll_x, -1);
				game->new_msg = false; 
			}
			nk_group_end(ctx);
		}

		ctx->style.window.padding = prev_padding;

		if (show)
		{
			nk_layout_row_template_begin(ctx, 30);
			nk_layout_row_template_push_static(ctx, 240);
			
			static char buffer[CHAT_MSG_MAX];
			nk_flags flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;

			if (should_focus)
			{
				nk_edit_focus(ctx, flags);
			}

			nk_flags ret;
			if ((ret = nk_edit_string_zero_terminated(ctx, flags, buffer, CHAT_MSG_MAX, nk_filter_ascii)) && ret & NK_EDIT_COMMITED)
			{
				if (should_focus == false)
				{
					game_send_chatmsg(game, buffer);
					*buffer = 0x00;
				}
				should_focus = false;
			}
			nk_layout_row_template_push_dynamic(ctx);
			nk_button_label(ctx, "SEND");
		}

		if (nk_window_is_hovered(ctx) && show == false)
			game->app->on_ui = false;
	}
	nk_end(ctx);
	
	if (!show || fake_show)
		ctx->style.window.fixed_background = og_bg;
}

static void
game_ui_healthbar_window(client_game_t* game, struct nk_context* ctx)
{
	const rect_t* r = &game->health_bar.background; 
	char* label = game->ui_label;

	if (nk_begin(ctx, "health", nk_rect(r->pos.x, r->pos.y, r->size.x, r->size.y), 
			  NK_WINDOW_NO_INPUT | NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_dynamic(ctx, r->size.y, 1);
		f32 health = game->player->core->health;
		if (health == game->player->core->max_health)
			snprintf(label, UI_LABEL_SIZE, "%.0f HP", health);
		else if (health <= 0)
			snprintf(label, UI_LABEL_SIZE, "%f HP", health);
		else
			snprintf(label, UI_LABEL_SIZE, "%.2f HP", health);
		nk_label_colored(ctx, label, NK_TEXT_RIGHT, nk_rgba_f(1, 1, 1, 1.0));
	}
	nk_end(ctx);
}

static void
game_ui_ammo_window(client_game_t* game, struct nk_context* ctx)
{
	const rect_t* guncharge = &game->guncharge_bar.background;
	const rect_t* healthbar = &game->health_bar.background;
	struct nk_rect bounds = nk_rect(
		guncharge->pos.x, 
		guncharge->pos.y, 
		healthbar->size.x, 
		healthbar->size.y
	);
	bounds.y -= bounds.h;
	char* label = game->ui_label;

	if (nk_begin(ctx, "ammo", bounds, 
			  NK_WINDOW_BACKGROUND | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NOT_INTERACTIVE))
	{
		const cg_gun_t* gun = game->player->core->gun;
		nk_layout_row_dynamic(ctx, bounds.h, 1);
		if (gun->ammo <= 0)
		{
			f32 time_left = (gun->spec->reload_time - gun->reload_time);
			snprintf(label, UI_LABEL_SIZE, "%.2fs", time_left);
		}
		else
			snprintf(label, UI_LABEL_SIZE, "Ammo %d/%d", gun->ammo, gun->spec->max_ammo);
		nk_label(ctx, label, NK_TEXT_RIGHT);
	}
	nk_end(ctx);
}

void 
game_ui_update(client_game_t* game)
{
	if (game->player == NULL)
		return;

	struct nk_context* ctx = game->nk_ctx;

	game_ui_stats_window(game, ctx);
	game_ui_chat_window(game, ctx);

	gui_set_font(game->app, game->app->font_big);
	struct nk_style_item og_bg = ctx->style.window.fixed_background;
	ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));

	game_ui_kills_window(game, ctx);
	game_ui_score_window(game, ctx);

	struct nk_vec2 padding = ctx->style.window.padding;
	ctx->style.window.padding = nk_vec2(0, 0);
	game_ui_healthbar_window(game, ctx);
	game_ui_ammo_window(game, ctx);
	ctx->style.window.padding = padding;

	ctx->style.window.fixed_background = og_bg;
	gui_set_font(game->app, game->app->font);

	game_ui_tab_window(game, ctx);
}
