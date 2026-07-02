#include "scr_bird.h"

#define BIRD_X              (20)
#define BIRD_RADIUS         (4)
#define GRAVITY             (1)
#define FLAP_STRENGTH       (12)
#define PIPE_WIDTH          (12)
#define PIPE_GAP_HEIGHT     (24)
#define PIPE_SPACING        (56)
#define PIPE_MOVE_SPEED     (2)
#define BIRD_UPDATE_INTERVAL (50)

#define PIPE_SPEED_MIN      (1)
#define PIPE_SPEED_MAX      (6)
#define PIPE_SPEED_STEP     (1)

#define PIPE_GAP_MIN        (18)
#define PIPE_GAP_MAX        (38)
#define PIPE_GAP_STEP       (2)

#define BIRD_GRAVITY_MIN    (1)
#define BIRD_GRAVITY_MAX    (8)
#define BIRD_GRAVITY_STEP   (1)

#define BIRD_SETTINGS_COUNT (3)
#define BIRD_SETTING_SPEED  (0)
#define BIRD_SETTING_GAP    (1)
#define BIRD_SETTING_GRAVITY (2)

typedef struct {
    int16_t x;
    int16_t y;
    int16_t velocity;
    bool    alive;
    uint16_t score;
} bird_t;

typedef struct {
    int16_t x;
    int16_t gap_y;
    bool    active;
} pipe_t;

static bird_t bird;
static pipe_t pipes[3];
static uint8_t current_pipe_index;
static bool game_started;
static bool game_over;

static int8_t pipe_speed = PIPE_MOVE_SPEED;
static int16_t pipe_gap = PIPE_GAP_HEIGHT;
static int8_t bird_gravity = GRAVITY;
static uint8_t bird_setting = BIRD_SETTING_SPEED;
static const char* bird_setting_label[BIRD_SETTINGS_COUNT] = {
    "BG SPD",
    "GAP",
    "DROP"
};

static void view_scr_bird();

view_dynamic_t dyn_view_bird = {
    {
        .item_type = ITEM_TYPE_DYNAMIC,
    },
    view_scr_bird
};

view_screen_t scr_bird = {
    &dyn_view_bird,
    ITEM_NULL,
    ITEM_NULL,
    .focus_item = 0,
};

static void bird_reset() {
    bird.x = BIRD_X;
    bird.y = LCD_HEIGHT / 2;
    bird.velocity = 0;
    bird.alive = true;
    bird.score = 0;
    current_pipe_index = 0;
    game_started = false;
    game_over = false;

    for (uint8_t i = 0; i < 3; i++) {
        pipes[i].active = true;
        pipes[i].x = LCD_WIDTH + i * PIPE_SPACING;
        pipes[i].gap_y = (pipe_gap / 2) + (rand() % (LCD_HEIGHT - pipe_gap));
    }
}

static bool bird_hit_pipe(pipe_t* pipe) {
    if (bird.x + BIRD_RADIUS < pipe->x || bird.x - BIRD_RADIUS > pipe->x + PIPE_WIDTH) {
        return false;
    }
    if (bird.y - BIRD_RADIUS > pipe->gap_y + pipe_gap / 2 || bird.y + BIRD_RADIUS < pipe->gap_y - pipe_gap / 2) {
        return true;
    }
    return false;
}

static void game_update() {
    if (!game_started || game_over) {
        return;
    }

    bird.velocity += (bird_gravity + 1) / 2;
    bird.y += bird.velocity;

    if (bird.y - BIRD_RADIUS < 0) {
        bird.y = BIRD_RADIUS;
        bird.velocity = 0;
    }
    if (bird.y + BIRD_RADIUS >= LCD_HEIGHT) {
        bird.y = LCD_HEIGHT - BIRD_RADIUS;
        bird.alive = false;
    }

    for (uint8_t i = 0; i < 3; i++) {
        pipes[i].x -= pipe_speed;
        if (pipes[i].x + PIPE_WIDTH < 0) {
            pipes[i].x = LCD_WIDTH;
            pipes[i].gap_y = (pipe_gap / 2) + (rand() % (LCD_HEIGHT - pipe_gap));
            bird.score++;
        }
        if (bird_hit_pipe(&pipes[i])) {
            bird.alive = false;
        }
    }

    if (!bird.alive) {
        game_over = true;
        BUZZER_PlaySound(BUZZER_SOUND_LOWSCORE);
    }
}

static void view_scr_bird() {
    view_render.clear();

    if (!game_started) {
        view_render.setTextSize(1);
        view_render.setTextColor(WHITE);
        view_render.setCursor(4, 0);
        view_render.print("MODE = SEL");
        view_render.setCursor(4, 10);
        view_render.print("UP = CHANGE");
        view_render.setCursor(4, 20);
        view_render.print("DOWN = START");

        view_render.setCursor(4, 34);
        view_render.print(bird_setting_label[0]);
        view_render.setCursor(64, 34);
        view_render.print(pipe_speed);

        view_render.setCursor(4, 44);
        view_render.print(bird_setting_label[1]);
        view_render.setCursor(64, 44);
        view_render.print(pipe_gap);

        view_render.setCursor(4, 54);
        view_render.print(bird_setting_label[2]);
        view_render.setCursor(64, 54);
        view_render.print(bird_gravity);

        view_render.setCursor(96, 34 + bird_setting * 10);
        view_render.print(">");
        return;
    }

    for (uint8_t i = 0; i < 3; i++) {
        if (pipes[i].active) {
            view_render.fillRect(pipes[i].x, 0, PIPE_WIDTH, pipes[i].gap_y - pipe_gap / 2, WHITE);
            view_render.fillRect(pipes[i].x, pipes[i].gap_y + pipe_gap / 2, PIPE_WIDTH, LCD_HEIGHT - (pipes[i].gap_y + pipe_gap / 2), WHITE);
        }
    }

    view_render.fillCircle(bird.x, bird.y, BIRD_RADIUS, WHITE);

    view_render.setTextSize(1);
    view_render.setTextColor(WHITE);
    view_render.setCursor(2, 0);
    view_render.print("S:");
    view_render.print((int)bird.score);

    if (game_over) {
        view_render.setCursor(34, 24);
        view_render.print("GAME OVER");
        view_render.setCursor(18, 38);
        view_render.print("PRESS DOWN");
    }
}

void scr_bird_handle(ak_msg_t* msg) {
    switch (msg->sig) {
    case SCREEN_ENTRY: {
        bird_reset();
        timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_IDLE);
        timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_RENDER_SCREEN, BIRD_UPDATE_INTERVAL, TIMER_PERIODIC);
    } break;

    case AC_DISPLAY_RENDER_SCREEN: {
        if (game_started && !game_over) {
            game_update();
        }
    } break;

    case AC_DISPLAY_BUTON_UP_PRESSED: {
        if (!game_started && !game_over) {
            switch (bird_setting) {
            case BIRD_SETTING_SPEED:
                pipe_speed += PIPE_SPEED_STEP;
                if (pipe_speed > PIPE_SPEED_MAX) {
                    pipe_speed = PIPE_SPEED_MIN;
                }
                break;

            case BIRD_SETTING_GAP:
                pipe_gap += PIPE_GAP_STEP;
                if (pipe_gap > PIPE_GAP_MAX) {
                    pipe_gap = PIPE_GAP_MIN;
                }
                break;

            case BIRD_SETTING_GRAVITY:
                bird_gravity += BIRD_GRAVITY_STEP;
                if (bird_gravity > BIRD_GRAVITY_MAX) {
                    bird_gravity = BIRD_GRAVITY_MIN;
                }
                break;
            }
        }
    } break;

    case AC_DISPLAY_BUTON_DOWN_PRESSED: {
        if (game_over) {
            bird_reset();
            timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_RENDER_SCREEN, BIRD_UPDATE_INTERVAL, TIMER_PERIODIC);
        }
        if (!game_started) {
            game_started = true;
        }
        bird.velocity = -FLAP_STRENGTH;
    } break;

    case AC_DISPLAY_BUTON_MODE_PRESSED: {
        if (!game_started && !game_over) {
            bird_setting = (bird_setting + 1) % BIRD_SETTINGS_COUNT;
        } else {
            timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_RENDER_SCREEN);
            SCREEN_TRAN(scr_idle_handle, &scr_idle);
        }
    } break;

    default:
        break;
    }
}
