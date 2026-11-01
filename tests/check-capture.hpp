#pragma once

#include <QObject>

class CheckCapture : public QObject
{
    Q_OBJECT

  private slots:

    void outputs_filter_and_stack_windows();
    void hidden_workspaces_are_left_out();

    void grab_collects_every_display();
    void grab_reports_a_failing_tool();
    void grab_gives_up_on_a_slow_tool();
    void grab_rejects_garbage();
    void settle_waits_for_two_equal_pictures();
    void settle_keeps_its_deadline();

    void capture_freezes_before_showing_anything();
    void second_begin_is_refused_and_changes_nothing();
    void drag_selects_and_accept_composes();
    void selection_crosses_displays();
    void small_drag_chooses_nothing();
    void empty_selection_does_not_accept();
    void selection_moves_and_resizes();
    void keyboard_nudges_selection_inside_desktop();
    void window_mode_uses_the_window_picture();
    void window_mode_falls_back_to_the_frozen_screen();
    void screen_and_all_modes();
    void cancel_restores_focus_and_gives_nothing();
    void failed_grab_ends_the_capture();
    void display_change_ends_the_capture();
    void leaving_out_own_windows_waits_for_them();
    void own_windows_return_after_cancel();
    void countdown_grabs_a_fresh_picture();
    void cancel_during_countdown_grabs_nothing();

    void door_hands_over_and_answers();
    void door_without_listener_refuses();
    void door_replaces_a_dead_socket();
    void door_keeps_a_live_socket();
    void start_lock_excludes_a_second_holder();
};
