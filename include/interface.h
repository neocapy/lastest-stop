#pragma once

// interface.h
//
// Functions in the interface module offer a low-level API for window, input,
// graphics, and audio access. These are stateful calls that go through SDL2, 
// but absolutely no SDL2 implementation details should leak.

#include <string>
#include <vector>
#include <map>
#include <memory>

void interface_setup();
void interface_process_events();
void interface_render();
void interface_teardown();

bool interface_quit_requested();


