#include <SDL.h>
#include "config.h"
#include "interface.h"
#include "output_queue.h"

int main(int argc, char *argv[]) {
    
    output_queue_start_thread();

    interface_setup();
    
    while (!interface_quit_requested()) {
        interface_process_events();
        interface_render();

        SDL_Delay(10);
    }
    
    interface_teardown();
    
    return 0;

}
