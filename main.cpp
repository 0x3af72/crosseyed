#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

#include <SDL2/include/SDL2/SDL.h>

#include "sdl_circle.hpp"

#define DOT_SIZE 4
#define SCREEN_WIDTH (DOT_SIZE * 120) // each frame is 120 pixels wide
#define SCREEN_HEIGHT (DOT_SIZE * 200) // each frame is 200 pixels tall
#define SSCOPE_MODE (true) // whether to show in stereoscope mode or not
#define IMMORTAL (false) // debugging switch
#define ATSR(x, y, w, h) (shift_rects.push_back({x, y, w, h})) // i dont want to keep typing push back (stands for add to shift rects)

// random boolean generator for generating dots
std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> b_dist(0, 1);
bool randBool(){
    return b_dist(mt);
}

// dots shifting for displaying rectangles
void shiftRect(std::vector<std::vector<bool>>& dots, SDL_Rect shift_rect, const int SHIFT_BY = 2){
    for (int y = shift_rect.y; y != shift_rect.y + shift_rect.h; y++){
        for (int x = shift_rect.x; x != shift_rect.x + shift_rect.w; x++){

            if (x >= dots[0].size() || x < 0 || y >= dots.size() || y < 0) continue;

            int nx = x;
            bool dot = dots[y][nx];

            for (int i = 0; i != SHIFT_BY; i++){
                nx -= 1; // shift dots to left
                if (nx < 0 || nx >= dots[y].size()) continue; // out of range, dont do anything
                dots[y][nx] = dot;
            }
            for (int offset = 1; offset != SHIFT_BY + 1; offset++){
                dots[y][nx + offset] = randBool();
            }
        }
    }
}

int main(int argc, char* argv[]){

    // sdl2
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* win = SDL_CreateWindow("crosseyed", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * (1 + SSCOPE_MODE), SCREEN_HEIGHT, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, 0);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    // dots matrix
    std::vector<std::vector<bool>> dots(SCREEN_HEIGHT / DOT_SIZE, std::vector<bool>(SCREEN_WIDTH / DOT_SIZE));
    std::vector<SDL_Rect> shift_rects;

    // frame limiting
    const int FPS = 30;
    const float FRAME_DELAY = 1000 / FPS;
    Uint32 frame_start = 0;
    int frame_time = 0;

    // player variables
    // x: 0 - 80, y: 0 - 160 (account for width)
    int pl_x = 0, pl_y = 8; // start bottom left, confine player to only moving left and right
    const int PL_WIDTH = 20;
    int score = 0; // score, increase when any obstacle disappears
    bool pl_died = false;

    // falling obstacles
    // when decrease fall delay spawn delay must be multiple
    std::vector<std::pair<int, int>> obstacles; // x and y values
    int FALL_DELAY = 15; // fall delay in ticks
    int SPAWN_DELAY = 30; // spawn delay
    int SPAWN_PER_TIME = 1;
    int fd_new = 0, sd_new = 0;
    std::uniform_int_distribution<int> sp_dist(1, 3); // spawn per time distribution

    // mainloop
    unsigned long long ticks = 0;
    bool win_quit = false;
    while (!win_quit){

        // spawn obstacles
        if (ticks % SPAWN_DELAY == 0){
            SPAWN_PER_TIME = sp_dist(mt);
            std::uniform_int_distribution<int> sx_dist(0, 4 - (SPAWN_PER_TIME - 1));
            int sx = sx_dist(mt);
            for (int i = 0; i != SPAWN_PER_TIME; i++){
                obstacles.push_back({sx, -1}); // multiply by PL_WIDTH later. -1 because when spawn, also move down
                sx += 1;
            }
            if (sd_new){
                SPAWN_DELAY = sd_new;
                sd_new = 0;
            }
        }

        // event handling
        SDL_Event ev;
        while (SDL_PollEvent(&ev) != 0){
            if (ev.type == SDL_QUIT){
                win_quit = true;
            } else if (ev.type == SDL_KEYDOWN){
                SDL_Keycode key = ev.key.keysym.sym; // get key pressed

                // movement (a and s)
                if (key == SDLK_a){
                    if (pl_x != 0) pl_x -= 1;
                } else if (key == SDLK_d){
                    if (pl_x != 4) pl_x += 1;
                }

            }
        }

        // render display clear
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_SetRenderDrawColor(ren, 225, 225, 225, 255);

        // draw game boundary
        ATSR(0, 0, 10, 200);
        ATSR(120 - 10, 0, 10, 200);
        ATSR(0, 0, 120, 10);
        ATSR(0, 200 - 10, 120, 10);

        // draw player
        ATSR(10 + pl_x * PL_WIDTH, 10 + pl_y * PL_WIDTH, PL_WIDTH, PL_WIDTH);

        // draw and update obstacles
        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(), [
                &shift_rects, &ticks, &score,
                &pl_x, &pl_y, &pl_died, &FALL_DELAY
            ](std::pair<int, int>& obs){
            ATSR(10 + obs.first * PL_WIDTH, 10 + obs.second * PL_WIDTH, PL_WIDTH, PL_WIDTH);
            // fall
            if (ticks % FALL_DELAY == 0){
                obs.second += 1;
            }
            // check if die
            if (obs.first == pl_x && obs.second == pl_y && !IMMORTAL){
                pl_died = true;
            }
            // increment score and whether to delete
            if (obs.second == 9){
                score += 1;
                return true;
            }
            return false;
        }), obstacles.end());
        if (ticks % FALL_DELAY == 0){
            if (fd_new){
                FALL_DELAY = fd_new;
                fd_new = 0;
            }
        }

        // displaying stuff
        if (SSCOPE_MODE){

            // generate dots for both sides and show unedited (left side)
            for (int y = 0; y != SCREEN_HEIGHT / DOT_SIZE; y++){
                for (int x = 0; x != SCREEN_WIDTH / DOT_SIZE; x++){
                    bool rand = randBool();
                    dots[y][x] = rand;
                    if (rand){
                        SDL_Rect dot_rct = {x * DOT_SIZE, y * DOT_SIZE, DOT_SIZE, DOT_SIZE};
                        SDL_RenderFillRect(ren, &dot_rct);
                    }
                }
            }

            // shift all rects
            for (SDL_Rect rct: shift_rects){
                shiftRect(dots, rct);
            }
            shift_rects.clear();
            
            // now show the edited dots
            for (int y = 0; y != dots.size(); y++){
                for (int x = 0; x != dots.size(); x++){
                    if (dots[y][x]){
                        SDL_Rect dot_rct = {SCREEN_WIDTH + x * DOT_SIZE, y * DOT_SIZE, DOT_SIZE, DOT_SIZE};
                        SDL_RenderFillRect(ren, &dot_rct);
                    }
                }
            }

            // small circles for getting stereogram
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 150);
            SDL_RenderFillCircle(ren, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 20, 15);
            SDL_RenderFillCircle(ren, SCREEN_WIDTH + SCREEN_WIDTH / 2, SCREEN_HEIGHT - 20, 15);

        } else {

            // simply draw all rects normally
            for (SDL_Rect rct: shift_rects){
                SDL_Rect shift_rect = {rct.x * DOT_SIZE, rct.y * DOT_SIZE, rct.w * DOT_SIZE, rct.h * DOT_SIZE};
                SDL_RenderFillRect(ren, &shift_rect);
            }
            shift_rects.clear();
        }

        // death animation
        if (pl_died){

            // displaying death
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 150);
            SDL_Rect whole_rect = {0, 0, SCREEN_WIDTH * (1 + SSCOPE_MODE), SCREEN_HEIGHT};
            SDL_RenderFillRect(ren, &whole_rect);
            SDL_RenderPresent(ren);

            // delay before reset
            SDL_Delay(3000);

            // reset variables
            std::cout << "DIED WITH SCORE " << score << "\n";
            pl_died = false;
            pl_x = 0;
            obstacles.clear();
            score = 0;
            int FALL_DELAY = 15;
            int SPAWN_DELAY = 30;
            fd_new = 0; sd_new = 0;
            continue;
        }

        SDL_RenderPresent(ren);

        // increase speed every 270 ticks
        if ((ticks + 1) % 270 == 0 && FALL_DELAY > 2){
            fd_new = FALL_DELAY - 1;
            sd_new = SPAWN_DELAY - 2;
            std::cout << "DELAYS SET TO: " << FALL_DELAY << " " << SPAWN_DELAY << "\n";
        }

        // frame track
        frame_time = SDL_GetTicks() - frame_start;
        if (FRAME_DELAY > frame_time){
            if (FRAME_DELAY - frame_time < 300) SDL_Delay(FRAME_DELAY - frame_time);
        }
        // std::cout << "DELAY TIME: " << (FRAME_DELAY - frame_time) << "ms\n";
        frame_start = SDL_GetTicks();

        ticks++; // increment ticks
    }

    // cleanup
    SDL_Quit();
    return 0;
}