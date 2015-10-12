#include "arcade.h"
#include "field.h"
#include "options.h"
#include "io.h"

inline int options_quick_modifiable()
{
    return ((snake[0].alive || snake[1].alive) && (restart_after_timer || timer < 255));
}

void start_arcade_play()
{
    // let bullet/snake dynamics be ON:
    dynamics = 1;
    // black everything:
    clear();

    // setup the snakes:
    if (single_player)
    {
        init_snake(0, 60,90, UP, starting_size);
        snake[1].alive = 0;
    }
    else
    {
        init_snake(0, 60,90, UP, starting_size);
        init_snake(1, 60,70, DOWN, starting_size);
    }

    // put the snake on the map
    for (int p=0; p<2-single_player; ++p)
        superpixel[snake[p].head.y][snake[p].head.x] = snake[p].color;

    if (!torus)
        make_walls();
    make_food(food_count);
}

void show_arcade_options()
{
    show_duel_options();
    show_torus_options();
    show_speed_options();
    show_food_options();
    show_size_options();
    show_gun_options();
}

int handle_arcade_meta()
{
    if (gamepad_press[0] & gamepad_start)
    {
        if (timer == 255) // if we were paused...
        {
            if (restart_after_timer)
            {
                message("unpausing from menu\n");
                start_play_countdown();
                return 1;
            }
            message("unpausing\n");
            timer = 0;
        }
        else if (timer == 0) // we weren't paused
        {
            if (GAMEPAD_PRESSED(0, R))
            {
                restart_after_timer = 1;
                show_options();
                show_controls();
            }
            timer = 255;
        }
    }

    // pause game if it's on a timer...
    if (timer)
    {
        if (gamepad_press[0] & gamepad_select)
        {   // save screen shot
            message("taking picture\n");
            take_screenshot();
            return 1;
        }
        if (timer < 255) // not paused, probably the game preparing for something...
        {
            if (gamepad_press[0] & gamepad_start)
            {
                // if we just pressed start, either go to special options (if R is pressed):
                if (GAMEPAD_PRESSED(0, R))
                {
                    restart_after_timer = 1;
                    timer = 255;
                    superpixel[56][80] = bg_color;
                    superpixel[58][80] = bg_color;
                    superpixel[60][80] = bg_color;
                    show_controls();
                    show_options();
                }
                else // or speed up whatever is happening:
                {
                    timer = 0; // shortcut
                    message("speeding up whatever is happening (by start)\n");
                    if (restart_after_timer)
                        start_play_countdown();
                    else
                        start_game_play();
                    return 1;
                }
            }
            if (vga_frame % 60 == 0)
            {
                --timer;
                if (restart_after_timer)
                {   
                    // showing the player their mistake,
                    // until timer runs out:
                    if (!timer)
                        start_play_countdown();
                }
                else // no restart, count down!
                {
                    switch (timer)
                    {
                    case 3:
                        superpixel[56][80] = RGB(255,0,0);
                        superpixel[58][80] = RGB(255,255,0);
                        superpixel[60][80] = RGB(0,255,0);
                        break;
                    case 2:
                        superpixel[56][80] = bg_color;
                        break;
                    case 1:
                        superpixel[58][80] = bg_color;
                        break;
                    case 0:
                        start_game_play();
                        break;
                    }

                }
            }
        }
        update_arcade_options();
        return 1;
    }

    return 0;
}

void update_arcade_options()
{
    if (gamepad_press[0] & gamepad_down)
    {
        if (speed < 9)
            ++speed;
        message("speed = %d\n", (int)speed);
        if (options_quick_modifiable())
            show_speed_options();
    }
    else if (gamepad_press[0] & gamepad_up)
    {
        if (speed > 1)
            --speed;
        message("speed = %d\n", (int)speed);
        if (options_quick_modifiable())
            show_speed_options();
    }
    if (gamepad_press[0] & gamepad_Y)
    {
        torus = 1 - torus;
        message("torus = %d\n", (int)torus);
        if (torus)
        {
            // can safely make it into a torus, without resetting game
            if (options_quick_modifiable())
                show_torus_options();
            remove_walls();
        }
        else
        {
            // but cannot safely add walls without possibly breaking the game
            if (options_quick_modifiable())
                show_torus_options();
            else
            {
                restart_after_timer = 1; // you must reset the game
                timer = 255;
                show_options();
            }

            make_walls();
        }
    }
    if (gamepad_press[0] & gamepad_B)
    {   // add/remove bullets
        bullet_length = (bullet_length + 1)%4;
        if (bullet_length == 0)
        {   // we switched from guns to no guns, this requires a reset of the game...
            // kill all alive bullets:
            for (int p=0; p<2; ++p)
            for (int b=0; b<BULLETS; ++b)
                if (bullet[p][b].alive)
                {
                    superpixel[bullet[p][b].y][bullet[p][b].x] = bg_color;
                    bullet[p][b].alive = 0;
                }
                
        }
        if (options_quick_modifiable())
            show_gun_options();
    }
    if (gamepad_press[0] & gamepad_X)
    {
        single_player = 1 - single_player;
        message("single_player = %d\n", (int)single_player);
        if (options_quick_modifiable())
            show_duel_options();
        else
        { // create or destroy second player now
            if (single_player)
            {   // zip up tail til it reaches the head
                // now can remove the head (and tail):
                zip_snake(1, snake[1].head.y, snake[1].head.x,  bg_color);
                // kill off head, too:
                superpixel[snake[1].head.y][snake[1].head.x] = bg_color;
                // kill snake 1:
                snake[1].alive = 0;
            }
            else
            {
                // find a spot to put second player
                uint8_t y = rand()%SCREEN_H, x = rand()%SCREEN_W;
                while (superpixel[y][x] != bg_color)
                {
                    y = rand()%SCREEN_H;
                    x = rand()%SCREEN_W;
                }
                init_snake(1, y,x, rand()%4, starting_size);
            }
        }
    }
    if (gamepad_press[0] & gamepad_R)
    {
        if (starting_size < 13000)
        {
            int32_t increment = option_increment(starting_size);

            starting_size += increment;
            for (int p=0; p<2-single_player; ++p)
                snake[p].tail_wait += increment;
            message("size = %d\n", (int)starting_size);
            if (options_quick_modifiable())
                show_size_options();
        }
    }
    else if (gamepad_press[0] & gamepad_L)
    {
        if (starting_size > 1)
        {
            uint32_t decrement = option_decrement(starting_size);

            starting_size -= decrement;
            for (int p=0; p<2-single_player; ++p)
                snake[p].tail_wait -= decrement;
            message("size = %d\n", (int)starting_size);
            if (options_quick_modifiable())
                show_size_options();
        }
    }
    if (gamepad_press[0] & gamepad_right)
    {
        if (food_count < FOOD)
        {
            int32_t increment = option_increment(food_count);

            food_count += increment;
            make_food(increment);
            
            if (options_quick_modifiable())
                show_food_options();
        }
    }
    else if (gamepad_press[0] & gamepad_left)
    {
        if (food_count)
        { 
            int32_t decrement = option_decrement(food_count);

            food_count -= decrement;
            if (options_quick_modifiable())
                show_food_options();
            else
            {
                restart_after_timer = 1;
                show_options();
            }
        }
    }
}
