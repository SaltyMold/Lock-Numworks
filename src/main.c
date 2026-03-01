#include "libs/eadk.h"
#include "libs/storage.h"
#include "assets/lock_closed.h"
#include "assets/lock_open.h"

#define SIMULATOR 0

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Lock";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

#if SIMULATOR
void disable_keyboard(){}
void off_screen(){}
void restore_keyboard(){}
#else

void disable_keyboard(){
    __asm__ volatile("svc #54"); // SVC_USB_WILL_EXECUTE_DFU
    // need off_screen() to reactivate keyboard
}

void off_screen(){
    __asm__ volatile("svc #44"); // SVC_POWER_SUSPEND
}

void restore_keyboard(){
    // Restore On/Off and Home kernel handling
    __asm__ volatile("svc #51"); // SVC_USB_DID_EXECUTE_DFU
}

#endif

static const struct { eadk_key_t key; char ch; } keymap[] = {
    { eadk_key_zero, '0' }, { eadk_key_one, '1' }, { eadk_key_two, '2' },
    { eadk_key_three, '3' }, { eadk_key_four, '4' }, { eadk_key_five, '5' },
    { eadk_key_six, '6' }, { eadk_key_seven, '7' }, { eadk_key_eight, '8' },
    { eadk_key_nine, '9' }
};

int main(void) {
    eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_black);
    eadk_display_push_rect((eadk_rect_t){EADK_SCREEN_WIDTH / 2 - lock_open_width / 2, 10, lock_open_width, lock_open_height}, lock_open_data);
    eadk_display_draw_string("Enter password to lock", (eadk_point_t){EADK_SCREEN_WIDTH / 2 - 22/2*10, 10 + lock_open_height + 10}, true, eadk_color_white, eadk_color_black);
    
    eadk_keyboard_state_t state;
    char lock_buffer[5] = "";
    int lock_len = 0;

    for (;;) {
        state = eadk_keyboard_scan();

        for (int i = 0; i < 4; i++) {
            char s[2] = { lock_buffer[i] ? lock_buffer[i] : 'X', '\0' };
            eadk_display_draw_string(s, (eadk_point_t){EADK_SCREEN_WIDTH / 2 - 4*10 + i * 20 + 5, 10 + lock_open_height + 30}, true, eadk_color_white, eadk_color_black);
        }
        
        for (int k = 0; k < sizeof(keymap) / sizeof(keymap[0]); ++k) {
            if (eadk_keyboard_key_down(state, keymap[k].key)) {
                if (lock_len < 4) {
                    lock_buffer[lock_len++] = keymap[k].ch;
                    lock_buffer[lock_len] = '\0';
                    while (eadk_keyboard_key_down(state, keymap[k].key)) state = eadk_keyboard_scan();
                }
            }
        }

        if (eadk_keyboard_key_down(state, eadk_key_backspace)) {
            if (lock_len > 0) {
                lock_buffer[--lock_len] = '\0';
                while (eadk_keyboard_key_down(state, eadk_key_backspace)) state = eadk_keyboard_scan();
            }
        }

        if (eadk_keyboard_key_down(state, eadk_key_ok)) {
            if (lock_len == 4) break;
        }
    }

    char lock_code[5] = "";
    memcpy(lock_code, lock_buffer, lock_len + 1);
    for (int i = 0; i < 4; i++) lock_buffer[i] = '\0';
    lock_len = 0;
    
    disable_keyboard();
    off_screen();
    
    eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_black);
    eadk_display_push_rect((eadk_rect_t){EADK_SCREEN_WIDTH / 2 - lock_closed_width / 2, 10 + 12, lock_closed_width, lock_closed_height}, lock_closed_data);
    eadk_display_draw_string("Enter password to unlock", (eadk_point_t){EADK_SCREEN_WIDTH / 2 - 24/2*10, 10 + lock_closed_height + 12 + 10}, true, eadk_color_white, eadk_color_black);
    
    for (;;) {
        state = eadk_keyboard_scan();

        for (int i = 0; i < 4; i++) {
            char s[2] = { lock_buffer[i] ? lock_buffer[i] : 'X', '\0' };
            eadk_display_draw_string(s, (eadk_point_t){EADK_SCREEN_WIDTH / 2 - 4*10 + i * 20 + 5, 10 + lock_closed_height + 12 + 30}, true, eadk_color_white, eadk_color_black);
        }
        
        for (int k = 0; k < sizeof(keymap) / sizeof(keymap[0]); ++k) {
            if (eadk_keyboard_key_down(state, keymap[k].key)) {
                if (lock_len < 4) {
                    lock_buffer[lock_len++] = keymap[k].ch;
                    lock_buffer[lock_len] = '\0';
                    while (eadk_keyboard_key_down(state, keymap[k].key)) state = eadk_keyboard_scan();
                }
            }
        }

        if (eadk_keyboard_key_down(state, eadk_key_backspace)) {
            if (lock_len > 0) {
                lock_buffer[--lock_len] = '\0';
                while (eadk_keyboard_key_down(state, eadk_key_backspace)) state = eadk_keyboard_scan();
            }
        }

        if (eadk_keyboard_key_down(state, eadk_key_ok)) {
            if (strcmp(lock_buffer, lock_code) == 0) break;
        }

        if (eadk_keyboard_key_down(state, eadk_key_on_off)) {
            off_screen();
            eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_black);
            eadk_display_push_rect((eadk_rect_t){EADK_SCREEN_WIDTH / 2 - lock_closed_width / 2, 10 + 12, lock_closed_width, lock_closed_height}, lock_closed_data);
            eadk_display_draw_string("Enter password to unlock", (eadk_point_t){EADK_SCREEN_WIDTH / 2 - 24/2*10, 10 + lock_closed_height + 12 + 10}, true, eadk_color_white, eadk_color_black);
        }
    }

    restore_keyboard();
    return 0;
}

