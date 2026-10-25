/* ============================================================
   main.c  –  Smart Plant Manager (C console version)

   Replaces: PlantAnalyzer_GUI.java (the entire Swing application)

   Panels mapped to menu screens:
     HomePanel      -> main_menu()
     PlantInfoPanel -> plant_info_menu()
     MyGardenPanel  -> garden_menu()

   Java features that don't exist in C and how we replaced them:
     JFrame / Swing GUI  -> printf-based menus
     CardLayout          -> function calls switching screens
     Timer / TimerTask   -> dao_auto_kill_expired() called before each garden view
     ArrayList           -> PlantList / GardenList (fixed-size arrays + count)
     HashSet<Integer>    -> FavSet struct (small int array)
     System.out / JOptionPane -> printf / scanf (via ui_read_*)
     Pie chart           -> text bar chart in ui_show_chart()
     try-with-resources  -> manual if(!conn) checks in dao.c
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "plant.h"
#include "dao.h"
#include "ui.h"
#include "env.h"

/* ---- global in-memory favourite set (replaces Java HashSet<Integer>) ---- */
static FavSet g_favs = { .count = 0 };

/* ---- helper: is id in favset? ---- */
static int fav_contains(int id)
{
    for (int i = 0; i < g_favs.count; i++)
        if (g_favs.ids[i] == id) return 1;
    return 0;
}

/* ---- helper: toggle favourite ---- */
static void fav_toggle(int id)
{
    for (int i = 0; i < g_favs.count; i++) {
        if (g_favs.ids[i] == id) {
            /* remove – shift left */
            for (int j = i; j < g_favs.count - 1; j++)
                g_favs.ids[j] = g_favs.ids[j+1];
            g_favs.count--;
            printf("  Removed from favourites.\n");
            return;
        }
    }
    if (g_favs.count < MAX_GARDEN) {
        g_favs.ids[g_favs.count++] = id;
        printf("  Added to favourites.\n");
    }
}

/* ---- count alive / needs / dead ---- */
static void count_status(const GardenList *gl, int *alive, int *needs, int *dead)
{
    *alive = *needs = *dead = 0;
    for (int i = 0; i < gl->count; i++) {
        const GardenPlant *gp = &gl->items[i];
        if (gp->dead)               (*dead)++;
        else if (gp->minutesLeft==0)(*needs)++;
        else                         (*alive)++;
    }
}

/* ===========================================================
   SCREEN 1: Plant Info Panel
   Replaces: PlantInfoPanel (JList + JTextArea split pane)
   =========================================================== */
static void plant_info_menu(void)
{
    ui_clear();
    PlantList pl = dao_get_all_plants();

    while (1) {
        ui_clear();
        printf("  ============ PLANT INFO ============\n");
        ui_list_plants(&pl);
        printf("\n  Enter plant number for details (0 = Back): ");
        int choice = ui_read_int("");
        if (choice == 0) return;
        if (choice < 1 || choice > pl.count) {
            printf("  Invalid choice.\n");
            ui_pause();
            continue;
        }
        ui_clear();
        ui_show_plant_detail(&pl.items[choice - 1]);
        ui_pause();
    }
}

/* ===========================================================
   HELPER: export report to file
   Replaces: MyGardenPanel.exportReport()
   =========================================================== */
static void export_report(const GardenList *gl)
{
    FILE *f = fopen("garden_report.txt", "w");
    if (!f) { printf("  ERROR: Cannot write report file.\n"); return; }

    fprintf(f, "SMART PLANT MANAGER REPORT\n");
    fprintf(f, "Generated: ");

    time_t now = time(NULL);
    char tbuf[64];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(f, "%s\n\n", tbuf);

    fprintf(f, "%-30s  %-14s  %-20s\n", "Plant Name", "Status", "Last Watered");
    fprintf(f, "%s\n", "-------------------------------------------------------------");

    for (int i = 0; i < gl->count; i++) {
        const GardenPlant *gp = &gl->items[i];
        const char *status = gp->dead           ? "DEAD"
                           : gp->minutesLeft==0 ? "NEEDS WATER"
                                                : "ALIVE";
        fprintf(f, "%-30s  %-14s  %-20s\n",
                gp->name, status, gp->lastWatered);
    }

    fclose(f);
    printf("  Report saved to: garden_report.txt\n");
}

/* ===========================================================
   SCREEN 2: My Garden Panel
   Replaces: MyGardenPanel (the main garden management screen)
   =========================================================== */
static void garden_menu(void)
{
    int filter = 0; /* 0=All, 1=Alive, 2=NeedsWater, 3=Dead */
    const char *filter_names[] = { "All", "Alive", "Needs Water", "Dead" };

    while (1) {
        ui_clear();

        /* auto-kill happens inside dao_get_garden_plants() */
        GardenList gl = dao_get_garden_plants();

        /* apply filter */
        GardenList filtered;
        filtered.count = 0;
        for (int i = 0; i < gl.count; i++) {
            GardenPlant *gp = &gl.items[i];
            int show = 0;
            switch (filter) {
                case 0: show = 1; break;
                case 1: show = !gp->dead && gp->minutesLeft > 0; break;
                case 2: show = !gp->dead && gp->minutesLeft == 0; break;
                case 3: show = gp->dead; break;
            }
            if (show && filtered.count < MAX_GARDEN)
                filtered.items[filtered.count++] = *gp;
        }

        /* mark favourites */
        for (int i = 0; i < filtered.count; i++)
            filtered.items[i].favorite = fav_contains(filtered.items[i].gardenId);

        int alive, needs, dead;
        count_status(&gl, &alive, &needs, &dead);

        ui_print_dashboard(alive, needs, dead);
        printf("  Filter: [%s]\n\n", filter_names[filter]);
        ui_list_garden(&filtered, &g_favs);

        printf("\n  ============ GARDEN MENU ============\n");
        printf("  1. Add Plant\n");
        printf("  2. Water Plant\n");
        printf("  3. Remove Plant\n");
        printf("  4. Toggle Favourite\n");
        printf("  5. Export Report\n");
        printf("  6. Show Statistics Chart\n");
        printf("  7. Change Filter\n");
        printf("  0. Back to Home\n");
        printf("  =====================================\n");

        int choice = ui_read_int("  Choose: ");

        switch (choice) {

        /* ---------- ADD PLANT ----------
           Replaces: addPlant() -> JOptionPane.showInputDialog with plant list */
        case 1: {
            ui_clear();
            PlantList pl = dao_get_all_plants();
            ui_list_plants(&pl);
            int idx = ui_read_int("\n  Enter plant number to add (0=cancel): ");
            if (idx >= 1 && idx <= pl.count) {
                dao_add_to_garden(pl.items[idx-1].name);
                ui_pause();
            }
            break;
        }

        /* ---------- WATER PLANT ----------
           Replaces: water() -> checks dead, updates DB */
        case 2: {
            if (filtered.count == 0) { printf("  No plants to water.\n"); ui_pause(); break; }
            int idx = ui_read_int("  Enter list number to water (0=cancel): ");
            if (idx >= 1 && idx <= filtered.count) {
                GardenPlant *gp = &filtered.items[idx-1];
                if (gp->dead) {
                    printf("  This plant is dead. Cannot water.\n");
                } else {
                    dao_water(gp->gardenId);
                }
                ui_pause();
            }
            break;
        }

        /* ---------- REMOVE PLANT ----------
           Replaces: remove() -> JOptionPane.showConfirmDialog */
        case 3: {
            if (filtered.count == 0) { printf("  No plants to remove.\n"); ui_pause(); break; }
            int idx = ui_read_int("  Enter list number to remove (0=cancel): ");
            if (idx >= 1 && idx <= filtered.count) {
                GardenPlant *gp = &filtered.items[idx-1];
                printf("  Remove '%s'? (y/n): ", gp->name);
                char confirm[4];
                ui_read_str("", confirm, sizeof(confirm));
                if (confirm[0] == 'y' || confirm[0] == 'Y') {
                    dao_remove(gp->gardenId);
                    ui_pause();
                }
            }
            break;
        }

        /* ---------- TOGGLE FAVOURITE ----------
           Replaces: toggleFavorite() -> HashSet add/remove */
        case 4: {
            if (filtered.count == 0) { printf("  No plants.\n"); ui_pause(); break; }
            int idx = ui_read_int("  Enter list number to toggle favourite (0=cancel): ");
            if (idx >= 1 && idx <= filtered.count) {
                fav_toggle(filtered.items[idx-1].gardenId);
                ui_pause();
            }
            break;
        }

        /* ---------- EXPORT REPORT ----------
           Replaces: exportReport() -> PrintWriter to file */
        case 5:
            export_report(&gl);
            ui_pause();
            break;

        /* ---------- STATISTICS CHART ----------
           Replaces: showChart() -> Swing pie chart dialog */
        case 6:
            ui_clear();
            ui_show_chart(alive, needs, dead);
            ui_pause();
            break;

        /* ---------- CHANGE FILTER ----------
           Replaces: JComboBox filter dropdown */
        case 7:
            printf("\n  Filter options:\n");
            printf("  0. All\n  1. Alive\n  2. Needs Water\n  3. Dead\n");
            int f = ui_read_int("  Choose filter: ");
            if (f >= 0 && f <= 3) filter = f;
            break;

        case 0:
            return;

        default:
            printf("  Invalid option.\n");
            ui_pause();
        }
    }
}

/* ===========================================================
   SCREEN 0: Home / Login
   Replaces: HomePanel + main() login dialog
   =========================================================== */
static int login(void)
{
    ui_clear();
    printf("\n\n");
    printf("  +----------------------------------------------+\n");
    printf("  |       SMART PLANT MANAGER  –  LOGIN          |\n");
    printf("  +----------------------------------------------+\n\n");

    char user[64], pass[64];
    ui_read_str("  Username: ", user, sizeof(user));
    ui_read_str("  Password: ", pass, sizeof(pass));

    /* Same credentials as the Java version: admin / password */
    if (strcmp(user, "admin") == 0 && strcmp(pass, "password") == 0) {
        printf("\n  Login successful!\n");
        return 1;
    }
    printf("\n  Invalid credentials. Use admin / password\n");
    return 0;
}

static void main_menu(void)
{
    while (1) {
        ui_clear();

        /* Show live dashboard counts */
        GardenList gl = dao_get_garden_plants();
        int alive, needs, dead;
        count_status(&gl, &alive, &needs, &dead);
        ui_print_dashboard(alive, needs, dead);

        printf("  ============ HOME MENU ============\n");
        printf("  1. Plant Info\n");
        printf("  2. My Garden\n");
        printf("  0. Exit\n");
        printf("  ===================================\n");

        int choice = ui_read_int("  Choose: ");
        switch (choice) {
            case 1: plant_info_menu(); break;
            case 2: garden_menu();     break;
            case 0:
                printf("\n  Goodbye! Happy gardening!\n\n");
                return;
            default:
                printf("  Invalid option.\n");
                ui_pause();
        }
    }
}

/* ===========================================================
   main()
   =========================================================== */
int main(void)
{
    /* Load environment configuration from .env file */
    load_env(".env");

    /* ---- Login ---- */
    if (!login()) {
        ui_pause();
        return 1;
    }

    /* ---- Connect to MySQL using environment credentials ---- */
    if (!dao_connect(get_db_host(), get_db_user(), get_db_pass(), get_db_name())) {
        printf("\n  Could not connect to database '%s' at %s.\n", get_db_name(), get_db_host());
        printf("  Make sure MySQL is running and check your .env configuration.\n");
        ui_pause();
        return 1;
    }

    /* ---- Run the app ---- */
    main_menu();

    /* ---- Cleanup ---- */
    dao_close();
    return 0;
}