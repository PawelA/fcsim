#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <GLFW/glfw3.h>
#include <fcsim.h>

#include "text.h"
#include "ui.h"
#include "loader.h"
#include "event.h"
#include "load_layer.h"
#include "arena_layer.h"
#include "render_xml.h"

int the_width = 800;
int the_height = 800;
int the_cursor_x = 0;
int the_cursor_y = 0;
int the_ui_scale = 4;
void *the_window = NULL;

struct arena_layer the_arena_layer;

int main(void)
{
    freopen("renderOutputTest.tst", "a+", stdout);
    // printf("%s", xml);

    struct fcsim_arena arena;
    fcsim_read_xml(xml, sizeof(xml), &arena);
    // printf("%d\n", arena.block_cnt);

    struct fcsim_handle *handle = fcsim_new(&arena);

    struct fcsim_block_stat block_stats[1000]; // haha who needs efficient memory usage right

    int ticks = 0;
    do
    {
        fcsim_get_block_stats(handle, block_stats);
        fcsim_step(handle);
        ticks++;
    } while (!fcsim_has_won(&arena, block_stats));
    ticks -= 1;

    printf("%d\n", ticks);
}

/*void draw_arena(struct fcsim_arena *arena, struct fcsim_block_stat *stats);

int main(void)
{
    const char* cmd = "ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s 800x600 -i - "
                  "-threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip output.mp4";

    FILE* ffmpeg = _popen(cmd, "wb");

    int buffer [800*600];

    GLFWwindow *window;

    arena_layer_init(&the_arena_layer);

    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_SAMPLES, 4);
    window = glfwCreateWindow(800, 800, "fcsim demo", NULL, NULL);
    if (!window)
        return 1;
    the_window = window;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    //text_setup_draw();
    arena_layer_show(&the_arena_layer);

    struct fcsim_handle* handle = fcsim_new(&the_arena_layer.arena);
    fcsim_read_xml(xml, sizeof(xml), &the_arena_layer.arena);
    struct fcsim_block_stat* stats = malloc(sizeof(struct fcsim_block_stat) * the_arena_layer.arena.block_cnt);

    int ticks = 0;
    do {
        fcsim_get_block_stats(handle, stats);
        fcsim_step(handle);
        ticks++;
    }
    while(!fcsim_has_won(&the_arena_layer.arena, stats));

    FILE*fp = fopen("renderOutputTest.txt", "w+");
    fprintf(fp, "ticks: %d", ticks);
    fclose(fp);

    fcsim_get_block_stats(handle, stats);
    draw_arena(&the_arena_layer.arena, stats);

    while (!glfwWindowShouldClose(window)) {
        //arena_layer_draw(&the_arena_layer);
        glClear(GL_COLOR_BUFFER_BIT);

        fcsim_get_block_stats(handle, stats);
        draw_arena(&the_arena_layer.arena, stats);

        glfwSwapBuffers(window);

        glReadPixels(0, 0, 800, 600, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        fwrite(buffer, sizeof(int)*800*600, 1, ffmpeg);

        glfwPollEvents();
    }

    free(stats);
    _pclose(ffmpeg);
    glfwTerminate();
    return 0;
}*/
