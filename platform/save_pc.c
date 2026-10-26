#ifdef PLATFORM_PC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "gba_mem.h"
#include "save_pc.h"

/*
 * The save lives next to the other per-user data SDL knows about:
 *   macOS   ~/Library/Application Support/Rhythm Tengoku/rhythmtengoku.sav
 *   Windows %APPDATA%\Rhythm Tengoku\rhythmtengoku.sav
 *   Linux   ~/.local/share/Rhythm Tengoku/rhythmtengoku.sav
 * SDL_GetPrefPath creates the directory if it is missing.
 */
#define SAVE_FILE_NAME "rhythmtengoku.sav"

static char s_path[1024];
static int  s_dirty;
static int  s_path_failed;

static const char *save_path(void)
{
    const char *override;
    char *pref;

    if (s_path[0] != '\0') return s_path;
    if (s_path_failed) return NULL;

    /* RTPC_SAVE=<path> puts the save somewhere else. Testing drives the game
       with synthetic input (RTPC_AUTO), which can wander into menus and change
       real settings, so automated runs must not be pointed at the player's own
       save file. RTPC_SAVE=/dev/null disables saving entirely. */
    override = getenv("RTPC_SAVE");
    if (override != NULL && override[0] != '\0') {
        if (SDL_snprintf(s_path, sizeof(s_path), "%s", override) >= (int)sizeof(s_path)) {
            fprintf(stderr, "[SAVE] RTPC_SAVE path too long; saves will not persist\n");
            s_path[0] = '\0';
            s_path_failed = 1;
            return NULL;
        }
        fprintf(stderr, "[SAVE] using %s (RTPC_SAVE)\n", s_path);
        return s_path;
    }

    pref = SDL_GetPrefPath(NULL, "Rhythm Tengoku");
    if (pref == NULL) {
        fprintf(stderr, "[SAVE] SDL_GetPrefPath failed (%s); saves will not persist\n",
                SDL_GetError());
        s_path_failed = 1;
        return NULL;
    }
    if (SDL_snprintf(s_path, sizeof(s_path), "%s%s", pref, SAVE_FILE_NAME) >= (int)sizeof(s_path)) {
        fprintf(stderr, "[SAVE] save path too long; saves will not persist\n");
        s_path[0] = '\0';
        s_path_failed = 1;
        SDL_free(pref);
        return NULL;
    }
    SDL_free(pref);
    return s_path;
}

void rtpc_sram_load(void)
{
    const char *path = save_path();
    FILE *f;
    size_t got;

    s_dirty = 0;
    if (path == NULL) return;

    f = fopen(path, "rb");
    if (f == NULL) return;   // no save yet: gba_sram stays zeroed

    got = fread(gba_sram, 1, sizeof(gba_sram), f);
    fclose(f);

    // A short file is not fatal - the game checksums its own save buffer and
    // will treat a bad one as empty - but say so, because a truncated save is
    // usually a sign something went wrong on the last write.
    if (got != sizeof(gba_sram)) {
        fprintf(stderr, "[SAVE] %s is %zu bytes, expected %zu\n",
                path, got, sizeof(gba_sram));
    }
}

void rtpc_sram_mark_dirty(void)
{
    s_dirty = 1;
}

void rtpc_sram_flush(void)
{
    const char *path = save_path();
    char tmp[1024];
    FILE *f;

    if (!s_dirty) return;
    if (path == NULL) { s_dirty = 0; return; }

    // Write to a sibling file and rename over the real one, so a crash partway
    // through cannot leave a half-written save behind.
    if (SDL_snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp)) {
        s_dirty = 0;
        return;
    }

    f = fopen(tmp, "wb");
    if (f == NULL) {
        fprintf(stderr, "[SAVE] cannot open %s for writing\n", tmp);
        s_dirty = 0;
        return;
    }
    if (fwrite(gba_sram, 1, sizeof(gba_sram), f) != sizeof(gba_sram)) {
        fprintf(stderr, "[SAVE] short write to %s\n", tmp);
        fclose(f);
        remove(tmp);
        s_dirty = 0;
        return;
    }
    fclose(f);

    // Windows rename() refuses to clobber an existing file.
    remove(path);
    if (rename(tmp, path) != 0) {
        fprintf(stderr, "[SAVE] cannot rename %s to %s\n", tmp, path);
        remove(tmp);
    }
    s_dirty = 0;
}

#endif // PLATFORM_PC
