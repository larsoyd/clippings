#pragma once

#include <QObject>

class CheckCore : public QObject
{
    Q_OBJECT

  private slots:
    void compose_copies_exact_pixels();
    void compose_crosses_mixed_scales_and_negative_origin();
    void compose_leaves_gaps_clear();
    void compose_outside_every_display_is_empty();
    void output_lookup_honours_negative_layout();

    void annotations_stay_in_source_pixels();
    void arrow_points_at_its_tip();
    void redaction_removes_the_covered_pixels();
    void redaction_clears_colour_under_transparency();
    void pixelate_and_blur_only_touch_their_area();
    void export_equals_what_the_view_shows();
    void covers_read_earlier_annotations();
    void crop_keeps_annotation_positions();
    void text_grows_the_picture();

    void undo_and_redo_walk_states();
    void new_edit_drops_redo();
    void save_clears_modified_and_undo_sets_it();
    void lost_save_point_stays_modified();
    void history_stops_growing();
    void restyling_selection_is_one_step();
    void selected_follows_removal();
    void hit_finds_topmost();
    void hollow_shapes_ignore_their_middle();

    void write_refuses_unknown_format();
    void failed_write_keeps_old_file();
    void jpeg_flattens_on_white();
    void png_round_trips_alpha();
    void copy_reports_missing_tool();
    void copy_reports_failing_tool();
    void copy_sends_png();
};
