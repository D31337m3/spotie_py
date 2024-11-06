#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
#include <unistd.h>
#include <signal.h>

static volatile int running = 1;

void signal_handler(int signum) {
    running = 0;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    // ... rest of main
}


// Add this function near the top
void show_easter_egg() {
    printf("\n");
    printf("    🎵 You found the secret! 🎵\n");
    printf("    ________________________\n");
    printf("   |  ♫ ♪ ♫ ♪ ♫ ♪ ♫ ♪ ♫   |\n");
    printf("   |  Spotie was created    |\n");
    printf("   |  by music lovers       |\n");
    printf("   |  an for #1 and only 1  |\n");
    printf("   |  ♫ ♪ ♫ ♪ ♫ ♪ ♫ ♪ ♫   |\n");
    printf("    ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\n");
    printf("\n");
}

typedef struct {
    DBusConnection *bus;
    DBusError error;
    char *spotify_service;
    char *spotify_path;
    char *player_interface;
    char *properties_interface;
} SpotifyPlayer;


void check_spotify_daemon() {
    if (access("/usr/bin/spotifyd", F_OK) != 0) {
        char choice;
        printf("Spotify daemon not found\n");
        printf("Would you like to install spotifyd? (y/n): ");
        scanf(" %c", &choice);
        
        if (choice == 'y' || choice == 'Y') {
            if (access("/usr/bin/apt", F_OK) == 0) {
                system("sudo apt update && sudo apt install -y spotifyd");
            }
            else if (access("/usr/bin/pacman", F_OK) == 0) {
                system("sudo pacman -Sy spotifyd");
            }
            else if (access("/usr/local/bin/brew", F_OK) == 0) {
                system("brew install spotifyd");
            }
            else {
                printf("Package manager not detected. Please install spotifyd manually.\n");
                exit(1);
            }
            printf("Spotify daemon installed successfully!\n");
        }
        else {
            printf("Spotify daemon installation skipped\n");
            exit(0);
        }
    }
}
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void spotify_play(SpotifyPlayer *player) {
    DBusMessage *msg = dbus_message_new_method_call(
        player->spotify_service,
        player->spotify_path,
        player->player_interface,
        "Play"
    );
    
    if (!msg) {
        fprintf(stderr, "Failed to create D-Bus message\n");
        return;
    }
    
    dbus_connection_send(player->bus, msg, NULL);
    dbus_message_unref(msg);
    dbus_connection_flush(player->bus);
}

SpotifyPlayer* spotify_player_init() {
    SpotifyPlayer *player = malloc(sizeof(SpotifyPlayer));
    if (!player) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    check_spotify_daemon();

    player->spotify_service = "org.mpris.MediaPlayer2.spotify";
    player->spotify_path = "/org/mpris/MediaPlayer2";
    player->player_interface = "org.mpris.MediaPlayer2.Player";
    player->properties_interface = "org.freedesktop.DBus.Properties";

    dbus_error_init(&player->error);
    player->bus = dbus_bus_get(DBUS_BUS_SESSION, &player->error);

    if (dbus_error_is_set(&player->error)) {
        fprintf(stderr, "D-Bus Connection Error: %s\n", player->error.message);
        dbus_error_free(&player->error);
        free(player);
        return NULL;
    }

    return player;
}

void spotify_player_free(SpotifyPlayer *player) {
    if (player) {
        dbus_connection_unref(player->bus);
        free(player);
    }
}

// Adding new functions for playback control
void spotify_play(SpotifyPlayer *player) {
    DBusMessage *msg = dbus_message_new_method_call(
        player->spotify_service,
        player->spotify_path,
        player->player_interface,
        "Play"
    );
    dbus_connection_send(player->bus, msg, NULL);
    dbus_message_unref(msg);
    dbus_connection_flush(player->bus);
}

void spotify_pause(SpotifyPlayer *player) {
    DBusMessage *msg = dbus_message_new_method_call(
        player->spotify_service,
        player->spotify_path,
        player->player_interface,
        "Pause"
    );
    dbus_connection_send(player->bus, msg, NULL);
    dbus_message_unref(msg);
    dbus_connection_flush(player->bus);
}

void spotify_next(SpotifyPlayer *player) {
    DBusMessage *msg = dbus_message_new_method_call(
        player->spotify_service,
        player->spotify_path,
        player->player_interface,
        "Next"
    );
    dbus_connection_send(player->bus, msg, NULL);
    dbus_message_unref(msg);
    dbus_connection_flush(player->bus);
}

void spotify_previous(SpotifyPlayer *player) {
    DBusMessage *msg = dbus_message_new_method_call(
        player->spotify_service,
        player->spotify_path,
        player->player_interface,
        "Previous"
    );
    dbus_connection_send(player->bus, msg, NULL);
    dbus_message_unref(msg);
    dbus_connection_flush(player->bus);
}

char* get_current_track(SpotifyPlayer *player) {
    DBusMessage *msg = dbus_message_new_method_call(
        player->spotify_service,
        player->spotify_path,
        player->properties_interface,
        "Get"
    );
    
    const char *interface_name = player->player_interface;
    const char *property_name = "Metadata";
    
    dbus_message_append_args(msg,
        DBUS_TYPE_STRING, &interface_name,
        DBUS_TYPE_STRING, &property_name,
        DBUS_TYPE_INVALID);

    DBusMessage *reply;
    reply = dbus_connection_send_with_reply_and_block(player->bus, msg, -1, &player->error);
    
    DBusMessageIter variant, array, dict;
    char *track_name = NULL;
    
    if (reply) {
        DBusMessageIter args;
        dbus_message_iter_init(reply, &args);
        
        if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_VARIANT) {
            dbus_message_iter_recurse(&args, &variant);
            if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY) {
                dbus_message_iter_recurse(&variant, &array);
                while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_DICT_ENTRY) {
                    DBusMessageIter entry;
                    dbus_message_iter_recurse(&array, &entry);
                    char *key;
                    dbus_message_iter_get_basic(&entry, &key);
                    if (strcmp(key, "xesam:title") == 0) {
                        dbus_message_iter_next(&entry);
                        dbus_message_iter_recurse(&entry, &dict);
                        dbus_message_iter_get_basic(&dict, &track_name);
                        break;
                    }
                    dbus_message_iter_next(&array);
                }
            }
        }
        dbus_message_unref(reply);
    }
    
    dbus_message_unref(msg);
    return track_name ? strdup(track_name) : strdup("Unknown Track");
}
  void print_help() {
      printf("Spotie - Command Line Spotify Controller\n\n");
      printf("Usage: spotie [OPTIONS] COMMAND\n\n");
      printf("Options:\n");
      printf("  -h, --help     Show this help message\n");
      printf("  -v, --version  Display version information\n");
      printf("  -i, --interactive  Start interactive mode\n\n");
      printf("Commands:\n");
      printf("  play           Start playback\n");
      printf("  pause          Pause playback\n");
      printf("  next           Skip to next track\n");
      printf("  prev           Go to previous track\n");
      printf("  status         Show current playback status\n\n");
      printf("Interactive Mode Commands:\n");
      printf("  p              Play/Pause toggle\n");
      printf("  n              Next track\n");
      printf("  b              Previous track\n");
      printf("  q              Quit\n\n");
      printf("Examples:\n");
      printf("  spotie play\n");
      printf("  spotie --interactive\n");
      printf("  spotie status\n");
  }
  // In the main command parser, add a hidden command
if (strcmp(command, "42") == 0 || strcmp(command, "theanswer") == 0) {
    show_easter_egg();
    // Let's also play a specific song as part of the easter egg
    // Rick Roll them or play your favorite track
    spotify_play_uri(player, "spotify:track:4cOdK2wGLETKBW3PvgPWqT");  // Never Gonna Give You Up
    return 0;
}

  int start_interactive_mode() {
      SpotifyPlayer *player = spotify_player_init();
      if (!player) {
          return 1;
      }

      char cmd;
      printf("Spotify Controller\n");
      printf("Commands: p (play/pause), n (next), b (previous), q (quit)\n");

      while(1) {
          printf("> ");
          scanf(" %c", &cmd);

          switch(cmd) {
              case 'p':
                  spotify_play(player);
                  break;
              case 'n':
                  spotify_next(player);
                  break;
              case 'b':
                  spotify_previous(player);
                  break;
              case 'q':
                  goto cleanup;
              default:
                  printf("Unknown command\n");
          }

          char *current_track = get_current_track(player);
          printf("Now playing: %s\n", current_track);
      }

  cleanup:
      spotify_player_free(player);
      return 0;
  }

  int main(int argc, char *argv[]) {
      static struct option long_options[] = {
          {"help", no_argument, 0, 'h'},
          {"version", no_argument, 0, 'v'},
          {"interactive", no_argument, 0, 'i'},
          {0, 0, 0, 0}
      };

      int option_index = 0;
      int opt;

      while ((opt = getopt_long(argc, argv, "hvi", long_options, &option_index)) != -1) {
          switch (opt) {
              case 'h':
                  print_help();
                  return 0;
              case 'v':
                  printf("Spotie v1.0.0\n");
                  return 0;
              case 'i':
                  // Start interactive mode
                  return start_interactive_mode();
          }
      }

      if (optind < argc) {
          SpotifyPlayer *player = spotify_player_init();
          if (!player) return 1;

          const char *command = argv[optind];
          if (strcmp(command, "play") == 0) spotify_play(player);
          else if (strcmp(command, "pause") == 0) spotify_pause(player);
          else if (strcmp(command, "next") == 0) spotify_next(player);
          else if (strcmp(command, "prev") == 0) spotify_previous(player);
          else if (strcmp(command, "status") == 0) {
              char *current_track = get_current_track(player);
              printf("Now playing: %s\n", current_track);
          }
          else {
              printf("Unknown command. Use --help for usage information.\n");
              return 1;
          }

          spotify_player_free(player);
      }

      return 0;
  }




