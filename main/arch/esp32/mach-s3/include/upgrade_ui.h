#ifndef UPGRADE_UI_H
#define UPGRADE_UI_H

/**
 * @brief Show the full-screen upgrade progress UI.
 *
 * Must be called after LVGL is initialized.
 */
void upgrade_ui_show(void);

/**
 * @brief Update the progress bar and stage label.
 *
 * Call this periodically during the upgrade to refresh the display.
 * Also calls lv_timer_handler() to flush the display.
 *
 * @param percent  0–100 for normal progress; -1 for error
 * @param stage    Short text e.g. "Writing firmware..." or error message
 */
void upgrade_ui_update(int percent, const char *stage);

/**
 * @brief Close (delete) the upgrade UI.
 */
void upgrade_ui_close(void);

#endif /* UPGRADE_UI_H */
