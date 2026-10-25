/* ============================================================
   ui.c  –  Console UI implementation
   Every function here replaces a Swing widget or dialog.
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"
#include "plant.h"

/* ---- platform clear-screen ---- */
void ui_clear(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* ---- wait for Enter ---- */
void ui_pause(void)
{
    printf("\n  Press ENTER to continue...");
    fflush(stdout);
    /* flush any leftover newline, then wait */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ---- safe integer read ---- */
int ui_read_int(const char *prompt)
{
    char buf[64];
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return -1;
    return atoi(buf);
}

/* ---- safe string read ---- */
void ui_read_str(const char *prompt, char *buf, int maxlen)
{
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, maxlen, stdin)) { buf[0] = '\0'; return; }
    /* strip trailing newline */
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
}

/* -----------------------------------------------------------
   ui_print_dashboard()
   Replaces: the top JPanel with aliveLbl / needsLbl / deadLbl
   ----------------------------------------------------------- */
void ui_print_dashboard(int alive, int needs, int dead)
{
    printf("\n");
    printf("  +--------------------------------------------------+\n");
    printf("  |       *** SMART PLANT MANAGER ***                |\n");
    printf("  +--------------------------------------------------+\n");
    printf("  |  Alive: %-5d  Needs Water: %-5d  Dead: %-5d   |\n",
           alive, needs, dead);
    printf("  +--------------------------------------------------+\n");

    if (needs > 0)
        printf("  |  !! %d plant(s) need water urgently !           |\n", needs);
    else if (dead > 0)
        printf("  |  Some plants died. Consider replacing them.     |\n");
    else if (alive > 0)
        printf("  |  Garden is healthy!                             |\n");

    printf("  +--------------------------------------------------+\n\n");
}

/* -----------------------------------------------------------
   ui_print_water_bar()
   Replaces: JProgressBar (0-900 minutes range)
   Shows a text progress bar of water remaining.
   ----------------------------------------------------------- */
void ui_print_water_bar(long minutes_left, int dead)
{
    if (dead) {
        printf("    [####################] DEAD\n");
        return;
    }

    int total_bars = 20;
    /* scale 0-900 min to 0-20 bars */
    long capped = minutes_left > 900 ? 900 : minutes_left;
    int filled = (int)(capped * total_bars / 900);

    printf("    [");
    for (int i = 0; i < total_bars; i++)
        printf(i < filled ? "#" : "-");
    printf("] %ld min left\n", minutes_left);
}

/* -----------------------------------------------------------
   ui_list_plants()
   Replaces: JList<Plant> in PlantInfoPanel
   ----------------------------------------------------------- */
void ui_list_plants(const PlantList *pl)
{
    printf("\n  %-4s  %-30s  %-15s\n", "No.", "Plant Name", "Type");
    printf("  %s\n", "----  ------------------------------  ---------------");
    for (int i = 0; i < pl->count; i++) {
        printf("  %-4d  %-30s  %-15s\n",
               i + 1,
               pl->items[i].name,
               pl->items[i].plantType);
    }
}

/* -----------------------------------------------------------
   ui_show_plant_detail()
   Replaces: JTextArea details pane in PlantInfoPanel
   ----------------------------------------------------------- */
void ui_show_plant_detail(const Plant *p)
{
    printf("\n  +--------------------------------------------+\n");
    printf("  | PLANT DETAILS                              |\n");
    printf("  +--------------------------------------------+\n");
    printf("  | Name           : %-25s |\n", p->name);
    printf("  | Scientific Name: %-25s |\n", p->scientificName);
    printf("  | Type           : %-25s |\n", p->plantType);
    printf("  | Soil Type      : %-25s |\n", p->soilType);
    printf("  | Water Required : %-3d ml                    |\n", p->waterMl);
    printf("  | Water Interval : %-3d hrs                   |\n", p->waterIntervalHours);
    printf("  | Lifespan       : %-3d hrs                   |\n", p->lifespanHours);
    printf("  +--------------------------------------------+\n");
}

/* -----------------------------------------------------------
   ui_list_garden()
   Replaces: JList<GardenPlant> with GardenCellRenderer
   Shows status icons, favourites star, and water bar.
   ----------------------------------------------------------- */
void ui_list_garden(const GardenList *gl, const FavSet *favs)
{
    if (gl->count == 0) {
        printf("  (Garden is empty. Add plants from the menu.)\n");
        return;
    }

    printf("\n  %-4s  %-4s  %-28s  %-14s\n",
           "No.", " ID ", "Plant Name", "Status");
    printf("  %s\n",
           "----  ----  ----------------------------  --------------");

    for (int i = 0; i < gl->count; i++) {
        const GardenPlant *gp = &gl->items[i];

        /* check if favourite */
        int is_fav = 0;
        for (int f = 0; f < favs->count; f++)
            if (favs->ids[f] == gp->gardenId) { is_fav = 1; break; }

        const char *star   = is_fav  ? "[*]" : "   ";
        const char *icon   = gp->dead           ? "[X]"
                           : gp->minutesLeft==0 ? "[!]"
                                                : "[~]";
        const char *status = gp->dead           ? "DEAD"
                           : gp->minutesLeft==0 ? "NEEDS WATER"
                                                : "ALIVE";

        printf("  %-4d  %-4d  %s%s %-24s  %-14s\n",
               i + 1, gp->gardenId, star, icon, gp->name, status);

        ui_print_water_bar(gp->minutesLeft, gp->dead);
    }
    printf("\n  Legend: [~]=Alive  [!]=Needs Water  [X]=Dead  [*]=Favourite\n");
}

/* -----------------------------------------------------------
   ui_show_chart()
   Replaces: Swing pie chart (paintComponent arc drawing)
   Draws a text-based bar chart / percentage breakdown.
   ----------------------------------------------------------- */
void ui_show_chart(int alive, int needs, int dead)
{
    int total = alive + needs + dead;
    if (total == 0) total = 1;

    int pct_alive = alive * 100 / total;
    int pct_needs = needs * 100 / total;
    int pct_dead  = dead  * 100 / total;

    int bar_width = 40;
    int bars_alive = alive * bar_width / total;
    int bars_needs = needs * bar_width / total;
    int bars_dead  = dead  * bar_width / total;

    printf("\n  ============ GARDEN STATISTICS ============\n\n");

    /* Alive bar */
    printf("  ALIVE       [");
    for (int i = 0; i < bar_width; i++)
        printf(i < bars_alive ? "#" : " ");
    printf("] %3d%% (%d)\n", pct_alive, alive);

    /* Needs Water bar */
    printf("  NEEDS WATER [");
    for (int i = 0; i < bar_width; i++)
        printf(i < bars_needs ? "~" : " ");
    printf("] %3d%% (%d)\n", pct_needs, needs);

    /* Dead bar */
    printf("  DEAD        [");
    for (int i = 0; i < bar_width; i++)
        printf(i < bars_dead ? "X" : " ");
    printf("] %3d%% (%d)\n\n", pct_dead, dead);

    /* ASCII "pie" – simple text donut */
    printf("  ASCII Pie:\n");
    printf("      Alive       : ");
    for (int i = 0; i < bars_alive; i++) printf("O");
    printf("\n");
    printf("      Needs Water : ");
    for (int i = 0; i < bars_needs; i++) printf("~");
    printf("\n");
    printf("      Dead        : ");
    for (int i = 0; i < bars_dead; i++) printf("X");
    printf("\n");

    printf("\n  Total plants in garden: %d\n", total);
    printf("  ==========================================\n");
}