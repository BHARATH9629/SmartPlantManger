#ifndef UI_H
#define UI_H

/* ============================================================
   ui.h  –  Console UI helpers
   Replaces Java Swing: JOptionPane, JList, JTextArea, JProgressBar
   ============================================================ */

#include "plant.h"

/* Dashboard bar – replaces the top JPanel with aliveLbl/needsLbl/deadLbl */
void ui_print_dashboard(int alive, int needs, int dead);

/* Progress-bar replacement (text) */
void ui_print_water_bar(long minutes_left, int dead);

/* List all plants in catalogue with index numbers */
void ui_list_plants(const PlantList *pl);

/* Show full detail of one plant – replaces the JTextArea details pane */
void ui_show_plant_detail(const Plant *p);

/* List garden plants with status icons – replaces GardenCellRenderer */
void ui_list_garden(const GardenList *gl, const FavSet *favs);

/* Text-based pie chart – replaces the Swing paintComponent pie chart */
void ui_show_chart(int alive, int needs, int dead);

/* Pause and wait for Enter */
void ui_pause(void);

/* Clear screen (cross-platform) */
void ui_clear(void);

/* Safe integer input with prompt */
int ui_read_int(const char *prompt);

/* Safe string input with prompt */
void ui_read_str(const char *prompt, char *buf, int maxlen);

#endif /* UI_H */