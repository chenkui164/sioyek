﻿//todo:
// make the rest of config UIs have the same theme as boolean config
// add more epub-related configs
// fix the special case handling for download message in get_status_string

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <optional>
#include <memory>
#include <cctype>


#include <qpainterpath.h>
#include <qabstractitemmodel.h>
#include <qapplication.h>
#include <qboxlayout.h>
#include <qdatetime.h>

#ifndef SIOYEK_QT6
#include <qdesktopwidget.h>
#endif

#include <qkeyevent.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qopenglfunctions.h>
#include <qpushbutton.h>
#include <qsortfilterproxymodel.h>
#include <qstringlistmodel.h>
#include <qtextedit.h>
#include <qtimer.h>
#include <qtreeview.h>
#include <qwindow.h>
#include <qstandardpaths.h>
#include <qprocess.h>
#include <qguiapplication.h>
#include <qmimedata.h>
#include <qscreen.h>
#include <QGestureEvent>
#include <qjsonarray.h>
#include <qjsonobject.h>


#include "input.h"
#include "database.h"
#include "book.h"
#include "utils.h"
#include "ui.h"
#include "pdf_renderer.h"
#include "document.h"
#include "document_view.h"
#include "pdf_view_opengl_widget.h"
#include "config.h"
#include "utf8.h"
#include "synctex/synctex_parser.h"
#include "path.h"
#include "touchui/TouchMarkSelector.h"

#include "main_widget.h"


extern bool SHOULD_USE_MULTIPLE_MONITORS;
extern bool SORT_BOOKMARKS_BY_LOCATION;
extern bool FLAT_TABLE_OF_CONTENTS;
extern bool HOVER_OVERVIEW;
extern bool WHEEL_ZOOM_ON_CURSOR;
extern float MOVE_SCREEN_PERCENTAGE;
extern std::wstring LIBGEN_ADDRESS;
extern std::wstring INVERSE_SEARCH_COMMAND;
extern std::wstring PAPER_SEARCH_URL;
extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;
extern float SMALL_PIXMAP_SCALE;
extern std::wstring EXECUTE_COMMANDS[26];
extern float HIGHLIGHT_COLORS[26 * 3];
extern int STATUS_BAR_FONT_SIZE;
extern Path default_config_path;
extern Path default_keys_path;
extern std::vector<Path> user_config_paths;
extern std::vector<Path> user_keys_paths;
extern Path database_file_path;
extern Path tutorial_path;
extern Path last_opened_file_address_path;
extern Path auto_config_path;
extern std::wstring ITEM_LIST_PREFIX;
extern std::wstring SEARCH_URLS[26];
extern std::wstring MIDDLE_CLICK_SEARCH_ENGINE;
extern std::wstring SHIFT_MIDDLE_CLICK_SEARCH_ENGINE;
extern float DISPLAY_RESOLUTION_SCALE;
extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern int MAIN_WINDOW_SIZE[2];
extern int HELPER_WINDOW_SIZE[2];
extern int MAIN_WINDOW_MOVE[2];
extern int HELPER_WINDOW_MOVE[2];
extern float TOUCHPAD_SENSITIVITY;
extern int SINGLE_MAIN_WINDOW_SIZE[2];
extern int SINGLE_MAIN_WINDOW_MOVE[2];
extern float OVERVIEW_SIZE[2];
extern float OVERVIEW_OFFSET[2];
extern bool IGNORE_WHITESPACE_IN_PRESENTATION_MODE;
extern std::vector<MainWidget*> windows;
extern bool SHOW_DOC_PATH;
extern bool SINGLE_CLICK_SELECTS_WORDS;
extern std::wstring SHIFT_CLICK_COMMAND;
extern std::wstring CONTROL_CLICK_COMMAND;
extern std::wstring SHIFT_RIGHT_CLICK_COMMAND;
extern std::wstring CONTROL_RIGHT_CLICK_COMMAND;
extern std::wstring ALT_CLICK_COMMAND;
extern std::wstring ALT_RIGHT_CLICK_COMMAND;
extern Path local_database_file_path;
extern Path global_database_file_path;
extern Path downloaded_papers_path;
extern std::map<std::wstring, std::wstring> ADDITIONAL_COMMANDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_MACROS;
extern bool HIGHLIGHT_MIDDLE_CLICK;
extern std::wstring STARTUP_COMMANDS;
extern float CUSTOM_BACKGROUND_COLOR[3];
extern float CUSTOM_TEXT_COLOR[3];
extern float HYPERDRIVE_SPEED_FACTOR;
extern float SMOOTH_SCROLL_SPEED;
extern float SMOOTH_SCROLL_DRAG;
extern bool IGNORE_STATUSBAR_IN_PRESENTATION_MODE;
extern bool SUPER_FAST_SEARCH;
extern bool SHOW_CLOSEST_BOOKMARK_IN_STATUSBAR;
extern bool SHOW_CLOSE_PORTAL_IN_STATUSBAR;
extern bool CASE_SENSITIVE_SEARCH;
extern bool SHOW_DOCUMENT_NAME_IN_STATUSBAR;
extern std::wstring UI_FONT_FACE_NAME;
extern bool SHOULD_HIGHLIGHT_LINKS;
extern float SCROLL_VIEW_SENSITIVITY;
extern std::wstring STATUS_BAR_FORMAT;
extern bool INVERTED_HORIZONTAL_SCROLLING;
extern bool TOC_JUMP_ALIGN_TOP;
extern bool AUTOCENTER_VISUAL_SCROLL;
extern bool ALPHABETIC_LINK_TAGS;
extern bool VIMTEX_WSL_FIX;
extern float RULER_AUTO_MOVE_SENSITIVITY;
extern float TTS_RATE;
extern std::wstring HOLD_MIDDLE_CLICK_COMMAND;

extern UIRect PORTRAIT_EDIT_PORTAL_UI_RECT;
extern UIRect LANDSCAPE_EDIT_PORTAL_UI_RECT;

extern UIRect PORTRAIT_BACK_UI_RECT;
extern UIRect PORTRAIT_FORWARD_UI_RECT;
extern UIRect LANDSCAPE_BACK_UI_RECT;
extern UIRect LANDSCAPE_FORWARD_UI_RECT;

extern UIRect PORTRAIT_VISUAL_MARK_PREV;
extern UIRect PORTRAIT_VISUAL_MARK_NEXT;
extern UIRect LANDSCAPE_VISUAL_MARK_PREV;
extern UIRect LANDSCAPE_VISUAL_MARK_NEXT;
extern bool TOUCH_MODE;

const int MAX_SCROLLBAR = 10000;

const unsigned int INTERVAL_TIME = 200;

bool MainWidget::main_document_view_has_document()
{
    return (main_document_view != nullptr) && (doc() != nullptr);
}

class SelectionIndicator : public QWidget{
public:
    SelectionIndicator(QWidget* parent, bool begin, MainWidget* w, AbsoluteDocumentPos pos) : QWidget(parent) {
        is_begin = begin;
        main_widget = w;
        docpos = main_widget->doc()->absolute_to_page_pos(pos);

        begin_pixmap = QPixmap(":/begin.png");
        end_pixmap = QPixmap(":/end.png");
//        qDebug() << "Indicator created!\n";

//        setAttribute(Qt::WA_StyledBackground);
    }

    void update_pos(){
        WindowPos window_pos = main_widget->main_document_view->document_to_window_pos_in_pixels(docpos);
        if (is_begin){
            move(window_pos.x - width(), window_pos.y - height());
//            main_widget->text_selection_buttons->move(0, window_pos.y);
        }
        else{
            move(window_pos.x, window_pos.y);
        }
    }


    void mousePressEvent(QMouseEvent* mevent) {
        is_dragging = true;
    }

    QPoint get_actual_pos(QPoint pos){
        if (is_begin){
            return pos + QPoint(width(), height() );
        }
        else{
            return pos - QPoint(width() / 2, height() / 2);
        }
    }

    void mouseMoveEvent(QMouseEvent* mouse_event) {
        if (is_dragging){
            QPoint mouse_pos = get_actual_pos(mapToParent(mouse_event->pos()));

            WindowPos window_pos;
            window_pos.x = mouse_pos.x();
            window_pos.y = mouse_pos.y();

            docpos = main_widget->main_document_view->window_to_document_pos(window_pos);
            if (is_begin){
                move(mapToParent(mouse_event->pos()));
            }
            else{
                move(mouse_pos);
            }
            main_widget->update_mobile_selection();
        }
    }

    void mouseReleaseEvent(QMouseEvent* mevent) {
        if (is_dragging){
            QPoint parent_pos = get_actual_pos(mapToParent(mevent->pos()));

            WindowPos window_pos;
            window_pos.x = parent_pos.x();
            window_pos.y = parent_pos.y();

            docpos = main_widget->main_document_view->window_to_document_pos(window_pos);
//            AbsoluteDocumentPos abspos = main_widget->main_document_view->window_to_absolute_document_pos(window_pos);
//            AbsoluteDocumentPos abspos = main_widget->doc()->document_to_absolute_pos(docpos);

//            if (is_begin){
//                main_widget->selection_begin = abspos;
//            }
//            else{
//                main_widget->selection_end = abspos;
//            }

            main_widget->update_mobile_selection();
        }
        is_dragging = false;
    }

    void paintEvent(QPaintEvent* event){
//        WindowPos window_pos = main_widget->get_selected_rect_absolute()
        QPainterPath begin_path;
        begin_path.moveTo(width(), 0);
        begin_path.lineTo(0, 0);
        begin_path.arcTo(0, 0, width(), height(), 90, 180);
        begin_path.closeSubpath();

        QPainterPath end_path;
        end_path.moveTo(0, height());
        end_path.lineTo(width(), height());
        end_path.arcTo(0, 0, width(), height(), 270, 360);
        end_path.closeSubpath();


        QPainter painter(this);
        QPoint pos = this->pos();
        QRect rect;

        QColor red_color = QColor::fromRgb(255, 0, 0);
        painter.setPen(red_color);

        rect.setBottom(0);
        rect.setTop(height());
        rect.setLeft(0);
        rect.setRight(width());

//        painter.fillRect(rect, red_color);
        if (is_begin){
//            painter.fillPath(begin_path, red_color);
            painter.drawPixmap(0, 0, width(), height(), begin_pixmap);
        }
        else{

//            painter.fillPath(end_path, red_color);
            painter.drawPixmap(0, 0, width(), height(), end_pixmap);
        }
    }

    DocumentPos docpos;
private:
    bool is_dragging = false;
    bool is_begin;
    MainWidget* main_widget;
    QPixmap begin_pixmap;
    QPixmap end_pixmap;
};

void MainWidget::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);

    main_window_width = size().width();
    main_window_height = size().height();

    if (main_document_view->get_is_auto_resize_mode()) {
        main_document_view->fit_to_page_width();
		main_document_view->set_offset_y(main_window_height / 2 / main_document_view->get_zoom_level());
    }

    if (text_command_line_edit_container != nullptr) {
        text_command_line_edit_container->move(0, 0);
        text_command_line_edit_container->resize(main_window_width, 30);
    }

    if (status_label != nullptr) {
        int status_bar_height = get_status_bar_height();
        status_label->move(0, main_window_height - status_bar_height);
        status_label->resize(main_window_width, status_bar_height);
        if (should_show_status_label()) {
			status_label->show();
        }
    }

    if ((main_document_view->get_document() != nullptr) && (main_document_view->get_zoom_level() == 0)) {
        main_document_view->fit_to_page_width();
        update_current_history_index();
    }

    if (TOUCH_MODE && (current_widget_stack.size() > 0)){
        for (auto w : current_widget_stack) {
            QCoreApplication::postEvent(w, resize_event->clone());
        }
    }
    if (TOUCH_MODE) {
        if (text_selection_buttons) {
            QCoreApplication::postEvent(text_selection_buttons, resize_event->clone());
        }
        if (search_buttons) {
            QCoreApplication::postEvent(search_buttons, resize_event->clone());
        }
        if (highlight_buttons) {
            QCoreApplication::postEvent(highlight_buttons, resize_event->clone());
        }
    }
}

void MainWidget::set_overview_position(int page, float offset) {
    if (page >= 0) {
        auto abspos = main_document_view->get_document()->document_to_absolute_pos({ page, 0, offset });
		float page_height = main_document_view->get_document()->get_page_height(page);
		opengl_widget->set_overview_page(OverviewState{abspos.y});
		invalidate_render();
    }
}

void MainWidget::set_overview_link(PdfLink link) {

    auto [page, offset_x, offset_y] = parse_uri(mupdf_context, link.uri);
    if (page >= 1) {
        set_overview_position(page - 1, offset_y);
    }
}

void MainWidget::mouseMoveEvent(QMouseEvent* mouse_event) {

    if (is_drawing) {
		WindowPos current_window_pos = { mouse_event->pos().x(), mouse_event->pos().y() };
        AbsoluteDocumentPos mouse_abspos = main_document_view->window_to_absolute_document_pos(current_window_pos);
        FreehandDrawingPoint fdp;
        fdp.pos = mouse_abspos;
        fdp.pressure = 1.0f;
        opengl_widget->current_drawing.points.push_back(fdp);
        validate_render();
        return;
    }

    if (TOUCH_MODE){
        if (selection_begin_indicator){
            selection_begin_indicator->update_pos();
            selection_end_indicator->update_pos();
        }
        update_highlight_buttons_position();
        if (is_pressed){
            update_position_buffer();
        }
        if (was_last_mouse_down_in_ruler_next_rect || was_last_mouse_down_in_ruler_prev_rect){
            WindowPos current_window_pos = { mouse_event->pos().x(), mouse_event->pos().y() };
            int distance = abs(current_window_pos.x - ruler_moving_last_window_pos.x) + abs(current_window_pos.y - ruler_moving_last_window_pos.y);
            ruler_moving_last_window_pos = current_window_pos;
            ruler_moving_distance_traveled += distance;
            int num_next = ruler_moving_distance_traveled / static_cast<int>(std::max(RULER_AUTO_MOVE_SENSITIVITY, 1.0f));
            if (num_next > 0){
                ruler_moving_distance_traveled = 0;
            }

            for (int i = 0; i < num_next; i++){

                if (was_last_mouse_down_in_ruler_next_rect){
                    move_visual_mark_next();
                }
                else{
                    move_visual_mark_prev();
                }

                invalidate_render();
            }
            return;

        }
    }

    if (is_rotated()) {
        // we don't handle mouse events while document is rotated becausae proper handling
        // would increase the code complexity too much to be worth it
        return;
    }

    WindowPos mpos = { mouse_event->pos().x(), mouse_event->pos().y() };

    // if the mouse has moved too much when pressing middle mouse button, we assume that the user wants to drag
    // instead of smart jump
    if (QGuiApplication::mouseButtons() & Qt::MouseButton::MiddleButton) {
        if ((std::abs(mpos.x - last_mouse_down.x) + std::abs(mpos.y - last_mouse_down.y)) > 50) {
            is_dragging = true;
        }
    }

    std::optional<PdfLink> link = {};

    NormalizedWindowPos normal_mpos = main_document_view->window_to_normalized_window_pos(mpos);

    if (rect_select_mode) {
        if (rect_select_begin.has_value()) {
			AbsoluteDocumentPos abspos = main_document_view->window_to_absolute_document_pos(mpos);
			rect_select_end = abspos;
			fz_rect selected_rect;
			selected_rect.x0 = rect_select_begin.value().x;
			selected_rect.y0 = rect_select_begin.value().y;
			selected_rect.x1 = rect_select_end.value().x;
			selected_rect.y1 = rect_select_end.value().y;
			opengl_widget->set_selected_rectangle(selected_rect);

			validate_render();
        }
        return;
    }

    if (overview_resize_data) {
        // if we are resizing overview page, set the selected side of the overview window to the mosue position
        //float offset_diff_x = normal_x - overview_resize_data.value().original_mouse_pos.first;
        //float offset_diff_y = normal_y - overview_resize_data.value().original_mouse_pos.second;
        fvec2 offset_diff = fvec2(normal_mpos) - fvec2(overview_resize_data.value().original_normal_mouse_pos);
        opengl_widget->set_overview_side_pos(
            overview_resize_data.value().side_index,
            overview_resize_data.value().original_rect,
            offset_diff);
        validate_render();
        return;
    }

    if (overview_move_data) {
        fvec2 offset_diff = fvec2(normal_mpos) - fvec2(overview_move_data.value().original_normal_mouse_pos);
        offset_diff[1] = -offset_diff[1];
        fvec2 new_offsets = overview_move_data.value().original_offsets + offset_diff;
        opengl_widget->set_overview_offsets(new_offsets);
        validate_render();
        return;
    }

    if (opengl_widget->is_window_point_in_overview(normal_mpos)) {
        link = doc()->get_link_in_pos(opengl_widget->window_pos_to_overview_pos(normal_mpos));
        if (link) {
			setCursor(Qt::PointingHandCursor);
        }
        else {
			setCursor(Qt::ArrowCursor);
        }
        return;
    }

    if (main_document_view && (link = main_document_view->get_link_in_pos(mpos))) {
        // show hand cursor when hovering over links
        setCursor(Qt::PointingHandCursor);

        // if hover_overview config is set, we show an overview of links while hovering over them
        if (HOVER_OVERVIEW) {
            set_overview_link(link.value());
        }
    }
    else {
        setCursor(Qt::ArrowCursor);
        if (HOVER_OVERVIEW) {
            opengl_widget->set_overview_page({});
//            invalidate_render();
        }
    }

    if (is_dragging) {
        ivec2 diff = ivec2(mpos) - ivec2(last_mouse_down_window_pos);

        fvec2 diff_doc = diff / main_document_view->get_zoom_level();
        if (horizontal_scroll_locked) {
            diff_doc.values[0] = 0;
        }

        main_document_view->set_offsets(last_mouse_down_document_offset.x + diff_doc.x(),
            last_mouse_down_document_offset.y - diff_doc.y());
        validate_render();
    }

    if (is_selecting) {

        // When selecting, we occasionally update selected text
        //todo: maybe have a timer event that handles this periodically
	int msecs_since_last_text_select = last_text_select_time.msecsTo(QTime::currentTime());
	if (msecs_since_last_text_select > 16 || msecs_since_last_text_select < 0) {

            AbsoluteDocumentPos document_pos = main_document_view->window_to_absolute_document_pos(mpos);

            selection_begin = last_mouse_down;
            selection_end = document_pos;
            //fz_point selection_begin = { last_mouse_down.x(), last_mouse_down.y()};
            //fz_point selection_end = { document_x, document_y };

            main_document_view->get_text_selection(selection_begin,
                selection_end,
                is_word_selecting,
                main_document_view->selected_character_rects,
                selected_text);

            validate_render();
            last_text_select_time = QTime::currentTime();
        }
    }
}

void MainWidget::persist() {
    main_document_view->persist();

    // write the address of the current document in a file so that the next time
    // we launch the application, we open this document
    if (main_document_view->get_document()) {
        std::ofstream last_path_file(last_opened_file_address_path.get_path_utf8());

        std::string encoded_file_name_str = utf8_encode(main_document_view->get_document()->get_path());
        last_path_file << encoded_file_name_str.c_str() << std::endl;
        last_path_file.close();
    }
}
void MainWidget::closeEvent(QCloseEvent* close_event) {
    handle_close_event();
}

MainWidget::MainWidget(MainWidget* other) : MainWidget(other->mupdf_context, other->db_manager, other->document_manager, other->config_manager, other->command_manager, other->input_handler, other->checksummer, other->should_quit) {

}

MainWidget::MainWidget(fz_context* mupdf_context,
    DatabaseManager* db_manager,
    DocumentManager* document_manager,
    ConfigManager* config_manager,
    CommandManager* command_manager,
    InputHandler* input_handler,
    CachedChecksummer* checksummer,
    bool* should_quit_ptr,
    QWidget* parent):
    QQuickWidget(parent),
    mupdf_context(mupdf_context),
    db_manager(db_manager),
    document_manager(document_manager),
    config_manager(config_manager),
    input_handler(input_handler),
    checksummer(checksummer),
    should_quit(should_quit_ptr),
    command_manager(command_manager)
{

	//main_widget->quickWindow()->setGraphicsApi(QSGRendererInterface::OpenGL);
    quickWindow()->setGraphicsApi(QSGRendererInterface::OpenGL);
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_DeleteOnClose);


    inverse_search_command = INVERSE_SEARCH_COMMAND;
    if (DISPLAY_RESOLUTION_SCALE <= 0){
#ifdef SIOYEK_QT6
        pdf_renderer = new PdfRenderer(4, should_quit_ptr, mupdf_context, QGuiApplication::primaryScreen()->devicePixelRatio());
#else
        pdf_renderer = new PdfRenderer(4, should_quit_ptr, mupdf_context, QApplication::desktop()->devicePixelRatioF());
#endif
    }
    else {
        pdf_renderer = new PdfRenderer(4, should_quit_ptr, mupdf_context, DISPLAY_RESOLUTION_SCALE);

    }
    pdf_renderer->start_threads();


    main_document_view = new DocumentView(mupdf_context, db_manager, document_manager, config_manager, checksummer);
    opengl_widget = new PdfViewOpenGLWidget(main_document_view, pdf_renderer, config_manager, false, this);

    helper_document_view = new DocumentView(mupdf_context, db_manager, document_manager, config_manager, checksummer);
    helper_opengl_widget = new PdfViewOpenGLWidget(helper_document_view, pdf_renderer, config_manager, true);

#ifndef SIOYEK_ANDROID
    // weird hack, should not be necessary but application crashes without it when toggling window configuration
    helper_opengl_widget->show();
    helper_opengl_widget->hide();
#endif

    status_label = new QLabel(this);
    status_label->setStyleSheet(get_status_stylesheet());
    QFont label_font = QFont(get_font_face_name());
    label_font.setStyleHint(QFont::TypeWriter);
    status_label->setFont(label_font);

    // automatically open the helper window in second monitor
    int num_screens = QGuiApplication::screens().size();

    if ((num_screens > 1) && (HELPER_WINDOW_SIZE[0] > 0) && (SHOULD_USE_MULTIPLE_MONITORS)) {
        apply_window_params_for_two_window_mode();
    }
    else {
        apply_window_params_for_one_window_mode();
    }

    if (helper_opengl_widget){
        helper_opengl_widget->register_on_link_edit_listener([this](OpenedBookState state) {
            this->update_closest_link_with_opened_book_state(state);
        });
    }

    text_command_line_edit_container = new QWidget(this);
    text_command_line_edit_container->setStyleSheet(get_status_stylesheet());

    QHBoxLayout* text_command_line_edit_container_layout = new QHBoxLayout();

    text_command_line_edit_label = new QLabel();
    text_command_line_edit = new QLineEdit();

    text_command_line_edit_label->setFont(QFont(get_font_face_name()));
    text_command_line_edit->setFont(QFont(get_font_face_name()));

    text_command_line_edit_label->setStyleSheet(get_status_stylesheet());
    text_command_line_edit->setStyleSheet(get_status_stylesheet());

    text_command_line_edit_container_layout->addWidget(text_command_line_edit_label);
    text_command_line_edit_container_layout->addWidget(text_command_line_edit);
    text_command_line_edit_container_layout->setContentsMargins(10, 0, 10, 0);

    text_command_line_edit_container->setLayout(text_command_line_edit_container_layout);
    text_command_line_edit_container->hide();

    on_command_done = [&](std::string command_name) {
        bool is_numeric = false;
        int page_number = QString::fromStdString(command_name).toInt(&is_numeric);
        if (is_numeric) {
            if (main_document_view) {
                main_document_view->goto_page(page_number - 1);
            }
        }
        else {
			std::unique_ptr<Command> command = this->command_manager->get_command_with_name(command_name);
			handle_command_types(std::move(command), 0);
        }
    };

    // when pdf renderer's background threads finish rendering a page or find a new search result
    // we need to update the ui
    QObject::connect(pdf_renderer, &PdfRenderer::render_advance, this, &MainWidget::invalidate_render);
    QObject::connect(pdf_renderer, &PdfRenderer::search_advance, this, &MainWidget::invalidate_ui);
    // we check periodically to see if the ui needs updating
    // this is done so that thousands of search results only trigger
    // a few rerenders
    // todo: make interval time configurable
    validation_interval_timer = new QTimer(this);
    validation_interval_timer->setInterval(INTERVAL_TIME);

    QObject::connect(&network_manager, &QNetworkAccessManager::finished, [this](QNetworkReply* reply) {
        if (reply->url().toString().endsWith(".pdf")) {
            QByteArray pdf_data = reply->readAll();
            QString file_name = reply->url().fileName();

            QString path = QString::fromStdWString(downloaded_papers_path.slash(file_name.toStdWString()).get_path());
            QDir dir;
            dir.mkpath(QString::fromStdWString(downloaded_papers_path.get_path()));

            QFile file(path);
            bool opened = file.open(QIODeviceBase::WriteOnly);
            if (opened) {
				file.write(pdf_data);
				file.close();
				MainWidget* new_window = handle_new_window();
				new_window->open_document(path.toStdWString());
            }


        }
        else {
			std::string answer = reply->readAll().toStdString();
			QByteArray json_data = QByteArray::fromStdString(answer);
			QJsonDocument json_doc = QJsonDocument::fromJson(json_data);
			QJsonArray hits = json_doc.object().value("hits").toObject().value("hits").toArray();
			std::vector<std::wstring> hit_titles;
			std::vector<std::wstring> hit_authors;
			std::vector<std::wstring> hit_urls;
			for (int i = 0; i < hits.size(); i++) {
				std::wstring url = hits.at(i).toObject().value("_source").toObject().value("best_pdf_url").toString().toStdWString();

				if (url.size() > 0) {
					hit_titles.push_back(hits.at(i).toObject().value("_source").toObject().value("title").toString().toStdWString());
					hit_urls.push_back(url);
					QJsonArray contributors = hits.at(i).toObject().value("_source").toObject().value("contrib_names").toArray();
					std::wstring contrib_string;
					for (int j = 0; j < contributors.size(); j++) {
						contrib_string += contributors.at(j).toString().toStdWString() + L", ";
					}
					hit_authors.push_back(contrib_string);
				}
			}
			show_download_paper_menu(hit_titles, hit_authors, hit_urls);
        }

        });
    QObject::connect(&tts, &QTextToSpeech::stateChanged, [&](QTextToSpeech::State state) {
        if ((state == QTextToSpeech::Ready) || (state == QTextToSpeech::Error)) {
            if (is_reading) {
                move_visual_mark(1);
                //read_current_line();
                invalidate_render();
            }
		}
        });
    connect(validation_interval_timer, &QTimer::timeout, [&]() {

        if (TOUCH_MODE && selection_begin_indicator){
            selection_begin_indicator->update_pos();
            selection_end_indicator->update_pos();
        }

        if (is_render_invalidated) {
            validate_render();
        }
        else if (is_ui_invalidated) {
            validate_ui();
        }

        if (QGuiApplication::mouseButtons() & Qt::MouseButton::MiddleButton) {
            if ((last_middle_down_time.msecsTo(QTime::currentTime()) > 200) && (!is_dragging)) {
				auto commands = this->command_manager->create_macro_command("", HOLD_MIDDLE_CLICK_COMMAND);
				commands->run(this);
				invalidate_render();
                is_dragging = true;
            }
        }

        // detect if the document file has changed and if so, reload the document
        if (main_document_view != nullptr) {
            Document* doc = nullptr;
            if ((doc = main_document_view->get_document()) != nullptr) {

                // Wait until a safe amount of time has passed since the last time the file was updated on the filesystem
                // this is because LaTeX software frequently puts PDF files in an invalid state while it is being made in
                // multiple passes.

                if ((doc->get_milies_since_last_edit_time() < (2 * INTERVAL_TIME)) &&
                    doc->get_milies_since_last_edit_time() > INTERVAL_TIME &&
                    doc->get_milies_since_last_document_update_time() > doc->get_milies_since_last_edit_time()
                    ) {

                //if (doc->get_milies_since_last_document_update_time() > doc->get_milies_since_last_edit_time()) {
                    doc->reload();
                    pdf_renderer->clear_cache();
                    invalidate_render();
                }
            }
        }
        });
    validation_interval_timer->start();


    scroll_bar = new QScrollBar(this);
    QVBoxLayout* layout = new QVBoxLayout;
    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(opengl_widget);
    hlayout->addWidget(scroll_bar);

    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    opengl_widget->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addLayout(hlayout);
    setLayout(layout);

    scroll_bar->setMinimum(0);
    scroll_bar->setMaximum(MAX_SCROLLBAR);

    scroll_bar->connect(scroll_bar, &QScrollBar::actionTriggered, [this](int action) {
        int value = scroll_bar->value();
        if (main_document_view_has_document()) {
            float offset = doc()->max_y_offset() * value / static_cast<float>(scroll_bar->maximum());
            main_document_view->set_offset_y(offset);
            validate_render();
        }
        });


    scroll_bar->hide();

    if (SHOULD_HIGHLIGHT_LINKS) {
        opengl_widget->set_highlight_links(true, false);
    }

    grabGesture(Qt::TapAndHoldGesture, Qt::DontStartGestureOnChildren);
    grabGesture(Qt::PinchGesture, Qt::DontStartGestureOnChildren | Qt::ReceivePartialGestures);
    QObject::connect((QGuiApplication*)QGuiApplication::instance(), &QGuiApplication::applicationStateChanged, [&](Qt::ApplicationState state){
        if ((state == Qt::ApplicationState::ApplicationSuspended) || (state == Qt::ApplicationState::ApplicationInactive)){
#ifdef SIOYEK_ANDROID
            persist();
#endif
        }
#ifdef SIOYEK_ANDROID
    if (state == Qt::ApplicationState::ApplicationActive){
        if (!pending_intents_checked){
            pending_intents_checked = true;
            check_pending_intents("");
        }
    }
#endif

    });

    search_buttons = new SearchButtons(this);
    search_buttons->hide();
    highlight_buttons = new HighlightButtons(this);
    highlight_buttons->hide();
	text_selection_buttons = new TouchTextSelectionButtons(this);
    text_selection_buttons->hide();

    setMinimumWidth(500);
    setMinimumHeight(200);
    setFocus();
}

MainWidget::~MainWidget() {
    if (is_reading) {
        is_reading = false;
        tts.stop();
    }
    remove_self_from_windows();

    if (windows.size() == 0) {
        *should_quit = true;
		pdf_renderer->join_threads();
    }

    // todo: use a reference counting pointer for document so we can delete main_doc
    // and helper_doc in DocumentView's destructor, not here.
    // ideally this function should just become:
    //		delete main_document_view;
    //		if(helper_document_view != main_document_view) delete helper_document_view;
    if (main_document_view != nullptr) {
        //Document* main_doc = main_document_view->get_document();
        //if (main_doc != nullptr) delete main_doc;

        //Document* helper_doc = nullptr;
        //if (helper_document_view != nullptr) {
        //    helper_doc = helper_document_view->get_document();
        //}
        //if (helper_doc != nullptr && helper_doc != main_doc) delete helper_doc;
    }

    if (helper_document_view != nullptr && helper_document_view != main_document_view) {
        delete helper_document_view;
    }
}

bool MainWidget::is_pending_link_source_filled() {
    return (pending_link && pending_link.value().first);
}

std::wstring MainWidget::get_status_string() {

    QString status_string = QString::fromStdWString(STATUS_BAR_FORMAT);

    if (main_document_view->get_document() == nullptr) return L"";
    std::wstring chapter_name = main_document_view->get_current_chapter_name();

    status_string.replace("%{current_page}", QString::number(get_current_page_number() + 1));
    status_string.replace("%{current_page_label}",  QString::fromStdWString(get_current_page_label()));
    status_string.replace("%{num_pages}", QString::number(main_document_view->get_document()->num_pages()));

    if (chapter_name.size() > 0) {
        status_string.replace("%{chapter_name}", " [ " + QString::fromStdWString(chapter_name) + " ] ");
    }

    if (SHOW_DOCUMENT_NAME_IN_STATUSBAR) {
        std::optional<std::wstring> file_name = Path(main_document_view->get_document()->get_path()).filename();
        if (file_name) {
            status_string.replace("%{document_name}", " [ " + QString::fromStdWString(file_name.value()) + " ] ");
        }
    }

    int num_search_results = opengl_widget->get_num_search_results();
    float progress = -1;
    if (should_show_status_label()) {
        // Make sure statusbar is visible if we are searching
        if (!status_label->isVisible()) {
            status_label->show();
        }

        // show the 0th result if there are no results and the index + 1 otherwise
        if (opengl_widget->get_is_searching(&progress)) {

            int result_index = opengl_widget->get_num_search_results() > 0 ? opengl_widget->get_current_search_result_index() + 1 : 0;
            status_string.replace("%{search_results}", " | showing result " + QString::number(result_index) + " / " + QString::number(num_search_results));
            if (progress > 0) {
                status_string.replace("%{search_progress}", " (" + QString::number((int)(progress * 100)) + "%" + ")");
            }
        }
    }

    else {
        // Make sure statusbar is hidden if it should be
        if (!should_show_status_label()) {
            status_label->hide();
        }
    }

    if (is_pending_link_source_filled()) {
        status_string.replace("%{link_status}", " | linking ...");
    }
    if (link_to_edit) {
        status_string.replace("%{link_status}", " | editing link ...");
    }
    //if (current_pending_command && current_pending_command.value().requires_symbol) {
    if (is_waiting_for_symbol()) {
        std::wstring wcommand_name = utf8_decode(pending_command_instance->next_requirement(this).value().name);
        status_string.replace("%{waiting_for_symbol}", " " + QString::fromStdString(pending_command_instance->get_name()) + " waiting for symbol");
    }
    if (main_document_view != nullptr && main_document_view->get_document() != nullptr &&
        main_document_view->get_document()->get_is_indexing()) {
        status_string.replace("%{indexing}", " | indexing ... ");
    }
    if (opengl_widget && opengl_widget->get_overview_page()) {
        if (index_into_candidates > 0 && smart_view_candidates.size() > 0) {
            status_string.replace("%{preview_index}", " [ preview " + QString::number(index_into_candidates + 1) + " / " + QString::number(smart_view_candidates.size()) + " ]");

        }
    }
    if (this->synctex_mode) {
        status_string.replace("%{synctex}", " [ synctex ]");
    }
    if (this->mouse_drag_mode) {
        status_string.replace("%{drag}", " [ drag ]");
    }
    if (opengl_widget->is_presentation_mode()) {
        status_string.replace("%{presentation}", " [ presentation ]");
    }

    if (visual_scroll_mode) {
        status_string.replace("%{visual_scroll}", " [ visual scroll ]");
    }

    if (horizontal_scroll_locked) {
        status_string.replace("%{locked_scroll}", " [ locked horizontal scroll ]");
    }
    std::wstring highlight_select_char = L"";

    if (is_select_highlight_mode) {
        highlight_select_char = L"s";
    }

    status_string.replace("%{highlight}", " [ h" + QString::fromStdWString(highlight_select_char) + ":" + select_highlight_type + " ]");


    if (SHOW_CLOSEST_BOOKMARK_IN_STATUSBAR) {
        std::optional<BookMark> closest_bookmark = main_document_view->find_closest_bookmark();
        if (closest_bookmark) {
            status_string.replace("%{closest_bookmark}", " [ " + QString::fromStdWString(closest_bookmark.value().description) + " ]");
        }
    }


    if (SHOW_CLOSE_PORTAL_IN_STATUSBAR) {
        std::optional<Portal> close_portal = main_document_view->find_closest_portal(true);
        if (close_portal) {
            status_string.replace("%{close_portal}", " [ PORTAL ]");
        }
    }


    if (rect_select_mode) {
        status_string.replace("%{rect_select}", " [ select box ]");
    }


    if (custom_status_message.size() > 0) {
        status_string.replace("%{custom_message}", " [ " + QString::fromStdWString(custom_status_message) + " ]");
    }

    bool is_downloading = false;
    if (is_network_manager_running(&is_downloading)) {
        if (is_downloading) {
            status_string.replace("%{download}", " [ downloading ]");
        }
        else {
            status_string.replace("%{download}", " [ searching ]");
        }
    }

    status_string.replace("%{current_page}", "");
    status_string.replace("%{num_pages}", "");
    status_string.replace("%{chapter_name}", "");
    status_string.replace("%{search_results}", "");
    status_string.replace("%{link_status}", "");
    status_string.replace("%{waiting_for_symbol}", "");
    status_string.replace("%{indexing}", "");
    status_string.replace("%{preview_index}", "");
    status_string.replace("%{synctex}", "");
    status_string.replace("%{drag}", "");
    status_string.replace("%{presentation}", "");
    status_string.replace("%{visual_scroll}", "");
    status_string.replace("%{locked_scroll}", "");
    status_string.replace("%{highlight}", "");
    status_string.replace("%{closest_bookmark}", "");
    status_string.replace("%{close_portal}", "");
    status_string.replace("%{rect_select}", "");
    status_string.replace("%{custom_message}", "");
    status_string.replace("%{search_progress}", "");
    status_string.replace("%{download}", "");



    //return ss.str();
    return status_string.toStdWString();
}

void MainWidget::handle_escape() {

    // add high escape priority to overview and search, if any of them are escaped, do not escape any further
    if (opengl_widget) {
        bool should_return = false;
        if (opengl_widget->get_overview_page()) {
            opengl_widget->set_overview_page({});
            should_return = true;
        }
        else if (opengl_widget->get_is_searching(nullptr)) {
            opengl_widget->cancel_search();
			search_buttons->hide();
            should_return = true;
        }
        if (should_return) {
            validate_render();
            setFocus();
            return;
        }
    }

    clear_selection_indicators();
    typing_location = {};
    text_command_line_edit->setText("");
    pending_link = {};
    synchronize_pending_link();
    pending_command_instance = nullptr;
    //current_pending_command = {};

    pop_current_widget();

    if (main_document_view) {
        main_document_view->handle_escape();
        opengl_widget->handle_escape();
    }

    if (opengl_widget) {
		bool done_anything = false;
        if (opengl_widget->get_overview_page()) {
            done_anything = true;
        }
        if (main_document_view->selected_character_rects.size() > 0) {
            done_anything = true;
        }

        opengl_widget->set_overview_page({});
		main_document_view->selected_character_rects.clear();
		selected_text.clear();

        if (!done_anything) {
            opengl_widget->set_should_draw_vertical_line(false);
        }
    }
    //if (opengl_widget) opengl_widget->set_should_draw_vertical_line(false);

    text_command_line_edit_container->hide();

    clear_selected_rect();

    validate_render();
    setFocus();
}

void MainWidget::keyPressEvent(QKeyEvent* kevent) {
    if (TOUCH_MODE){
        if (kevent->key() == Qt::Key_Back){
            if (current_widget_stack.size() > 0){
                pop_current_widget();
            }
            else if (selection_begin_indicator){
                clear_selection_indicators();
            }
        }
    }
    key_event(false, kevent);
}

void MainWidget::keyReleaseEvent(QKeyEvent* kevent) {
    key_event(true, kevent);
}

void MainWidget::validate_render() {

    if (smooth_scroll_mode) {
        if (main_document_view_has_document()) {
            float secs = static_cast<float>(-QTime::currentTime().msecsTo(last_speed_update_time)) / 1000.0f;

            if (secs > 0.1f) {
                secs = 0.0f;
            }

            if (!opengl_widget->get_overview_page()) {
				float current_offset = main_document_view->get_offset_y();
				main_document_view->set_offset_y(current_offset + smooth_scroll_speed * secs);
            }
            else {
                OverviewState state = opengl_widget->get_overview_page().value();
                //opengl_widget->get_overview_offsets(&overview_offset_x, &overview_offset_y);
                //opengl_widget->set_overview_offsets(overview_offset_x, overview_offset_y + smooth_scroll_speed * secs);
                state.absolute_offset_y += smooth_scroll_speed * secs;
                opengl_widget->set_overview_page(state);
            }
            float accel = SMOOTH_SCROLL_DRAG;
            if (smooth_scroll_speed > 0) {
                smooth_scroll_speed -= secs * accel;
                if (smooth_scroll_speed < 0) smooth_scroll_speed = 0;
                
            }
            else {
                smooth_scroll_speed += secs * accel;
                if (smooth_scroll_speed > 0) smooth_scroll_speed = 0;
            }


            last_speed_update_time = QTime::currentTime();
        }
    }
    if (TOUCH_MODE && is_moving()){
        auto current_time = QTime::currentTime();
        float secs = current_time.msecsTo(last_speed_update_time) / 1000.0f;
        float move_x = secs * velocity_x;
        float move_y = secs * velocity_y;
        if (horizontal_scroll_locked) {
            move_x = 0;
        }
        main_document_view->move(move_x, move_y);

        velocity_x = dampen_velocity(velocity_x, secs);
        velocity_y = dampen_velocity(velocity_y, secs);
        if (!is_moving()){
            validation_interval_timer->setInterval(INTERVAL_TIME);
        }
        last_speed_update_time = current_time;

    }
    if (!isVisible()) {
        return;
    }
    if (last_smart_fit_page) {
        int current_page = get_current_page_number();
        if (current_page != last_smart_fit_page) {
            main_document_view->fit_to_page_width(true);
            last_smart_fit_page = current_page;
        }
    }
    if (opengl_widget->is_presentation_mode()) {
        int current_page = get_current_page_number();
        if (current_page >= 0){
			opengl_widget->set_visible_page_number(current_page);
            if (IGNORE_WHITESPACE_IN_PRESENTATION_MODE) {
                main_document_view->set_offset_y(
                    main_document_view->get_document()->get_accum_page_height(current_page) +
                    main_document_view->get_document()->get_page_height(current_page) / 2);
            }
            else {
				main_document_view->set_offset_y(
					main_document_view->get_document()->get_accum_page_height(current_page) +
					main_document_view->get_document()->get_page_height(current_page) / 2 +
					static_cast<float>(get_status_bar_height() / 2 / main_document_view->get_zoom_level()));
            }
			if (IGNORE_WHITESPACE_IN_PRESENTATION_MODE) {
				main_document_view->fit_to_page_height(true);
			}
			else {
				main_document_view->fit_to_page_height_width_minimum();
			}
		}
    }

    if (main_document_view && main_document_view->get_document()) {
        std::optional<Portal> link = main_document_view->find_closest_portal();

        if (helper_document_view){

            if (link) {
                helper_document_view->goto_link(&link.value());
            }
            else {
                helper_document_view->set_null_document();
            }
        }
    }
    validate_ui();
    update();
    if (opengl_widget != nullptr) {
		opengl_widget->update();
    }

    if (helper_opengl_widget != nullptr) {
		helper_opengl_widget->update();
    }

    is_render_invalidated = false;
    if (smooth_scroll_mode && (smooth_scroll_speed != 0)) {
        is_render_invalidated = true;
    }
    if (TOUCH_MODE && is_moving()){
        is_render_invalidated = true;
    }
}

void MainWidget::validate_ui() {
    status_label->setText(QString::fromStdWString(get_status_string()));
    is_ui_invalidated = false;
}

bool MainWidget::move_document(float dx, float dy, bool force) {
    if (main_document_view_has_document()) {
        return main_document_view->move(dx, dy, force);
        //float prev_vertical_line_pos = opengl_widget->get_vertical_line_pos();
        //float new_vertical_line_pos = prev_vertical_line_pos - dy;
        //opengl_widget->set_vertical_line_pos(new_vertical_line_pos);
    }
    return false;
}

void MainWidget::move_document_screens(int num_screens) {
    int view_height = opengl_widget->height();
    float move_amount = num_screens * view_height * MOVE_SCREEN_PERCENTAGE;
    move_document(0, move_amount);
}

//QString MainWidget::get_status_stylesheet() {
//    if (STATUS_BAR_FONT_SIZE > -1) {
//        QString	font_size_stylesheet = QString("font-size: %1px").arg(STATUS_BAR_FONT_SIZE);
//        return QString("background-color: %1; color: %2; border: 0; %3").arg(
//            get_color_qml_string(STATUS_BAR_COLOR[0], STATUS_BAR_COLOR[1], STATUS_BAR_COLOR[2]),
//            get_color_qml_string(STATUS_BAR_TEXT_COLOR[0], STATUS_BAR_TEXT_COLOR[1], STATUS_BAR_TEXT_COLOR[2]),
//            font_size_stylesheet
//        );
//    }
//    else{
//        return QString("background-color: %1; color: %2; border: 0").arg(
//            get_color_qml_string(STATUS_BAR_COLOR[0], STATUS_BAR_COLOR[1], STATUS_BAR_COLOR[2]),
//            get_color_qml_string(STATUS_BAR_TEXT_COLOR[0], STATUS_BAR_TEXT_COLOR[1], STATUS_BAR_TEXT_COLOR[2])
//        );
//    }
//}
//

void MainWidget::on_config_file_changed(ConfigManager* new_config) {

    status_label->setStyleSheet(get_status_stylesheet());
    status_label->setFont(QFont(get_font_face_name()));
    text_command_line_edit_container->setStyleSheet(get_status_stylesheet());
    text_command_line_edit->setFont(QFont(get_font_face_name()));

    text_command_line_edit_label->setStyleSheet(get_status_stylesheet());
    text_command_line_edit->setStyleSheet(get_status_stylesheet());
    //status_label->setStyleSheet(get_status_stylesheet());

    int status_bar_height = get_status_bar_height();
    status_label->move(0, main_window_height - status_bar_height);
    status_label->resize(size().width(), status_bar_height);

    //text_command_line_edit_container->setStyleSheet("background-color: black; color: white; border: none;");
}

//void MainWidget::toggle_dark_mode()
//{
//	this->dark_mode = !this->dark_mode;
//	if (this->opengl_widget) {
//		this->opengl_widget->set_dark_mode(this->dark_mode);
//	}
//	if (this->helper_opengl_widget) {
//		this->helper_opengl_widget->set_dark_mode(this->dark_mode);
//	}
//}

void MainWidget::toggle_mouse_drag_mode(){
    this->mouse_drag_mode = !this->mouse_drag_mode;
}

void MainWidget::do_synctex_forward_search(const Path& pdf_file_path, const Path& latex_file_path, int line, int column) {
#ifndef SIOYEK_ANDROID

    std::wstring latex_file_path_with_redundant_dot = add_redundant_dot_to_path(latex_file_path.get_path());

    std::string latex_file_string = latex_file_path.get_path_utf8();
    std::string latex_file_with_redundant_dot_string = utf8_encode(latex_file_path_with_redundant_dot);
    std::string pdf_file_string = pdf_file_path.get_path_utf8();

    synctex_scanner_p scanner = synctex_scanner_new_with_output_file(pdf_file_string.c_str(), nullptr, 1);

    int stat = synctex_display_query(scanner, latex_file_string.c_str(), line, column, 0);
    int target_page = -1;

    if (stat <= 0) {
        stat = synctex_display_query(scanner, latex_file_with_redundant_dot_string.c_str(), line, column, 0);
    }

    if (stat > 0) {
        synctex_node_p node;

        std::vector<std::pair<int, fz_rect>> highlight_rects;

        std::optional<fz_rect> first_rect = {};

        while ((node = synctex_scanner_next_result(scanner))) {
            int page = synctex_node_page(node);
            target_page = page-1;

            float x = synctex_node_box_visible_h(node);
            float y = synctex_node_box_visible_v(node);
            float w = synctex_node_box_visible_width(node);
            float h = synctex_node_box_visible_height(node);

            fz_rect doc_rect;
            doc_rect.x0 = x;
            doc_rect.y0 = y;
            doc_rect.x1 = x + w;
            doc_rect.y1 = y - h;

            if (!first_rect) {
                first_rect = doc_rect;
            }

            highlight_rects.push_back(std::make_pair(target_page, doc_rect));

            break; // todo: handle this properly
        }
        if (target_page != -1) {

            if ((main_document_view->get_document() == nullptr) ||
                (pdf_file_path.get_path() != main_document_view->get_document()->get_path())) {

                open_document(pdf_file_path);

            }

            opengl_widget->set_synctex_highlights(highlight_rects);
            if (highlight_rects.size() == 0) {
                main_document_view->goto_page(target_page);
            }
            else {
                main_document_view->goto_offset_within_page({ target_page, main_document_view->get_offset_x(), first_rect.value().y0 });
            }
        }
    }
    else {
        open_document(pdf_file_path);
    }
    synctex_scanner_free(scanner);
#endif
}


void MainWidget::update_link_with_opened_book_state(Portal lnk, const OpenedBookState& new_state) {
    std::wstring docpath = main_document_view->get_document()->get_path();
    Document* link_owner = document_manager->get_document(docpath);

    lnk.dst.book_state = new_state;

    if (link_owner) {
        link_owner->update_portal(lnk);
    }

    db_manager->update_portal(link_owner->get_checksum(),
        new_state.offset_x, new_state.offset_y, new_state.zoom_level, lnk.src_offset_y);

    link_to_edit = {};
}

void MainWidget::update_closest_link_with_opened_book_state(const OpenedBookState& new_state) {
    std::optional<Portal> closest_link = main_document_view->find_closest_portal();
    if (closest_link) {
        update_link_with_opened_book_state(closest_link.value(), new_state);
    }
}

void MainWidget::invalidate_render() {
    invalidate_ui();
    is_render_invalidated = true;
}

void MainWidget::invalidate_ui() {
    is_render_invalidated = true;
}

void MainWidget::open_document(const PortalViewState& lvs) {
    DocumentViewState dvs;
    auto path = checksummer->get_path(lvs.document_checksum);
    if (path) {
        dvs.book_state = lvs.book_state;
        dvs.document_path = path.value();
        open_document(dvs);
    }
}

void MainWidget::open_document_with_hash(const std::string& path, std::optional<float> offset_x, std::optional<float> offset_y, std::optional<float> zoom_level) {
    std::optional<std::wstring> maybe_path = checksummer->get_path(path);
    if (maybe_path) {
        Path path(maybe_path.value());
        open_document(path, offset_x, offset_y, zoom_level);
    }
}

void MainWidget::open_document(const Path& path, std::optional<float> offset_x, std::optional<float> offset_y, std::optional<float> zoom_level) {

    opengl_widget->clear_all_selections();

    //save the previous document state
    if (main_document_view) {
        main_document_view->persist();
    }

    if (main_document_view->get_view_width() > main_window_width) {
        main_window_width = main_document_view->get_view_width();
    }

    main_document_view->on_view_size_change(main_window_width, main_window_height);
    main_document_view->open_document(path.get_path(), &this->is_render_invalidated);
    bool has_document = main_document_view_has_document();

    if (has_document) {
        //setWindowTitle(QString::fromStdWString(path.get_path()));
        if (path.filename().has_value()) {
            setWindowTitle(QString::fromStdWString(path.filename().value()));
        }
        else {
            setWindowTitle(QString::fromStdWString(path.get_path()));
        }

    }

    if ((path.get_path().size() > 0) && (!has_document)) {
        show_error_message(L"Could not open file: " + path.get_path());
    }

    if (offset_x) {
        main_document_view->set_offset_x(offset_x.value());
    }
    if (offset_y) {
        main_document_view->set_offset_y(offset_y.value());
    }

    if (zoom_level) {
        main_document_view->set_zoom_level(zoom_level.value(), true);
    }

    // reset smart fit when changing documents
    last_smart_fit_page = {};
    opengl_widget->on_document_view_reset();
    show_password_prompt_if_required();

    if (main_document_view_has_document()) {
        scroll_bar->setSingleStep(std::max(MAX_SCROLLBAR / doc()->num_pages() / 10, 1));
        scroll_bar->setPageStep(MAX_SCROLLBAR / doc()->num_pages());
        update_scrollbar();
    }


}

void MainWidget::open_document_at_location(const Path& path_,
    int page,
    std::optional<float> x_loc,
    std::optional<float> y_loc,
    std::optional<float> zoom_level)
{
    //save the previous document state
    if (main_document_view) {
        main_document_view->persist();
    }
    std::wstring path = path_.get_path();

    open_document(path, &this->is_render_invalidated, true, {}, true);
    bool has_document = main_document_view_has_document();

    if (has_document) {
        //setWindowTitle(QString::fromStdWString(path));
        push_state();
    }

    if ((path.size() > 0) && (!has_document)) {
        show_error_message(L"Could not open file: " + path);
    }

    main_document_view->on_view_size_change(main_window_width, main_window_height);

    AbsoluteDocumentPos absolute_pos = main_document_view->get_document()->document_to_absolute_pos(
        { page, x_loc.value_or(0), y_loc.value_or(0) }, true);

    if (x_loc) {
        main_document_view->set_offset_x(absolute_pos.x);
    }
    main_document_view->set_offset_y(absolute_pos.y);

    if (zoom_level) {
        main_document_view->set_zoom_level(zoom_level.value(), true);
    }

    // reset smart fit when changing documents
    last_smart_fit_page = {};
    opengl_widget->on_document_view_reset();
    show_password_prompt_if_required();
}

void MainWidget::open_document(const DocumentViewState& state)
{
    open_document(state.document_path, state.book_state.offset_x, state.book_state.offset_y, state.book_state.zoom_level);
}


bool MainWidget::is_waiting_for_symbol() {
	return ((pending_command_instance != nullptr) &&
		pending_command_instance->next_requirement(this).has_value() &&
		(pending_command_instance->next_requirement(this).value().type == RequirementType::Symbol));
}

void MainWidget::handle_command_types(std::unique_ptr<Command> new_command, int num_repeats) {

    if (new_command == nullptr) {
        return;
    }

    if (new_command) {
		new_command->set_num_repeats(num_repeats);
        if (new_command->pushes_state()) {
            push_state();
        }
		if (main_document_view_has_document()) {
			main_document_view->disable_auto_resize_mode();
		}
        advance_command(std::move(new_command));
		update_scrollbar();
    }
    return;

}

void MainWidget::key_event(bool released, QKeyEvent* kevent) {
    validate_render();

    if (typing_location.has_value()) {

        if (released == false) {
			if (kevent->key() == Qt::Key::Key_Escape) {
				handle_escape();
                return;
			}

            bool should_focus = false;
			if (kevent->key() == Qt::Key::Key_Return) {
				typing_location.value().next_char();
			}
			else if (kevent->key() == Qt::Key::Key_Backspace) {
                typing_location.value().backspace();
			}
			else if (kevent->text().size() > 0) {
				char c = kevent->text().at(0).unicode();
				should_focus = typing_location.value().advance(c);
			}

			int page = typing_location.value().page;
			fz_rect character_rect = fz_rect_from_quad(typing_location.value().character->quad);
			std::optional<fz_rect> wrong_rect = {};

			if (typing_location.value().previous_character) {
				wrong_rect = fz_rect_from_quad(typing_location.value().previous_character->character->quad);
			}

			if (should_focus) {
				main_document_view->set_offset_y(typing_location.value().focus_offset());
			}
			opengl_widget->set_typing_rect(page, character_rect, wrong_rect);

		}
        return;

    }


    if (released == false) {

#ifdef SIOYEK_ANDROID
            if (kevent->key() == Qt::Key::Key_VolumeDown) {
                move_visual_mark_next();
            }
            if (kevent->key() == Qt::Key::Key_VolumeUp) {
                move_visual_mark_prev();
            }
#endif


        if (kevent->key() == Qt::Key::Key_Escape) {
            handle_escape();
        }

        if (kevent->key() == Qt::Key::Key_Return || kevent->key() == Qt::Key::Key_Enter) {
            if (text_command_line_edit_container->isVisible()) {
                text_command_line_edit_container->hide();
                setFocus();
                handle_pending_text_command(text_command_line_edit->text().toStdWString());
                return;
            }
        }

        std::vector<int> ignored_codes = {
            Qt::Key::Key_Shift,
            Qt::Key::Key_Control,
            Qt::Key::Key_Alt
        };
        if (std::find(ignored_codes.begin(), ignored_codes.end(), kevent->key()) != ignored_codes.end()) {
            return;
        }
        if (is_waiting_for_symbol()) {

            char symb = get_symbol(kevent->key(), kevent->modifiers() & Qt::ShiftModifier, pending_command_instance->special_symbols());
            if (symb) {
                pending_command_instance->set_symbol_requirement(symb);
                advance_command(std::move(pending_command_instance));
            }
            return;
        }
        int num_repeats = 0;
        bool is_control_pressed = (kevent->modifiers() & Qt::ControlModifier) || (kevent->modifiers() & Qt::MetaModifier);
        std::vector<std::unique_ptr<Command>> commands = input_handler->handle_key(
            kevent,
            kevent->modifiers() & Qt::ShiftModifier,
            is_control_pressed,
            kevent->modifiers() & Qt::AltModifier,
            &num_repeats);

        for (auto& command : commands) {
            handle_command_types(std::move(command), num_repeats);
        }
    }

}

void MainWidget::handle_right_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed) {

    if (is_rotated()) {
        return;
    }
    if (is_shift_pressed || is_control_pressed || is_alt_pressed) {
        return;
    }

    if ((down == true) && opengl_widget->get_overview_page()) {
        opengl_widget->set_overview_page({});
        //main_document_view->set_line_index(-1);
        invalidate_render();
        return;
    }

    if ((main_document_view->get_document() != nullptr) && (opengl_widget != nullptr)) {

        // disable visual mark and overview window when we are in synctex mode
        // because we probably don't need them (we are editing our own document after all)
        // we can always use middle click to jump to a destination which is probably what we
        // need anyway
        if (down == true && (!this->synctex_mode)) {
            if (pending_command_instance && (pending_command_instance->get_name() == "goto_mark")) {
                return_to_last_visual_mark();
                return;
            }

            if (overview_under_pos(click_pos)) {
                return;
            }

            visual_mark_under_pos(click_pos);

        }
        else {
            if (this->synctex_mode) {
                if (down == false) {
					synctex_under_pos(click_pos);
                }
            }
        }

    }

}

void MainWidget::handle_left_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed) {

    if (is_rotated()) {
        return;
    }
    if (is_shift_pressed || is_control_pressed || is_alt_pressed) {
        return;
    }

    if (TOUCH_MODE){
        was_last_mouse_down_in_ruler_next_rect = false;
        was_last_mouse_down_in_ruler_prev_rect = false;
        if (down){
            last_press_point = mapFromGlobal(QCursor::pos());
            last_press_msecs = QDateTime::currentMSecsSinceEpoch();
            velocity_x = 0;
            velocity_y = 0;
            is_pressed = true;
        }
        if (!down){
            is_pressed = false;
            QPoint current_pos = mapFromGlobal(QCursor::pos());
            qint64 current_time = QDateTime::currentMSecsSinceEpoch();
            QPointF vel;
            if (((current_pos-last_press_point).manhattanLength() < 10) && ((current_time - last_press_msecs) < 500)){
                if (handle_quick_tap(click_pos)) {
                    is_dragging = false;
                    invalidate_render();
                    return;
                }
            }
            else if (is_flicking(&vel)){
                //            float time_msecs = current_time - last_press_msecs;
                //            QPoint diff_vector = current_pos - last_press_point;
                //            QPoint velocity = diff_vector / (time_msecs / 1000.0f) * 2;
                velocity_x = -vel.x();
                velocity_y = vel.y();
                if (is_moving()){
                    validation_interval_timer->setInterval(0);
                }
                last_speed_update_time = QTime::currentTime();
            }
        }
        int window_width = width();
        int window_height = height();

        NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(click_pos);

		if (down && is_visual_mark_mode()) {
			if (screen()->orientation() == Qt::PortraitOrientation) {
				if (PORTRAIT_VISUAL_MARK_NEXT.enabled && PORTRAIT_VISUAL_MARK_NEXT.contains(nwp)) {
					move_visual_mark_next();
					was_last_mouse_down_in_ruler_next_rect = true;
					ruler_moving_last_window_pos = click_pos;
				}
				else if (PORTRAIT_VISUAL_MARK_PREV.enabled && PORTRAIT_VISUAL_MARK_PREV.contains(nwp)) {
					move_visual_mark_prev();
					was_last_mouse_down_in_ruler_prev_rect = true;
					ruler_moving_last_window_pos = click_pos;
				}
			}
			else {
				if (LANDSCAPE_VISUAL_MARK_NEXT.enabled && LANDSCAPE_VISUAL_MARK_NEXT.contains(nwp)) {
					move_visual_mark_next();
					was_last_mouse_down_in_ruler_next_rect = true;
					ruler_moving_last_window_pos = click_pos;
				}
				else if (LANDSCAPE_VISUAL_MARK_PREV.enabled && LANDSCAPE_VISUAL_MARK_PREV.contains(nwp)) {
					move_visual_mark_prev();
					was_last_mouse_down_in_ruler_prev_rect = true;
					ruler_moving_last_window_pos = click_pos;
				}

			}

		}

    }

    AbsoluteDocumentPos abs_doc_pos = main_document_view->window_to_absolute_document_pos(click_pos);

    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(click_pos);


    if (!TOUCH_MODE){
        if (opengl_widget) opengl_widget->set_should_draw_vertical_line(false);
    }

    if (rect_select_mode) {
        if (down == true) {
            if (rect_select_end.has_value()) {
                //clicked again after selecting, we should clear the selected rectangle
                clear_selected_rect();
            }
            else {
                rect_select_begin = abs_doc_pos;
            }
        }
        else {
            if (rect_select_begin.has_value() && rect_select_end.has_value()) {
				rect_select_end = abs_doc_pos;
				fz_rect selected_rectangle;
				selected_rectangle.x0 = rect_select_begin.value().x;
				selected_rectangle.y0 = rect_select_begin.value().y;
				selected_rectangle.x1 = rect_select_end.value().x;
				selected_rectangle.y1 = rect_select_end.value().y;
				opengl_widget->set_selected_rectangle(selected_rectangle);

                // is pending rect command
                if (pending_command_instance) {
                    pending_command_instance->set_rect_requirement(selected_rectangle);
                    advance_command(std::move(pending_command_instance));
                }

				this->rect_select_mode = false;
				this->rect_select_begin = {};
				this->rect_select_end = {};
            }
			
        }
		return;
    }
    else {
        if (down == true) {
            clear_selected_rect();
        }
    }

    if (down == true) {

        PdfViewOpenGLWidget::OverviewSide border_index = static_cast<PdfViewOpenGLWidget::OverviewSide>(-1);
        if (opengl_widget->is_window_point_in_overview_border(normal_x, normal_y, &border_index)) {
            PdfViewOpenGLWidget::OverviewResizeData resize_data;
            resize_data.original_normal_mouse_pos = NormalizedWindowPos{ normal_x, normal_y };
            resize_data.original_rect = opengl_widget->get_overview_rect();
            resize_data.side_index = border_index;
            overview_resize_data = resize_data;
            return;
        }
        if (opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
            float original_offset_x, original_offset_y;

            PdfViewOpenGLWidget::OverviewMoveData move_data;
            opengl_widget->get_overview_offsets(&original_offset_x, &original_offset_y);
            move_data.original_normal_mouse_pos = NormalizedWindowPos{ normal_x, normal_y };
            move_data.original_offsets = fvec2{ original_offset_x, original_offset_y };
            overview_move_data = move_data;
            return;
        }

        selection_begin = abs_doc_pos;
        //selection_begin_x = x_;
        //selection_begin_y = y_;

        last_mouse_down = abs_doc_pos;
        //last_mouse_down_x = x_;
        //last_mouse_down_y = y_;
        last_mouse_down_window_pos = click_pos;
        last_mouse_down_document_offset = main_document_view->get_offsets();
        //last_mouse_down_window_x = x;
        //last_mouse_down_window_y = y;

        if (!TOUCH_MODE){
            main_document_view->selected_character_rects.clear();
        }

        if ((!TOUCH_MODE) && (!mouse_drag_mode)) {
            is_selecting = true;
			if (SINGLE_CLICK_SELECTS_WORDS) {
				is_word_selecting = true;
			}
        }
        else {
            is_dragging = true;
        }
    }
    else {
        selection_end = abs_doc_pos;

        is_selecting = false;
        is_dragging = false;

        bool was_overview_mode = overview_move_data.has_value() || overview_resize_data.has_value();

        overview_move_data = {};
        overview_resize_data = {};

        //if (was_overview_mode) {
        //    return;
        //}

        if ((!was_overview_mode) &&  (!TOUCH_MODE) && (!mouse_drag_mode) && (manhattan_distance(fvec2(last_mouse_down), fvec2(abs_doc_pos)) > 5)) {

            //fz_point selection_begin = { last_mouse_down_x, last_mouse_down_y };
            //fz_point selection_end = { x_, y_ };

            main_document_view->get_text_selection(last_mouse_down,
                abs_doc_pos,
                is_word_selecting,
                main_document_view->selected_character_rects,
                selected_text);
            is_word_selecting = false;
        }
        else {
//            WindowPos current_window_pos = {};
//            handle_click(click_pos);
            if (!TOUCH_MODE){
                handle_click(click_pos);
                clear_selected_text();
            }
            else{
                int distance = abs(click_pos.x - last_mouse_down_window_pos.x) + abs(click_pos.y - last_mouse_down_window_pos.y);
                if (distance < 20){ // we don't want to accidentally click on links when moving the document
                    handle_click(click_pos);
                }
            }

        }
        validate_render();
    }
}


void MainWidget::push_state(bool update) {

    if (!main_document_view_has_document()) return; // we don't add empty document to history

    DocumentViewState dvs = main_document_view->get_state();

    //if (history.size() > 0) { // this check should always be true
    //	history[history.size() - 1] = dvs;
    //}
    //// don't add the same place in history multiple times
    //// todo: we probably don't need this check anymore
    //if (history.size() > 0) {
    //	DocumentViewState last_history = history.back();
    //	if (last_history == dvs) return;
    //}

    // delete all history elements after the current history point
	history.erase(history.begin() + (1 + current_history_index), history.end());
    if (!((history.size() > 0) && (history.back() == dvs))) {
		history.push_back(dvs);
    }
    if (update) {
		current_history_index = static_cast<int>(history.size() - 1);
    }
}

void MainWidget::next_state() {
    //update_current_history_index();
    if (current_history_index < (static_cast<int>(history.size())-1)) {
        current_history_index++;
        if (current_history_index + 1 < history.size()) {
			set_main_document_view_state(history[current_history_index + 1]);
        }

    }
}

void MainWidget::prev_state() {
    //update_current_history_index();
    if (current_history_index >= 0) {

        /*
        Goto previous history
        In order to edit a link, we set the link to edit and jump to the link location, when going back, we
        update the link with the current location of document, therefore, we must check to see if a link
        is being edited and if so, we should update its destination position
        */
        if (link_to_edit) {

            //std::wstring link_document_path = checksummer->get_path(link_to_edit.value().dst.document_checksum).value();
            std::wstring link_document_path = history[current_history_index].document_path;
            Document* link_owner = document_manager->get_document(link_document_path);

            OpenedBookState state = main_document_view->get_state().book_state;
            link_to_edit.value().dst.book_state = state;

            if (link_owner) {
                link_owner->update_portal(link_to_edit.value());
            }

            db_manager->update_portal(checksummer->get_checksum(history[current_history_index].document_path),
                state.offset_x, state.offset_y, state.zoom_level, link_to_edit->src_offset_y);
            link_to_edit = {};
        }

        if (current_history_index == (history.size() - 1)) {
            if (!(history[history.size() - 1] == main_document_view->get_state())) {
                push_state(false);
            }
        }
        if (history[current_history_index] == main_document_view->get_state()) {
            current_history_index--;
        }
        if (current_history_index >= 0) {
            DocumentViewState new_state = history[current_history_index];
            // save the current document in the list of opened documents
            if (doc() && doc()->get_path() != new_state.document_path) {
                persist();
            }
			set_main_document_view_state(new_state);
			current_history_index--;
        }
    }
}

void MainWidget::update_current_history_index() {
    if (main_document_view_has_document()) {
        int index_to_update = current_history_index + 1;
        if (index_to_update < history.size()-1) {
			DocumentViewState current_state = main_document_view->get_state();
			history[index_to_update] = current_state;
        }
    }
}

void MainWidget::set_main_document_view_state(DocumentViewState new_view_state) {

	if ((!main_document_view_has_document()) || (main_document_view->get_document()->get_path() != new_view_state.document_path)) {
		open_document(new_view_state.document_path, &this->is_ui_invalidated);

		//setwindowtitle(qstring::fromstdwstring(new_view_state.document_path));
	}

	main_document_view->on_view_size_change(main_window_width, main_window_height);
	main_document_view->set_book_state(new_view_state.book_state);
}

void MainWidget::handle_click(WindowPos click_pos) {


    if (!main_document_view_has_document()) {
        return;
    }

    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(click_pos);

    if (opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
        auto [doc_page, doc_x, doc_y] = opengl_widget->window_pos_to_overview_pos({ normal_x, normal_y });
        auto link = main_document_view->get_document()->get_link_in_pos(doc_page, doc_x, doc_y);
        if (link) {
            handle_link_click(link.value());
        }
        return;
    }

    auto link = main_document_view->get_link_in_pos(click_pos);
    selected_highlight_index = main_document_view->get_highlight_index_in_pos(click_pos);

    if (TOUCH_MODE && (selected_highlight_index != -1)){
        show_highlight_buttons();
    }


    if (link.has_value()) {
        handle_link_click(link.value());
    }
}
ReferenceType MainWidget::find_location_of_text_under_pointer(WindowPos pointer_pos, int* out_page, float* out_offset, bool update_candidates) {

    auto [page, offset_x, offset_y] = main_document_view->window_to_document_pos(pointer_pos);
    int current_page_number = get_current_page_number();

    fz_stext_page* stext_page = main_document_view->get_document()->get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);

    std::optional<std::pair<std::wstring, std::wstring>> generic_pair =\
        main_document_view->get_document()->get_generic_link_name_at_position(flat_chars, offset_x, offset_y);

    std::optional<std::wstring> reference_text_on_pointer = main_document_view->get_document()->get_reference_text_at_position(flat_chars, offset_x, offset_y);
    std::optional<std::wstring> equation_text_on_pointer = main_document_view->get_document()->get_equation_text_at_position(flat_chars, offset_x, offset_y);

    if (generic_pair) {
        std::vector<DocumentPos> candidates = main_document_view->get_document()->find_generic_locations(generic_pair.value().first,
            generic_pair.value().second);
        if (candidates.size() > 0) {
            if (update_candidates) {
                smart_view_candidates = candidates;
                index_into_candidates = 0;
            }
            *out_page = candidates[index_into_candidates].page;
            *out_offset = candidates[index_into_candidates].y;
            return ReferenceType::Generic;
        }
    }
    if (equation_text_on_pointer) {
         std::optional<IndexedData> eqdata_ = main_document_view->get_document()->find_equation_with_string(equation_text_on_pointer.value(), current_page_number);
         if (eqdata_) {
             IndexedData refdata = eqdata_.value();
             *out_page = refdata.page;
             *out_offset = refdata.y_offset;
             return ReferenceType::Equation;
         }
    }

    if (reference_text_on_pointer) {
         std::optional<IndexedData> refdata_ = main_document_view->get_document()->find_reference_with_string(reference_text_on_pointer.value());
         if (refdata_) {
             IndexedData refdata = refdata_.value();
             *out_page = refdata.page;
             *out_offset = refdata.y_offset;
             return ReferenceType::Reference;
         }

    }

    return ReferenceType::None;
}

void MainWidget::mouseReleaseEvent(QMouseEvent* mevent) {

    bool is_shift_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);
    bool is_control_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
    bool is_alt_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::AltModifier);

    if (is_drawing) {
        is_drawing = false;
        //int initial_size = freehand_drawing_points.size();
        std::vector<FreehandDrawingPoint> pruned_points = prune_freehand_drawing_points(opengl_widget->current_drawing.points);
        opengl_widget->current_drawing.points.clear();

        FreehandDrawing pruned_drawing;
        pruned_drawing.points = pruned_points;
        pruned_drawing.color[0] = opengl_widget->current_drawing.color[0];
        pruned_drawing.color[1] = opengl_widget->current_drawing.color[1];
        pruned_drawing.color[2] = opengl_widget->current_drawing.color[2];
        pruned_drawing.color[3] = opengl_widget->current_drawing.color[3];
        pruned_drawing.creattion_time = QDateTime::currentDateTime();
        doc()->add_freehand_drawing(pruned_drawing);
        //int a = 2;
        invalidate_render();
        return;
    }

    if (TOUCH_MODE){

        pdf_renderer->no_rerender = false;
    }

	if (is_rotated()) {
		return;
	}

    if (mevent->button() == Qt::MouseButton::LeftButton) {
        if (is_shift_pressed) {
			auto commands = command_manager->create_macro_command("", SHIFT_CLICK_COMMAND);
			commands->run(this);
        }
        else if (is_control_pressed) {
			auto commands = command_manager->create_macro_command("", CONTROL_CLICK_COMMAND);
			commands->run(this);
        }
        else if (is_alt_pressed) {
			auto commands = command_manager->create_macro_command("", ALT_CLICK_COMMAND);
			commands->run(this);
        }
        else {
			handle_left_click({ mevent->pos().x(), mevent->pos().y() }, false, is_shift_pressed, is_control_pressed, is_alt_pressed);
			if (is_select_highlight_mode && (main_document_view->selected_character_rects.size() > 0)) {
				main_document_view->add_highlight(selection_begin, selection_end, select_highlight_type);
                clear_selected_text();
			}
			if (main_document_view->selected_character_rects.size() > 0) {
				copy_to_clipboard(selected_text, true);
			}
        }

    }

    if (mevent->button() == Qt::MouseButton::RightButton) {
        if (is_shift_pressed) {
			auto commands = command_manager->create_macro_command("", SHIFT_RIGHT_CLICK_COMMAND);
			commands->run(this);
        }
        else if (is_control_pressed) {
			auto commands = command_manager->create_macro_command("", CONTROL_RIGHT_CLICK_COMMAND);
			commands->run(this);
        }
        else if (is_alt_pressed) {
			auto commands = command_manager->create_macro_command("", ALT_RIGHT_CLICK_COMMAND);
			commands->run(this);
        }
        else {
			handle_right_click({ mevent->pos().x(), mevent->pos().y() }, false, is_shift_pressed, is_control_pressed, is_alt_pressed);
        }
    }

    if (mevent->button() == Qt::MouseButton::MiddleButton) {
        if (!is_dragging) {

            if (HIGHLIGHT_MIDDLE_CLICK
                && main_document_view->selected_character_rects.size() > 0
                && !(opengl_widget && opengl_widget->get_overview_page())) {
                command_manager->get_command_with_name("add_highlight_with_current_type")->run(this);
                invalidate_render();
            }
            else {
                if (last_middle_down_time.msecsTo(QTime::currentTime()) > 200) {
                }
                else {
					smart_jump_under_pos({ mevent->pos().x(), mevent->pos().y() });
                }
            }
        }
        else{
            is_dragging = false;

        }
    }

}

void MainWidget::mouseDoubleClickEvent(QMouseEvent* mevent) {
    if (!TOUCH_MODE) {
        if (mevent->button() == Qt::MouseButton::LeftButton) {
            is_selecting = true;
            if (SINGLE_CLICK_SELECTS_WORDS) {
                is_word_selecting = false;
            }
            else {
                is_word_selecting = true;
            }
        }
    }
}

void MainWidget::mousePressEvent(QMouseEvent* mevent) {
    bool is_shift_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);
    bool is_control_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
    bool is_alt_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::AltModifier);

    if (freehand_drawing_mode && (mevent->button() == Qt::MouseButton::LeftButton)) {
        is_drawing = true;
        opengl_widget->current_drawing.points.clear();
        opengl_widget->current_drawing.color[0] = HIGHLIGHT_COLORS[(current_freehand_type - 'a') * 3];
        opengl_widget->current_drawing.color[1] = HIGHLIGHT_COLORS[(current_freehand_type - 'a') * 3 + 1];
        opengl_widget->current_drawing.color[2] = HIGHLIGHT_COLORS[(current_freehand_type - 'a') * 3 + 2];
        opengl_widget->current_drawing.color[3] = 1.0f;
        return;
    }

    if (mevent->button() == Qt::MouseButton::LeftButton) {
        handle_left_click({ mevent->pos().x(), mevent->pos().y() }, true, is_shift_pressed, is_control_pressed, is_alt_pressed);
    }

    if (mevent->button() == Qt::MouseButton::RightButton) {
        handle_right_click({ mevent->pos().x(), mevent->pos().y() }, true, is_shift_pressed, is_control_pressed, is_alt_pressed);
    }

    if (mevent->button() == Qt::MouseButton::MiddleButton) {
		last_middle_down_time = QTime::currentTime();
        last_mouse_down_window_pos = WindowPos{mevent->pos().x(), mevent->pos().y()};
        last_mouse_down_document_offset = main_document_view->get_offsets();
    }

    if (mevent->button() == Qt::MouseButton::XButton1) {
        handle_command_types(command_manager->get_command_with_name("prev_state"), 0);
        invalidate_render();
    }

    if (mevent->button() == Qt::MouseButton::XButton2) {
        handle_command_types(command_manager->get_command_with_name("next_state"), 0);
        invalidate_render();
    }
}

void MainWidget::wheelEvent(QWheelEvent* wevent) {

    std::unique_ptr<Command> command = nullptr;
    //bool is_touchpad = wevent->source() == Qt::MouseEventSource::MouseEventSynthesizedBySystem;
    //bool is_touchpad = true;
    float vertical_move_amount = VERTICAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;
    float horizontal_move_amount = HORIZONTAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;

    //if (is_touchpad) {
    //	vertical_move_amount *= TOUCHPAD_SENSITIVITY;
    //	horizontal_move_amount *= TOUCHPAD_SENSITIVITY;
    //}
    if (main_document_view_has_document()) {
        main_document_view->disable_auto_resize_mode();
    }

    bool is_control_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier) ||
        QApplication::queryKeyboardModifiers().testFlag(Qt::MetaModifier);

    bool is_shift_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier);
    bool is_visual_mark_mode = opengl_widget->get_should_draw_vertical_line() && visual_scroll_mode;


#ifdef SIOYEK_QT6
    int x = wevent->position().x();
    int y = wevent->position().y();
#else
    int x = wevent->pos().x();
    int y = wevent->pos().y();
#endif

    WindowPos mouse_window_pos = { x, y };
    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(mouse_window_pos);

#ifdef SIOYEK_QT6
	int num_repeats = abs(wevent->angleDelta().y() / 120);
	float num_repeats_f = abs(wevent->angleDelta().y() / 120.0);
#else
	int num_repeats = abs(wevent->delta() / 120);
	float num_repeats_f = abs(wevent->delta() / 120.0);
#endif

    if (num_repeats == 0) {
        num_repeats = 1;
    }

    if ((!is_control_pressed) && (!is_shift_pressed)) {
        if (opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
            if (wevent->angleDelta().y() > 0) {
                scroll_overview(-1);
            }
            if (wevent->angleDelta().y() < 0) {
                scroll_overview(1);
            }
            validate_render();
        }
        else {

            if (wevent->angleDelta().y() > 0) {

                if (is_visual_mark_mode) {
                    command = command_manager->get_command_with_name("move_visual_mark_up");
                }
                else {
                    move_vertical(-72.0f * vertical_move_amount * num_repeats_f);
					update_scrollbar();
                    return;
                }
            }
            if (wevent->angleDelta().y() < 0) {

                if (is_visual_mark_mode) {
                    command = command_manager->get_command_with_name("move_visual_mark_down");
                }
                else {
                    move_vertical(72.0f * vertical_move_amount * num_repeats_f);
					update_scrollbar();
                    return;
                }
            }

			float inverse_factor = INVERTED_HORIZONTAL_SCROLLING ? -1.0f : 1.0f;

            if (wevent->angleDelta().x() > 0) {
                move_horizontal(-72.0f * horizontal_move_amount * num_repeats_f * inverse_factor);
                return;
            }
            if (wevent->angleDelta().x() < 0) {
                move_horizontal(72.0f * horizontal_move_amount * num_repeats_f * inverse_factor);
                return;
            }
        }
    }

    if (is_control_pressed) {
        float zoom_factor = 1.0f + num_repeats_f * (ZOOM_INC_FACTOR - 1.0f);
        zoom(mouse_window_pos, zoom_factor, wevent->angleDelta().y() > 0);
        return;
    }
    if (is_shift_pressed) {
        float inverse_factor = INVERTED_HORIZONTAL_SCROLLING ? -1.0f : 1.0f;

        if (wevent->angleDelta().y() > 0) {
            move_horizontal(-72.0f * horizontal_move_amount * num_repeats_f * inverse_factor);
            return;
        }
        if (wevent->angleDelta().y() < 0) {
            move_horizontal(72.0f * horizontal_move_amount * num_repeats_f * inverse_factor);
            return;
        }

    }

    if (command) {
        //handle_command(command, num_repeats);
        command->set_num_repeats(num_repeats);
        command->run(this);
    }
}

void MainWidget::show_mark_selector() {
    TouchMarkSelector* mark_selector = new TouchMarkSelector(this);

    set_current_widget(mark_selector);
    QObject::connect(mark_selector, &TouchMarkSelector::onMarkSelected, [&](QString mark) {

        if (mark.size() > 0 && pending_command_instance) {
            char symbol = mark.toStdString().at(0);
			pending_command_instance->set_symbol_requirement(symbol);
			advance_command(std::move(pending_command_instance));
            invalidate_render();
        }

		pop_current_widget();
        });
}

void MainWidget::show_textbar(const std::wstring& command_name, bool should_fill_with_selected_text, const std::wstring& initial_value) {
    if (TOUCH_MODE) {
        QString init = "";

        if (should_fill_with_selected_text) {
            init = QString::fromStdWString(selected_text);
        }

        if (initial_value.size() > 0 ){
			init = QString::fromStdWString(initial_value);
		}

        TouchTextEdit* edit_widget = new TouchTextEdit(QString::fromStdWString(command_name), init, this);

        QObject::connect(edit_widget, &TouchTextEdit::confirmed, [&](QString text) {
				pop_current_widget();
                //setFocus();
                handle_pending_text_command(text.toStdWString());
            });

        QObject::connect(edit_widget, &TouchTextEdit::cancelled, [&]() {
            pop_current_widget();
            });

        edit_widget->resize(width() * 0.8, height() * 0.8);
        edit_widget->move(width() * 0.1, height() * 0.1);
        push_current_widget(edit_widget);
        show_current_widget();
    }
    else {
		text_command_line_edit->clear();
		if (should_fill_with_selected_text) {
			text_command_line_edit->setText(QString::fromStdWString(selected_text));
		}
		text_command_line_edit_label->setText(QString::fromStdWString(command_name));
		text_command_line_edit_container->show();
		text_command_line_edit->setFocus();
    }
}

bool MainWidget::helper_window_overlaps_main_window() {

    QRect main_window_rect = get_main_window_rect();
    QRect helper_window_rect = get_helper_window_rect();
    QRect intersection = main_window_rect.intersected(helper_window_rect);
    return intersection.width() > 50;
}

void MainWidget::toggle_window_configuration() {

    QWidget* helper_window = get_top_level_widget(helper_opengl_widget);
    QWidget* main_window = get_top_level_widget(opengl_widget);

    if (helper_opengl_widget->isVisible()) {
        apply_window_params_for_one_window_mode();
    }
    else {
        apply_window_params_for_two_window_mode();
        if (helper_window_overlaps_main_window()) {
            helper_window->activateWindow();
        }
        else {
            main_window->activateWindow();
        }


    }
}

void MainWidget::toggle_two_window_mode() {

    //main_widget.resize(window_width, window_height);

    QWidget* helper_window = get_top_level_widget(helper_opengl_widget);

    if (helper_opengl_widget->isHidden()) {

        QPoint pos = helper_opengl_widget->pos();
        QSize size = helper_opengl_widget->size();
        helper_window->show();
        helper_window->resize(size);
        helper_window->move(pos);

        if (!helper_window_overlaps_main_window()) {
            activateWindow();
        }
    }
    else {
        helper_window->hide();
    }
}

fz_stext_char* MainWidget::get_closest_character_to_cusrsor(QPoint pos){

    WindowPos window_pos = { pos.x(), pos.y() };
    DocumentPos doc_pos = main_document_view->window_to_document_pos(window_pos);
    int current_page = get_current_page_number();
    fz_stext_page* stext_page = doc()->get_stext_with_page_number(current_page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    int location_index = -1;

    fz_point doc_point;
    doc_point.x = doc_pos.x;
    doc_point.y = doc_pos.y;
    return find_closest_char_to_document_point(flat_chars, doc_point, &location_index);
}

std::optional<std::wstring> MainWidget::get_paper_name_under_cursor() {
	QPoint mouse_pos = mapFromGlobal(QCursor::pos());
    WindowPos window_pos = { mouse_pos.x(), mouse_pos.y() };
    auto normal_pos = main_document_view->window_to_normalized_window_pos(window_pos);

    if (opengl_widget->is_window_point_in_overview(normal_pos)) {
        auto [doc_page, doc_x, doc_y] = opengl_widget->window_pos_to_overview_pos(normal_pos);
        return main_document_view->get_document()->get_paper_name_at_position(doc_page, doc_x, doc_y);
    }
    else {
        DocumentPos doc_pos = main_document_view->window_to_document_pos(window_pos);
        return main_document_view->get_document()->
            get_paper_name_at_position(doc_pos.page, doc_pos.x, doc_pos.y);
    }
}

void MainWidget::smart_jump_under_pos(WindowPos pos){
    if (!main_document_view_has_document()) {
        return;
    }


    Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
    bool is_shift_pressed = modifiers.testFlag(Qt::ShiftModifier);

    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(pos);

    // if overview page is open and we middle click on a paper name, search it in a search engine
    if (opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
        auto [doc_page, doc_x, doc_y] = opengl_widget->window_pos_to_overview_pos({ normal_x, normal_y });
        std::optional<std::wstring> paper_name = main_document_view->get_document()->get_paper_name_at_position(doc_page, doc_x, doc_y);
        if (paper_name) {
            handle_paper_name_on_pointer(paper_name.value(), is_shift_pressed);
        }
        return;
    }

    auto [page, offset_x, offset_y] = main_document_view->window_to_document_pos(pos);

    fz_stext_page* stext_page = main_document_view->get_document()->get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);

    int target_page;
    float target_y_offset;
    if (find_location_of_text_under_pointer(pos, &target_page, &target_y_offset) != ReferenceType::None) {
        long_jump_to_destination(target_page, target_y_offset);
    }
    else {
		std::optional<std::wstring> paper_name_on_pointer = main_document_view->get_document()->get_paper_name_at_position(flat_chars, offset_x, offset_y);
		if (paper_name_on_pointer) {
			handle_paper_name_on_pointer(paper_name_on_pointer.value(), is_shift_pressed);
		}
    }

    //std::optional<std::pair<std::wstring, std::wstring>> generic_pair =\
    //        main_document_view->get_document()->get_generic_link_name_at_position(flat_chars, offset_x, offset_y);

    //std::optional<std::wstring> text_on_pointer = main_document_view->get_document()->get_text_at_position(flat_chars, offset_x, offset_y);
    //std::optional<std::wstring> reference_text_on_pointer = main_document_view->get_document()->get_reference_text_at_position(flat_chars, offset_x, offset_y);
    //std::optional<std::wstring> equation_text_on_pointer = main_document_view->get_document()->get_equation_text_at_position(flat_chars, offset_x, offset_y);

    //if (generic_pair) {
    //    int page;
    //    float y_offset;

    //    if (main_document_view->get_document()->find_generic_location(generic_pair.value().first,
    //                                                                  generic_pair.value().second,
    //                                                                  &page,
    //                                                                  &y_offset)) {

    //        long_jump_to_destination(page, y_offset);
    //        return;
    //    }
    //}
    //if (equation_text_on_pointer) {
    //    std::optional<IndexedData> eqdata_ = main_document_view->get_document()->find_equation_with_string(
    //                equation_text_on_pointer.value(),
    //                get_current_page_number());
    //    if (eqdata_) {
    //        IndexedData refdata = eqdata_.value();
    //        long_jump_to_destination(refdata.page, refdata.y_offset);
    //        return;
    //    }
    //}

    //if (reference_text_on_pointer) {
    //    std::optional<IndexedData> refdata_ = main_document_view->get_document()->find_reference_with_string(reference_text_on_pointer.value());
    //    if (refdata_) {
    //        IndexedData refdata = refdata_.value();
    //        long_jump_to_destination(refdata.page, refdata.y_offset);
    //        return;
    //    }

    //}
}

void MainWidget::visual_mark_under_pos(WindowPos pos){
    //float doc_x, doc_y;
    //int page;
    DocumentPos document_pos = main_document_view->window_to_document_pos(pos);
    if (document_pos.page != -1) {
        opengl_widget->set_should_draw_vertical_line(true);
        fz_pixmap* pixmap = main_document_view->get_document()->get_small_pixmap(document_pos.page);
        std::vector<unsigned int> hist = get_max_width_histogram_from_pixmap(pixmap);
        std::vector<unsigned int> line_locations;
        std::vector<unsigned int> _;
        get_line_begins_and_ends_from_histogram(hist, _, line_locations);
        int small_doc_x = static_cast<int>(document_pos.x * SMALL_PIXMAP_SCALE);
        int small_doc_y = static_cast<int>(document_pos.y * SMALL_PIXMAP_SCALE);
        int best_vertical_loc = find_best_vertical_line_location(pixmap, small_doc_x, small_doc_y);
        //int best_vertical_loc = line_locations[find_nth_larger_element_in_sorted_list(line_locations, static_cast<unsigned int>(small_doc_y), 2)];
        float best_vertical_loc_doc_pos = best_vertical_loc / SMALL_PIXMAP_SCALE;
        WindowPos window_pos = main_document_view->document_to_window_pos_in_pixels({document_pos.page, 0, best_vertical_loc_doc_pos});
        auto [abs_doc_x, abs_doc_y] = main_document_view->window_to_absolute_document_pos(window_pos);
        main_document_view->set_vertical_line_pos(abs_doc_y);
        int container_line_index = main_document_view->get_line_index_of_pos(document_pos);
        if (container_line_index == -1) {
			main_document_view->set_line_index(main_document_view->get_line_index_of_vertical_pos());
        }
        else {
			main_document_view->set_line_index(container_line_index);
        }
        validate_render();
    }
}


bool MainWidget::overview_under_pos(WindowPos pos){

    std::optional<PdfLink> link;
    smart_view_candidates.clear();
    index_into_candidates = 0;

    if (main_document_view && (link = main_document_view->get_link_in_pos(pos))) {
        if (QString::fromStdString(link.value().uri).startsWith("http")) {
            // can't open overview to web links
            return false;
        }
        else {
			set_overview_link(link.value());
			return true;
        }
    }

    int autoreference_page;
    float autoreference_offset;
    if (find_location_of_text_under_pointer(pos, &autoreference_page, &autoreference_offset, true) != ReferenceType::None) {
        set_overview_position(autoreference_page, autoreference_offset);
        return true;
    }

    return false;
}

void MainWidget::set_synctex_mode(bool mode) {
    if (mode) {
        this->opengl_widget->set_overview_page({});
    }
    this->synctex_mode = mode;
}

void MainWidget::toggle_synctex_mode(){
    this->set_synctex_mode(!this->synctex_mode);
}

void MainWidget::handle_portal() {
    if (!main_document_view_has_document()) return;

    if (is_pending_link_source_filled()) {
        auto [source_path, pl] = pending_link.value();
        pl.dst = main_document_view->get_checksum_state();

        if (source_path.has_value()) {
			add_portal(source_path.value(), pl);
        }

        pending_link = {};
    }
	else {
		pending_link = std::make_pair<std::wstring, Portal>(main_document_view->get_document()->get_path(),
			Portal::with_src_offset(main_document_view->get_offset_y()));
	}

	synchronize_pending_link();
	refresh_all_windows();
    validate_render();
}

void MainWidget::handle_pending_text_command(std::wstring text) {
    if (pending_command_instance) {
        pending_command_instance->set_text_requirement(text);
        advance_command(std::move(pending_command_instance));
    }
}

void MainWidget::toggle_fullscreen() {
    if (isFullScreen()) {
        helper_opengl_widget->setWindowState(Qt::WindowState::WindowMaximized);
        setWindowState(Qt::WindowState::WindowMaximized);
    }
    else {
        helper_opengl_widget->setWindowState(Qt::WindowState::WindowFullScreen);
        setWindowState(Qt::WindowState::WindowFullScreen);
    }
}

void MainWidget::toggle_presentation_mode() {
    if (opengl_widget->is_presentation_mode()) {
        set_presentation_mode(false);
    }
    else {
        set_presentation_mode(true);
    }
}

void MainWidget::set_presentation_mode(bool mode) {
    if (mode) {
        opengl_widget->set_visible_page_number(get_current_page_number());
    }
    else {
        opengl_widget->set_visible_page_number({});
    }
}

void MainWidget::complete_pending_link(const PortalViewState& destination_view_state) {
    Portal& pl = pending_link.value().second;
    pl.dst = destination_view_state;
    main_document_view->get_document()->add_portal(pl);
    pending_link = {};
}

void MainWidget::long_jump_to_destination(int page, float offset_y) {
    long_jump_to_destination({ page, main_document_view->get_offset_x(), offset_y });
}


void MainWidget::long_jump_to_destination(float abs_offset_y) {

    auto [page, _, offset_y] = main_document_view->get_document()->absolute_to_page_pos(
        { main_document_view->get_offset_x(), abs_offset_y });
    long_jump_to_destination(page, offset_y);
}

void MainWidget::long_jump_to_destination(DocumentPos pos) {
    if (!is_pending_link_source_filled()) {
        push_state();
        main_document_view->goto_offset_within_page({ pos.page, pos.x, pos.y });
    }
    else {
        // if we press the link button and then click on a pdf link, we automatically link to the
        // link's destination

        PortalViewState dest_state;
        dest_state.document_checksum = main_document_view->get_document()->get_checksum();
        dest_state.book_state.offset_x = pos.x;
        dest_state.book_state.offset_y = main_document_view->get_page_offset(pos.page) + pos.y;
        dest_state.book_state.zoom_level = main_document_view->get_zoom_level();

        complete_pending_link(dest_state);
    }
    invalidate_render();
}

void MainWidget::set_current_widget(QWidget* new_widget) {
    for (auto widget : current_widget_stack) {
        widget->hide();
        widget->deleteLater();
    }
    current_widget_stack.clear();
    current_widget_stack.push_back(new_widget);
    //if (current_widget != nullptr) {
    //    current_widget->hide();
    //    garbage_widgets.push_back(current_widget);
    //}
    //current_widget = new_widget;

    //if (garbage_widgets.size() > 2) {
    //    delete garbage_widgets[0];
    //    garbage_widgets.erase(garbage_widgets.begin());
    //}
}

void MainWidget::push_current_widget(QWidget* new_widget) {
	if (current_widget_stack.size() > 0) {
		current_widget_stack.back()->hide();
	}
	current_widget_stack.push_back(new_widget);
}

void MainWidget::pop_current_widget() {
	if (current_widget_stack.size() > 0) {
		current_widget_stack.back()->hide();
		current_widget_stack.back()->deleteLater();
		current_widget_stack.pop_back();
	}
	if (current_widget_stack.size() > 0) {
		current_widget_stack.back()->show();
	}
}

bool MainWidget::focus_on_visual_mark_pos(bool moving_down) {
    //float window_x, window_y;

    if (!main_document_view->get_ruler_window_rect().has_value()) {
		return false;
    }

    float thresh = 1 - VISUAL_MARK_NEXT_PAGE_THRESHOLD;
    fz_rect ruler_window_rect = main_document_view->get_ruler_window_rect().value();
    //main_document_view->absolute_to_window_pos(0, main_document_view->get_vertical_line_pos(), &window_x, &window_y);
    //if ((window_y < -thresh) || (window_y > thresh)) {
    if (ruler_window_rect.y0 < -1 || ruler_window_rect.y1 > 1) {
        main_document_view->goto_vertical_line_pos();
        return true;
    }
    if ((moving_down && (ruler_window_rect.y0 < -thresh)) || ((!moving_down) && (ruler_window_rect.y1 > thresh))) {
        main_document_view->goto_vertical_line_pos();
        //float distance = (main_document_view->get_view_height() / main_document_view->get_zoom_level()) * VISUAL_MARK_NEXT_PAGE_FRACTION;
        //main_document_view->move_absolute(0, -distance);
        return true;
    }
    return false;
}

void MainWidget::toggle_visual_scroll_mode() {
    visual_scroll_mode = !visual_scroll_mode;
}

std::optional<std::wstring> MainWidget::get_current_file_name() {
    if (main_document_view) {
        if (main_document_view->get_document()) {
            return main_document_view->get_document()->get_path();
        }
    }
    return {};
}

CommandManager* MainWidget::get_command_manager(){
    return command_manager;
}

void MainWidget::toggle_dark_mode() {
    this->opengl_widget->toggle_dark_mode();
}

void MainWidget::execute_command(std::wstring command, std::wstring text, bool wait) {

    std::wstring file_path = main_document_view->get_document()->get_path();
    QString qfile_path = QString::fromStdWString(file_path);
    std::vector<std::wstring> path_parts;
    split_path(file_path, path_parts);
    std::wstring file_name = path_parts.back();
    QString qfile_name = QString::fromStdWString(file_name);

    QString qtext = QString::fromStdWString(command);

    qtext.arg(qfile_path);

#ifdef SIOYEK_QT6
    QStringList command_parts_ = qtext.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#else
    QStringList command_parts_ = qtext.split(QRegExp("\\s+"), QString::SkipEmptyParts);
#endif

    QStringList command_parts;
    while (command_parts_.size() > 0) {
        if ((command_parts_.size() <= 1) || (!command_parts_.at(0).endsWith("\\"))) {
            command_parts.append(command_parts_.at(0));
            command_parts_.pop_front();
        }
        else {
            QString first_part = command_parts_.at(0);
            QString second_part = command_parts_.at(1);
            QString new_command_part = first_part.left(first_part.size() - 1) + " " + second_part;
            command_parts_.pop_front();
            command_parts_.replace(0, new_command_part);
        }
    }
    if (command_parts.size() > 0) {
        QString command_name = command_parts[0];
        QStringList command_args;

        command_parts.takeFirst();

		QPoint mouse_pos_ = mapFromGlobal(QCursor::pos());
        WindowPos mouse_pos = { mouse_pos_.x(), mouse_pos_.y() };
        DocumentPos mouse_pos_document = main_document_view->window_to_document_pos(mouse_pos);

        for (int i = 0; i < command_parts.size(); i++) {
            // lagacy number macros, now replaced with names ones
            command_parts[i].replace("%1", qfile_path);
            command_parts[i].replace("%2", qfile_name);
            command_parts[i].replace("%3", QString::fromStdWString(selected_text));
            command_parts[i].replace("%4", QString::number(get_current_page_number()));
            command_parts[i].replace("%5", QString::fromStdWString(text));

            // new named macros
            command_parts[i].replace("%{file_path}", qfile_path);
            command_parts[i].replace("%{file_name}", qfile_name);
            command_parts[i].replace("%{selected_text}", QString::fromStdWString(selected_text));
            if (selected_text.size() > 0) {
                auto selection_begin_document = main_document_view->get_document()->absolute_to_page_pos(selection_begin);
                command_parts[i].replace("%{selection_begin_document}",
                        QString::number(selection_begin_document.page) + " " + QString::number(selection_begin_document.x) + " " + QString::number(selection_begin_document.y));
                auto selection_end_document = main_document_view->get_document()->absolute_to_page_pos(selection_end);
                command_parts[i].replace("%{selection_end_document}",
                        QString::number(selection_end_document.page) + " " + QString::number(selection_end_document.x) + " " + QString::number(selection_end_document.y));
            }
            command_parts[i].replace("%{page_number}", QString::number(get_current_page_number()));
            command_parts[i].replace("%{command_text}", QString::fromStdWString(text));


            command_parts[i].replace("%{mouse_pos_window}", QString::number(mouse_pos.x) + " " + QString::number(mouse_pos.y));
            //command_parts[i].replace("%{mouse_pos_window}", QString("%1 %2").arg(mouse_pos.x, mouse_pos.y));
            command_parts[i].replace("%{mouse_pos_document}", QString::number(mouse_pos_document.page) + " " + QString::number(mouse_pos_document.x) + " " + QString::number(mouse_pos_document.y));
            //command_parts[i].replace("%{mouse_pos_document}", QString("%1 %2 %3").arg(mouse_pos_document.page, mouse_pos_document.x, mouse_pos_document.y));
            if (command_parts[i].indexOf("%{paper_name}") != -1) {
                std::optional<std::wstring> maybe_paper_name = get_paper_name_under_cursor();
                if (maybe_paper_name) {
                    command_parts[i].replace("%{paper_name}", QString::fromStdWString(maybe_paper_name.value()));
                }
            }

            command_parts[i].replace("%{sioyek_path}", QCoreApplication::applicationFilePath());
            command_parts[i].replace("%{local_database}", QString::fromStdWString(local_database_file_path.get_path()));
            command_parts[i].replace("%{shared_database}", QString::fromStdWString(global_database_file_path.get_path()));

            int selected_rect_page = -1;
            fz_rect selected_rect_rect;
            if (get_selected_rect_document(selected_rect_page, selected_rect_rect)) {
                QString format_string = "%1,%2,%3,%4,%5";
                QString rect_string = format_string
                    .arg(QString::number(selected_rect_page))
                    .arg(QString::number(selected_rect_rect.x0))
                    .arg(QString::number(selected_rect_rect.y0))
                    .arg(QString::number(selected_rect_rect.x1))
                    .arg(QString::number(selected_rect_rect.y1));
				command_parts[i].replace("%{selected_rect}", rect_string);
            }


            std::wstring selected_line_text;
            if (main_document_view) {
                selected_line_text = main_document_view->get_selected_line_text().value_or(L"");
				command_parts[i].replace("%{zoom_level}", QString::number(main_document_view->get_zoom_level()));
            }

            if (selected_line_text.size() > 0) {
				command_parts[i].replace("%6", QString::fromStdWString(selected_line_text));
				command_parts[i].replace("%{line_text}", QString::fromStdWString(selected_line_text));
			}

            std::wstring command_parts_ = command_parts[i].toStdWString();
            command_args.push_back(command_parts[i]);
        }

        run_command(command_name.toStdWString(), command_args, wait);
    }

}
void MainWidget::handle_paper_name_on_pointer(std::wstring paper_name, bool is_shift_pressed) {
    if (paper_name.size() > 5) {
        char type;
        if (is_shift_pressed) {
            type = SHIFT_MIDDLE_CLICK_SEARCH_ENGINE[0];
        }
        else {
            type = MIDDLE_CLICK_SEARCH_ENGINE[0];
        }
        if ((type >= 'a') && (type <= 'z')) {
            search_custom_engine(paper_name, SEARCH_URLS[type - 'a']);
        }
    }

}
void MainWidget::move_vertical(float amount) {
    if (!smooth_scroll_mode) {
		move_document(0, amount);
		validate_render();
    }
    else {
        smooth_scroll_speed += amount * SMOOTH_SCROLL_SPEED;
		validate_render();
    }
}

void MainWidget::zoom(WindowPos pos, float zoom_factor, bool zoom_in) {
    last_smart_fit_page = {};
    if (zoom_in) {
        if (WHEEL_ZOOM_ON_CURSOR) {
            main_document_view->zoom_in_cursor(pos, zoom_factor);
        }
        else {
            main_document_view->zoom_in(zoom_factor);
        }
    } else {
        if (WHEEL_ZOOM_ON_CURSOR) {
            main_document_view->zoom_out_cursor(pos, zoom_factor);
        }
        else {
            main_document_view->zoom_out(zoom_factor);
        }
    }
    validate_render();
}

bool MainWidget::move_horizontal(float amount, bool force){
    if (!horizontal_scroll_locked) {
        bool ret = move_document(amount, 0, force);
        validate_render();
        return ret;
    }
    return true;
}

std::optional<std::string> MainWidget::get_last_opened_file_checksum() {

    std::vector<std::wstring> opened_docs_hashes;
    std::wstring current_checksum = L"";
    if (main_document_view_has_document()) {
        current_checksum = utf8_decode(main_document_view->get_document()->get_checksum());
    }

    db_manager->select_opened_books_path_values(opened_docs_hashes);

    size_t index = 0;
    while (index < opened_docs_hashes.size()) {
        if (opened_docs_hashes[index] == current_checksum) {
            index++;
        }
        else {
            return utf8_encode(opened_docs_hashes[index]);
        }
    }

    return {};
}
void MainWidget::get_window_params_for_one_window_mode(int* main_window_size, int* main_window_move){
    if (SINGLE_MAIN_WINDOW_SIZE[0] >= 0) {
        main_window_size[0] = SINGLE_MAIN_WINDOW_SIZE[0];
        main_window_size[1] = SINGLE_MAIN_WINDOW_SIZE[1];
        main_window_move[0] = SINGLE_MAIN_WINDOW_MOVE[0];
        main_window_move[1] = SINGLE_MAIN_WINDOW_MOVE[1];
    }
    else{
#ifdef SIOYEK_QT6
        int window_width = QGuiApplication::primaryScreen()->geometry().width();
        int window_height = QGuiApplication::primaryScreen()->geometry().height();
#else
        int window_width = QApplication::desktop()->screenGeometry(0).width();
        int window_height = QApplication::desktop()->screenGeometry(0).height();
#endif

        main_window_size[0] = window_width;
        main_window_size[1] = window_height;
        main_window_move[0] = 0;
        main_window_move[1] = 0;
    }
}
void MainWidget::get_window_params_for_two_window_mode(int* main_window_size, int* main_window_move, int* helper_window_size, int* helper_window_move) {
    if (MAIN_WINDOW_SIZE[0] >= 0) {
        main_window_size[0] = MAIN_WINDOW_SIZE[0];
        main_window_size[1] = MAIN_WINDOW_SIZE[1];
        main_window_move[0] = MAIN_WINDOW_MOVE[0];
        main_window_move[1] = MAIN_WINDOW_MOVE[1];
        helper_window_size[0] = HELPER_WINDOW_SIZE[0];
        helper_window_size[1] = HELPER_WINDOW_SIZE[1];
        helper_window_move[0] = HELPER_WINDOW_MOVE[0];
        helper_window_move[1] = HELPER_WINDOW_MOVE[1];
    }
    else {
#ifdef SIOYEK_QT6
        int num_screens = QGuiApplication::screens().size();
#else
        int num_screens = QApplication::desktop()->numScreens();
#endif
        int main_window_width = get_current_monitor_width();
        int main_window_height = get_current_monitor_height();
        if (num_screens > 1) {
#ifdef SIOYEK_QT6
            int second_window_width = QGuiApplication::screens().at(1)->geometry().width();
            int second_window_height = QGuiApplication::screens().at(1)->geometry().height();
#else
            int second_window_width = QApplication::desktop()->screenGeometry(1).width();
            int second_window_height = QApplication::desktop()->screenGeometry(1).height();
#endif
            main_window_size[0] = main_window_width;
            main_window_size[1] = main_window_height;
            main_window_move[0] = 0;
            main_window_move[1] = 0;
            helper_window_size[0] = second_window_width;
            helper_window_size[1] = second_window_height;
            helper_window_move[0] = main_window_width;
            helper_window_move[1] = 0;
        }
        else {
            main_window_size[0] = main_window_width / 2;
            main_window_size[1] = main_window_height;
            main_window_move[0] = 0;
            main_window_move[1] = 0;
            helper_window_size[0] = main_window_width / 2;
            helper_window_size[1] = main_window_height;
            helper_window_move[0] = main_window_width / 2;
            helper_window_move[1] = 0;
        }
    }
}

void MainWidget::apply_window_params_for_one_window_mode(bool force_resize){

    QWidget* main_window = get_top_level_widget(opengl_widget);

    int main_window_width = get_current_monitor_width();

    int main_window_size[2];
    int main_window_move[2];

    get_window_params_for_one_window_mode(main_window_size, main_window_move);

    bool should_maximize = main_window_width == main_window_size[0];

    if (should_maximize) {
        main_window->move(main_window_move[0], main_window_move[1]);
        main_window->hide();
        if (force_resize) {
			main_window->resize(main_window_size[0], main_window_size[1]);
        }
        main_window->showMaximized();
    }
    else {
        main_window->move(main_window_move[0], main_window_move[1]);
        main_window->resize(main_window_size[0], main_window_size[1]);
    }


    if (helper_opengl_widget != nullptr) {
        helper_opengl_widget->hide();
        helper_opengl_widget->move(0, 0);
        helper_opengl_widget->resize(main_window_size[0], main_window_size[1]);
    }
}

void MainWidget::apply_window_params_for_two_window_mode() {
    QWidget* main_window = get_top_level_widget(opengl_widget);
    QWidget* helper_window = get_top_level_widget(helper_opengl_widget);

    //int main_window_width = QApplication::desktop()->screenGeometry(0).width();
    int main_window_width = get_current_monitor_width();

    int main_window_size[2];
    int main_window_move[2];
    int helper_window_size[2];
    int helper_window_move[2];

    get_window_params_for_two_window_mode(main_window_size, main_window_move, helper_window_size, helper_window_move);

    bool should_maximize = main_window_width == main_window_size[0];


    if (helper_opengl_widget != nullptr) {
        helper_window->move(helper_window_move[0], helper_window_move[1]);
        helper_window->resize(helper_window_size[0], helper_window_size[1]);
        helper_window->show();
    }

    if (should_maximize) {
        main_window->hide();
        main_window->showMaximized();
    }
    else {
		main_window->move(main_window_move[0], main_window_move[1]);
		main_window->resize(main_window_size[0], main_window_size[1]);
    }
}

QRect MainWidget::get_main_window_rect() {
    QPoint main_window_pos = pos();
    QSize main_window_size = size();
    return QRect(main_window_pos, main_window_size);
}

QRect MainWidget::get_helper_window_rect() {
    QPoint helper_window_pos = helper_opengl_widget->pos();
    QSize helper_window_size = helper_opengl_widget->size();
    return QRect(helper_window_pos, helper_window_size);
}

void MainWidget::open_document(const std::wstring& doc_path,
    bool* invalid_flag,
    bool load_prev_state,
    std::optional<OpenedBookState> prev_state,
    bool force_load_dimensions) {
    opengl_widget->clear_all_selections();

    if (main_document_view) {
        main_document_view->persist();
    }

    main_document_view->open_document(doc_path, invalid_flag, load_prev_state, prev_state, force_load_dimensions);

    std::optional<std::wstring> filename = Path(doc_path).filename();
    if (filename) {
        setWindowTitle(QString::fromStdWString(filename.value()));
    }
}

#ifndef Q_OS_MACOS
void MainWidget::dragEnterEvent(QDragEnterEvent* e)
{
	e->acceptProposedAction();

}

void MainWidget::dragMoveEvent(QDragMoveEvent* e)
{
	e->acceptProposedAction();

}

void MainWidget::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        auto urls = event->mimeData()->urls();
        std::wstring path = urls.at(0).toLocalFile().toStdWString();
        // ignore file:/// at the beginning of the URL
//#ifdef Q_OS_WIN
//        path = path.substr(8, path.size() - 8);
//#else
//        path = path.substr(7, path.size() - 7);
//#endif
        //handle_args(QStringList() << QApplication::applicationFilePath() << QString::fromStdWString(path));
        push_state();
        open_document(path, &is_render_invalidated);
    }
}
#endif

void MainWidget::highlight_words() {

    int page = get_current_page_number();
    fz_stext_page* stext_page = main_document_view->get_document()->get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    std::vector<fz_rect> word_rects;
    std::vector<std::pair<fz_rect, int>> word_rects_with_page;
    std::vector<std::pair<fz_rect, int>> visible_word_rects;

    get_flat_chars_from_stext_page(stext_page, flat_chars);
    get_flat_words_from_flat_chars(flat_chars, word_rects);
    for (auto rect : word_rects) {
        word_rects_with_page.push_back(std::make_pair(rect, page));
    }

    for (auto [rect, page] : word_rects_with_page) {
        if (is_rect_visible(page, rect)) {
			visible_word_rects.push_back(std::make_pair(rect, page));
		}
    }

    opengl_widget->set_highlight_words(visible_word_rects);
    opengl_widget->set_should_highlight_words(true);

}

std::vector<fz_rect> MainWidget::get_flat_words(std::vector<std::vector<fz_rect>>* flat_word_chars) {
    int page = get_current_page_number();
    auto res = main_document_view->get_document()->get_page_flat_words(page);
    if (flat_word_chars != nullptr) {
        *flat_word_chars = main_document_view->get_document()->get_page_flat_word_chars(page);
    }
    return res;
}

std::optional<fz_irect> MainWidget::get_tag_window_rect(std::string tag, std::vector<fz_irect>* char_rects) {

    int page = get_current_page_number();
    std::vector<fz_rect> word_char_rects;
    std::optional<fz_rect> rect = get_tag_rect(tag, &word_char_rects);

    if (rect.has_value()) {

        fz_irect window_rect = main_document_view->document_to_window_irect(page, rect.value());
        if (char_rects != nullptr) {
            for (auto c : word_char_rects) {
				char_rects->push_back(main_document_view->document_to_window_irect(page, c));
            }
        }
		return window_rect;
    }
    return {};
}

std::optional<fz_rect> MainWidget::get_tag_rect(std::string tag, std::vector<fz_rect>* word_chars) {

    int page = get_current_page_number();
    std::vector<std::vector<fz_rect>> all_word_chars;
    std::vector<fz_rect> word_rects;
    if (word_chars == nullptr) {
		word_rects = get_flat_words(nullptr);
    }
    else {
		word_rects = get_flat_words(&all_word_chars);
    }

    std::vector<std::vector<fz_rect>> visible_word_chars;
    std::vector<fz_rect> visible_word_rects;

    for (int i = 0; i < word_rects.size(); i++) {
        if (is_rect_visible(page, word_rects[i])) {
            visible_word_rects.push_back(word_rects[i]);
            if (word_chars != nullptr) {
                visible_word_chars.push_back(all_word_chars[i]);
            }
        }
    }

	int index = get_index_from_tag(tag);
    if (index < visible_word_rects.size()) {
        if (word_chars != nullptr) {
            *word_chars = visible_word_chars[index];
        }
		return visible_word_rects[index];
    }
    return {};
}

bool MainWidget::is_rotated() {
    return opengl_widget->is_rotated();
}

void MainWidget::show_password_prompt_if_required() {
	if (main_document_view && (main_document_view->get_document() != nullptr)) {
		if (main_document_view->get_document()->needs_authentication()) {
            if ((pending_command_instance == nullptr) || (pending_command_instance->get_name() != "enter_password")) {
				handle_command_types(command_manager->get_command_with_name("enter_password"), 1);
			}
		}
	}
}

void MainWidget::on_new_paper_added(const std::wstring& file_path) {
    if (is_pending_link_source_filled()) {
        PortalViewState dst_view_state;

        dst_view_state.book_state.offset_x = 0;
        dst_view_state.book_state.offset_y = 0;
        dst_view_state.book_state.zoom_level = 1;
        Document* new_doc = document_manager->get_document(file_path);
        new_doc->open(nullptr, false, "", true);
        fz_rect first_page_rect = new_doc->get_page_rect_no_cache(0);
        document_manager->free_document(new_doc);
        float first_page_width = first_page_rect.x1 - first_page_rect.x0;
        float first_page_height = first_page_rect.y1 - first_page_rect.y0;

        dst_view_state.document_checksum = checksummer->get_checksum(file_path);

        if (helper_document_view) {
            float helper_view_width = helper_document_view->get_view_width();
            float helper_view_height = helper_document_view->get_view_height();
            float zoom_level = helper_view_width / first_page_width;
            dst_view_state.book_state.zoom_level = zoom_level;
			dst_view_state.book_state.offset_y = -std::abs(-helper_view_height / zoom_level / 2 + first_page_height / 2);
        }
        complete_pending_link(dst_view_state);
        invalidate_render();
    }
}
void MainWidget::handle_link_click(const PdfLink& link) {

	if (link.uri.substr(0, 4).compare("http") == 0) {
		open_web_url(utf8_decode(link.uri));
		return;
	}

	if (link.uri.substr(0, 4).compare("file") == 0) {
        QString path_uri = QString::fromStdString(link.uri.substr(7, link.uri.size() - 7));
        auto parts = path_uri.split('#');
        std::wstring path_part = parts.at(0).toStdWString();
        auto docpath = doc()->get_path();
        Path linked_file_path = Path(doc()->get_path()).file_parent().slash(path_part);
        int page = 0;
        if (parts.size() > 0) {
            std::string page_string = parts.at(1).toStdString();
            page_string = page_string.substr(5, page_string.size() - 5);
            page = QString::fromStdString(page_string).toInt() - 1;
        }
        push_state();
        open_document_at_location(linked_file_path, page, {}, {}, {});
		return;
	}

	auto [page, offset_x, offset_y] = parse_uri(mupdf_context, link.uri);

	// convert one indexed page to zero indexed page
	page--;

	// we usually just want to center the y offset and not the x offset (otherwise for example
	// a link at the right side of the screen will be centered, causing most of screen state to be empty)
	offset_x = main_document_view->get_offset_x();

    long_jump_to_destination({ page, offset_x, offset_y });
}

void MainWidget::save_auto_config() {
    std::wofstream outfile(auto_config_path.get_path_utf8());
    outfile << get_serialized_configuration_string();
    outfile.close();
}

std::wstring MainWidget::get_serialized_configuration_string() {
    float overview_size[2];
    float overview_offset[2];
    opengl_widget->get_overview_offsets(&overview_offset[0], &overview_offset[1]);
    opengl_widget->get_overview_size(&overview_size[0], &overview_size[1]);

    QString overview_config = "overview_size %1 %2\noverview_offset %3 %4\n";
    std::wstring overview_config_string = overview_config.arg(QString::number(overview_size[0]),
        QString::number(overview_size[1]),
        QString::number(overview_offset[0]),
        QString::number(overview_offset[1])).toStdWString();
    return overview_config_string + get_window_configuration_string();
}
std::wstring MainWidget::get_window_configuration_string() {

	QString config_string_multi = "main_window_size    %1 %2\nmain_window_move     %3 %4\nhelper_window_size    %5 %6\nhelper_window_move     %7 %8";
	QString config_string_single = "single_main_window_size    %1 %2\nsingle_main_window_move     %3 %4";

	QString main_window_size_w = QString::number(size().width());
	QString main_window_size_h = QString::number(size().height());
	QString helper_window_size_w = QString::number(-1);
	QString helper_window_size_h = QString::number(-1);
	QString main_window_move_x = QString::number(pos().x());
	QString main_window_move_y = QString::number(pos().y());
	QString helper_window_move_x = QString::number(-1);
	QString helper_window_move_y = QString::number(-1);

	if ((helper_opengl_widget != nullptr) && helper_opengl_widget->isVisible()) {
		helper_window_size_w = QString::number(helper_opengl_widget->size().width());
		helper_window_size_h = QString::number(helper_opengl_widget->size().height());
		helper_window_move_x = QString::number(helper_opengl_widget->pos().x());
		helper_window_move_y = QString::number(helper_opengl_widget->pos().y());
		return (config_string_multi.arg(main_window_size_w,
			main_window_size_h,
			main_window_move_x,
			main_window_move_y,
			helper_window_size_w,
			helper_window_size_h,
			helper_window_move_x,
			helper_window_move_y).toStdWString());
	}
	else {
		return (config_string_single.arg(main_window_size_w,
			main_window_size_h,
			main_window_move_x,
			main_window_move_y).toStdWString());
	}
}

void MainWidget::handle_close_event() {
    save_auto_config();
#ifndef SIOYEK_ANDROID
    persist();
#endif

	// we need to delete this here (instead of destructor) to ensure that application
	// closes immediately after the main window is closed
	delete helper_opengl_widget;
    helper_opengl_widget = nullptr;
}

Document* MainWidget::doc() {
    return main_document_view->get_document();
}

void MainWidget::return_to_last_visual_mark() {
	main_document_view->goto_vertical_line_pos();
	opengl_widget->set_should_draw_vertical_line(true);
    pending_command_instance = nullptr;
	validate_render();
}

void MainWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMaximized() ) {
            //int width = size().width();
            //int height = size().height();
            //main_window_width = get_current_monitor_width();
            //main_window_height = get_current_monitor_height();
		}
    }
    QWidget::changeEvent(event);
}

void MainWidget::move_visual_mark_next() {
    opengl_widget->clear_underline();

    int prev_line_index = main_document_view->get_line_index();
    int vertical_line_page = main_document_view->get_vertical_line_page();
    int current_line_index, current_page;

    fz_rect current_ruler_rect = doc()->get_ith_next_line_from_absolute_y(vertical_line_page, prev_line_index, 0, true, &current_line_index, &current_page);
    current_ruler_rect = main_document_view->absolute_to_window_rect(current_ruler_rect);

    if (current_ruler_rect.x1 <= 1.0f) {
        fz_rect new_rect = move_visual_mark(1);
		fz_rect new_ruler_rect = main_document_view->absolute_to_window_rect(new_rect);
        if (new_ruler_rect.x0 < -1) {
            float offset = (new_ruler_rect.x0 + 0.9f) * main_window_width / 2;
            move_horizontal(-offset);
        }

        if (new_ruler_rect.x0 > 1) {
            float offset = (new_ruler_rect.x0 - 0.1f) * main_window_width / 2;
            move_horizontal(-offset);
        }

    }
    else {
        
        WindowPos pos;

        pos.x = main_window_width;
        pos.y = main_window_height - static_cast<int>((current_ruler_rect.y1 + 1) * main_window_height / 2);

        AbsoluteDocumentPos abspos = main_document_view->window_to_absolute_document_pos(pos);

        if (false) {
			bool is_truncated = move_horizontal(-static_cast<float>(main_window_width));

			if (is_truncated) {
				opengl_widget->set_underline(abspos);
			}
        }
        else {
			 move_horizontal(-static_cast<float>(main_window_width), true);
        }

    }
}

void MainWidget::move_visual_mark_prev() {
    int prev_line_index = main_document_view->get_line_index();
    int vertical_line_page = main_document_view->get_vertical_line_page();
    int current_line_index, current_page;

    fz_rect current_ruler_rect = doc()->get_ith_next_line_from_absolute_y(vertical_line_page, prev_line_index, 0, true, &current_line_index, &current_page);
    current_ruler_rect = main_document_view->absolute_to_window_rect(current_ruler_rect);

    if (current_ruler_rect.x0 >= -1.0f) {
        fz_rect new_rect = move_visual_mark(-1);
		fz_rect new_ruler_rect = main_document_view->absolute_to_window_rect(new_rect);
        if (new_ruler_rect.x1 > 1) {
            float offset = (new_ruler_rect.x1 - 0.9f) * main_window_width / 2;
            move_horizontal(-offset); //todo: fix this
        }

    }
    else {
        move_horizontal(static_cast<float>(main_window_width));
    }
}

fz_rect MainWidget::move_visual_mark(int offset) {
    bool moving_down = offset >= 0;

    int prev_line_index = main_document_view->get_line_index();
    int new_line_index, new_page;
    int vertical_line_page = main_document_view->get_vertical_line_page();
    fz_rect ruler_rect = doc()->get_ith_next_line_from_absolute_y(vertical_line_page, prev_line_index, offset, true, &new_line_index, &new_page);
    main_document_view->set_line_index(new_line_index);
	main_document_view->set_vertical_line_rect(ruler_rect);
	if (focus_on_visual_mark_pos(moving_down)) {
		float distance = (main_document_view->get_view_height() / main_document_view->get_zoom_level()) * VISUAL_MARK_NEXT_PAGE_FRACTION / 2;
		main_document_view->move_absolute(0, distance);
	}
    if (is_reading) {
        read_current_line();
    }
    return ruler_rect;
}

bool MainWidget::is_visual_mark_mode() {
    return opengl_widget->get_should_draw_vertical_line();
}

void MainWidget::scroll_overview(int amount) {
    float vertical_move_amount = VERTICAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY * SCROLL_VIEW_SENSITIVITY;
	OverviewState state = opengl_widget->get_overview_page().value();
	state.absolute_offset_y += 36.0f * vertical_move_amount * amount;
	opengl_widget->set_overview_page(state);
    handle_portal_overview_update();
}

std::wstring MainWidget::get_current_page_label() { 
    return doc()->get_page_label(main_document_view->get_center_page_number());
}
int MainWidget::get_current_page_number() const {
    //
    if (opengl_widget->get_should_draw_vertical_line()) {
        return main_document_view->get_vertical_line_page();
    }
    else {
        return main_document_view->get_center_page_number();
    }
}

void MainWidget::set_inverse_search_command(const std::wstring& new_command) {
    inverse_search_command = new_command;
}

void MainWidget::focusInEvent(QFocusEvent* ev) {
    int index = -1;
    for (size_t i = 0; i < windows.size(); i++) {
        if (windows[i] == this) {
			index = i;
			break;
        }
    }
    if (index > 0) {
        std::swap(windows[0], windows[index]);
    }
}

void MainWidget::toggle_statusbar() {
    should_show_status_label_ = !should_show_status_label_;

    if (!should_show_status_label()) {
        status_label->hide();
    }
    else {
        status_label->show();
    }
}

void MainWidget::toggle_titlebar() {

    Qt::WindowFlags window_flags = windowFlags();
    if (window_flags.testFlag(Qt::FramelessWindowHint)) {
        setWindowFlag(Qt::FramelessWindowHint, false);
    }
    else {
        setWindowFlag(Qt::FramelessWindowHint, true);
    }
    show();
}

void MainWidget::focus_text(int page, const std::wstring& text) {
    std::vector<std::wstring> line_texts;
    std::vector<fz_rect> line_rects;
    line_rects = main_document_view->get_document()->get_page_lines(page, &line_texts);

    std::string encoded_text = utf8_encode(text);

    int max_score = -1;
    int max_index = -1;

    for (int i = 0; i < line_texts.size(); i++) {
        std::string encoded_line = utf8_encode(line_texts[i]);
        int score = lcs(encoded_text.c_str(), encoded_line.c_str(), encoded_text.size(), encoded_line.size());
        //fts::fuzzy_match(encoded_line.c_str(), encoded_text.c_str(), score);
        if (score > max_score) {
            max_index = i;
            max_score = score;
        }
    }

    if (max_index < line_rects.size()) {
		main_document_view->set_line_index(max_index);
		main_document_view->set_vertical_line_rect(line_rects[max_index]);
		if (focus_on_visual_mark_pos(true)) {
			float distance = (main_document_view->get_view_height() / main_document_view->get_zoom_level()) * VISUAL_MARK_NEXT_PAGE_FRACTION / 2;
			main_document_view->move_absolute(0, distance);
		}
    }
}

int MainWidget::get_current_monitor_width() {
    if (this->window()->windowHandle() != nullptr) {
		return this->window()->windowHandle()->screen()->geometry().width();
    }
    else {
#ifdef SIOYEK_QT6
        return QGuiApplication::primaryScreen()->geometry().width();
#else
		return QApplication::desktop()->screenGeometry(0).width();
#endif
    }
}

int MainWidget::get_current_monitor_height() {
    if (this->window()->windowHandle() != nullptr) {
		return this->window()->windowHandle()->screen()->geometry().height();
    }
    else {
#ifdef SIOYEK_QT6
        return QGuiApplication::primaryScreen()->geometry().height();
#else
		return QApplication::desktop()->screenGeometry(0).height();
#endif
    }
}

void MainWidget::reload() {
    pdf_renderer->delete_old_pages(true, true);
    if (doc()) {
		doc()->reload();
    }
}


void MainWidget::synctex_under_pos(WindowPos position) {
#ifndef SIOYEK_ANDROID
	auto [page, doc_x, doc_y] = main_document_view->window_to_document_pos(position);
	std::wstring docpath = main_document_view->get_document()->get_path();
	std::string docpath_utf8 = utf8_encode(docpath);
	synctex_scanner_p scanner = synctex_scanner_new_with_output_file(docpath_utf8.c_str(), nullptr, 1);

	int stat = synctex_edit_query(scanner, page + 1, doc_x, doc_y);

	if (stat > 0) {
		synctex_node_p node;
		while ((node = synctex_scanner_next_result(scanner))) {
			int line = synctex_node_line(node);
			int column = synctex_node_column(node);
			if (column < 0) column = 0;
			int tag = synctex_node_tag(node);
			const char* file_name = synctex_scanner_get_name(scanner, tag);
#ifdef Q_OS_WIN
            // the path returned by synctex is formatted in unix style, for example it is something like this
			// in windows: d:/some/path/file.pdf
            // this doesn't work with Vimtex for some reason, so here we have to convert the path separators
			// to windows style and make sure the driver letter is capitalized
            QDir file_path = QDir(file_name);
            QString new_path = QDir::toNativeSeparators(file_path.absolutePath());
            new_path[0] = new_path[0].toUpper();
	    if (VIMTEX_WSL_FIX) {
				new_path = file_name;
	    }

#endif

			std::string line_string = std::to_string(line);
			std::string column_string = std::to_string(column);

			if (inverse_search_command.size() > 0) {
#ifdef Q_OS_WIN
				QString command = QString::fromStdWString(inverse_search_command).arg(new_path, line_string.c_str(), column_string.c_str());
#else
				QString command = QString::fromStdWString(inverse_search_command).arg(file_name, line_string.c_str(), column_string.c_str());
#endif
                std::wstring res = command.toStdWString();
				QProcess::startDetached(command);
			}
			else {
				show_error_message(L"inverse_search_command is not set in prefs_user.config");
			}

		}

	}
	synctex_scanner_free(scanner);

#endif
}

void MainWidget::set_status_message(std::wstring new_status_string) {
    custom_status_message = new_status_string;
}

void MainWidget::remove_self_from_windows() {
    for (size_t i = 0; i < windows.size(); i++) {
        if (windows[i] == this) {
            windows.erase(windows.begin() + i);
            break;
        }
    }
}


std::optional<DocumentPos> MainWidget::get_overview_position() {
    auto overview_state_ = opengl_widget->get_overview_page();
    if (overview_state_.has_value()){
        OverviewState overview_state = overview_state_.value();
        return main_document_view->get_document()->absolute_to_page_pos({ 0, overview_state.absolute_offset_y });
        //DocumentPos pos = { overview_state.page, 0.0f, overview_state.offset_y };
        //return pos;
    }
    return {};
}

void MainWidget::add_portal(std::wstring source_path, Portal new_link) {
	if (source_path == main_document_view->get_document()->get_path()) {
		main_document_view->get_document()->add_portal(new_link);
	}
	else {
		const std::unordered_map<std::wstring, Document*> cached_documents = document_manager->get_cached_documents();
		for (auto [doc_path, doc] : cached_documents) {
			if (source_path == doc_path) {
				doc->add_portal(new_link, false);
			}
		}

		db_manager->insert_portal(checksummer->get_checksum(source_path),
			new_link.dst.document_checksum,
			new_link.dst.book_state.offset_x,
			new_link.dst.book_state.offset_y,
			new_link.dst.book_state.zoom_level,
			new_link.src_offset_y);
	}
}

void MainWidget::handle_keyboard_select(const std::wstring& text) {
    if (text[0] == '#') {
        // we can select text using window-space coordinates.
        // this is not something that the user should be able to do, but it's useful for scripts.
		QStringList parts = QString::fromStdWString(text.substr(1, text.size()-1)).split(' ');
        if (parts.size() == 2) {
            QString begin_text = parts.at(0);
            QString end_text = parts.at(1);
            QStringList begin_parts = begin_text.split(',');
            QStringList end_parts = end_text.split(',');
            if ((begin_parts.size() == 3) && (end_parts.size() == 3)) {

                int begin_page_number = begin_parts.at(0).toInt();
                float begin_offset_x = begin_parts.at(1).toFloat();
                float begin_offset_y = begin_parts.at(2).toFloat();

                int end_page_number = end_parts.at(0).toInt();
                float end_offset_x = end_parts.at(1).toFloat();
                float end_offset_y = end_parts.at(2).toFloat();

                DocumentPos begin_doc_pos = { begin_page_number, begin_offset_x, begin_offset_y };
                DocumentPos end_doc_pos = { end_page_number, end_offset_x, end_offset_y };

                WindowPos begin_window_pos = main_document_view->document_to_window_pos_in_pixels(begin_doc_pos);
                WindowPos end_window_pos = main_document_view->document_to_window_pos_in_pixels(end_doc_pos);

				handle_left_click(begin_window_pos, true, false, false, false);
				handle_left_click(end_window_pos, false, false, false, false);
            }
        }

		opengl_widget->set_should_highlight_words(false);
	}
	else {
        // here we select with "user-friendly" tags

		QStringList parts = QString::fromStdWString(text).split(' ');

        if (parts.size() == 1) {
            std::vector<fz_irect> schar_rects;
            std::optional<fz_irect> srect_ = get_tag_window_rect(parts.at(0).toStdString(), &schar_rects);
            if (schar_rects.size() > 1){
                fz_irect srect = schar_rects[0];
                fz_irect erect = schar_rects[schar_rects.size() - 2];
                int w = erect.x1 - erect.x0;

				handle_left_click({ (srect.x0 + srect.x1) / 2 - 1, (srect.y0 + srect.y1) / 2 }, true, false, false, false);
				handle_left_click({ erect.x0 , (erect.y0 + erect.y1) / 2 }, false, false, false, false);
				opengl_widget->set_should_highlight_words(false);
            }
        }
        if (parts.size() == 2) {

            std::vector<fz_irect> schar_rects;
            std::vector<fz_irect> echar_rects;

            std::optional<fz_irect> srect_ = get_tag_window_rect(parts.at(0).toStdString(), &schar_rects);
            std::optional<fz_irect> erect_ = get_tag_window_rect(parts.at(1).toStdString(), &echar_rects);

            if ((schar_rects.size() > 0) && (echar_rects.size() > 0)) {
                fz_irect srect = schar_rects[0];
                fz_irect erect = echar_rects[0];
                int w = erect.x1 - erect.x0;

				handle_left_click({ (srect.x0 + srect.x1) / 2 - 1, (srect.y0 + srect.y1) / 2 }, true, false, false, false);
				handle_left_click({ erect.x0 - w/2 , (erect.y0 + erect.y1) / 2 }, false, false, false, false);
				opengl_widget->set_should_highlight_words(false);
            }
            else if (srect_.has_value() && erect_.has_value()) {
                fz_irect srect = srect_.value();
                fz_irect erect = erect_.value();

				handle_left_click({ srect.x0 + 5, (srect.y0 + srect.y1) / 2 }, true, false, false, false);
				handle_left_click({ erect.x0 - 5 , (erect.y0 + erect.y1) / 2 }, false, false, false, false);
				opengl_widget->set_should_highlight_words(false);
            }

		}
	}
}


void MainWidget::toggle_scrollbar() {

    // dirty hack!
    // really the content of this closure should be in toggle_scrollbar method, however,
	// for some unknown reason if we do that, new windows are created very small when 
	// toggle_scrollbar is in startup_commands. Strangely the culprit seems to be this line:
    // scroll_bar->show()
    // todo: figure out why this is the case and fix it
    QTimer::singleShot(100, [&]() {
			if (scroll_bar->isVisible()) {
				scroll_bar->hide();
			}
			else {
				scroll_bar->show();
			}
			main_window_width = opengl_widget->width();
        });
}

void MainWidget::update_scrollbar() {
    if (main_document_view_has_document()) {
        float offset = main_document_view->get_offset_y();
        int scroll = static_cast<int>(MAX_SCROLLBAR * offset / doc()->max_y_offset());
        scroll_bar->setValue(scroll);
    }
}

void MainWidget::handle_portal_overview_update() {
    std::optional<OverviewState> current_state_ = opengl_widget->get_overview_page();
    if (current_state_) {
        OverviewState current_state = current_state_.value();
        if (current_state.doc != nullptr) {
            std::optional<Portal> link_ = main_document_view->find_closest_portal();
            if (link_) {
                Portal link = link_.value();
                OpenedBookState link_new_state = link.dst.book_state;
                link_new_state.offset_y = current_state.absolute_offset_y;
                update_link_with_opened_book_state(link, link_new_state);
            }
        }
    }
}

void MainWidget::goto_overview() {
    if (opengl_widget->get_overview_page()) {
        OverviewState overview = opengl_widget->get_overview_page().value();
        if (overview.doc != nullptr) {
            std::optional<Portal> closest_link_ = main_document_view->find_closest_portal();
            if (closest_link_) {
                push_state();
                open_document(closest_link_.value().dst);
            }
        }
        else {
			std::optional<DocumentPos> maybe_overview_position = get_overview_position();
			if (maybe_overview_position.has_value()) {
				long_jump_to_destination(maybe_overview_position.value());
			}
        }
		opengl_widget->set_overview_page({});

    }
}

QString MainWidget::get_font_face_name() {
    if (UI_FONT_FACE_NAME.empty()) {
        return "Monaco";
    }
    else {
        return QString::fromStdWString(UI_FONT_FACE_NAME);
    }
}

void MainWidget::reset_highlight_links() {
    if (SHOULD_HIGHLIGHT_LINKS) {
        opengl_widget->set_highlight_links(true, false);
    }
    else {
        opengl_widget->set_highlight_links(false, false);
    }
}

void MainWidget::set_rect_select_mode(bool mode) {
    rect_select_mode = mode;
    if (mode == true) {
        opengl_widget->set_selected_rectangle({ 0, 0, 0, 0 });
    }
}

void MainWidget::clear_selected_rect() {
    opengl_widget->clear_selected_rectangle();
    //rect_select_mode = false;
    //rect_select_begin = {};
    //rect_select_end = {};
}

std::optional<fz_rect> MainWidget::get_selected_rect_absolute() {
    return opengl_widget->get_selected_rectangle();
}

bool MainWidget::get_selected_rect_document(int& out_page, fz_rect& out_rect) {
    std::optional<fz_rect> absrect = get_selected_rect_absolute();
    if (absrect) {

        AbsoluteDocumentPos top_left;
        AbsoluteDocumentPos bottom_right;

        top_left.x = absrect.value().x0;
        top_left.y = absrect.value().y0;
        bottom_right.x = absrect.value().x1;
        bottom_right.y = absrect.value().y1;

        DocumentPos top_left_document =  main_document_view->get_document()->absolute_to_page_pos(top_left);
        DocumentPos bottom_right_document =  main_document_view->get_document()->absolute_to_page_pos(bottom_right);

        fz_rect document_rect;
        document_rect.x0 = top_left_document.x;
        document_rect.y0 = top_left_document.y;
        document_rect.x1 = bottom_right_document.x;
        document_rect.y1 = bottom_right_document.y;

        out_rect = document_rect;
        out_page = top_left_document.page;

        return true;
    }
    else {
        return false;
    }
}


bool CharacterAddress::backspace() {
    if (previous_character) {
        CharacterAddress& prev = *previous_character;

        this->page = prev.page;
        this->block = prev.block;
        this->line = prev.line;
        this->doc = prev.doc;
        this->character = prev.character;
        delete previous_character;
        this->previous_character = nullptr;
        return false;
    }
    else {
		return false;
    }
}

bool CharacterAddress::advance(char c) {
    if (!previous_character) {
		if (character->c == c) {
			return next_char();
		}
		else {
            previous_character = new CharacterAddress();
            previous_character->page = page;
            previous_character->block = block;
            previous_character->line = line;
            previous_character->doc = doc;
            previous_character->character = character;
            next_char();
			return false;
		}
    }
    return false;
}

bool CharacterAddress::next_char() {
	if (character->next) {
		character = character->next;
        return false;
	}
	else {
		next_line();
        return true;
	}
}

bool CharacterAddress::next_line() {
	if (line->next) {
		line = line->next;
		character = line->first_char;
        return false;
	}
	else {
		next_block();
        return true;
	}
}

bool CharacterAddress::next_block() {
	if (block->next) {
		block = block->next;
		line = block->u.t.first_line;
		character = line->first_char;
        return false;
	}
	else {
		next_page();
        return true;
	}
}

bool CharacterAddress::next_page() {
    if (page < doc->num_pages() - 1) {
		page = page + 1;
		block = doc->get_stext_with_page_number(page)->first_block;
		line = block->u.t.first_line;
		character = line->first_char;
		return true;
    }
    return false;
}

float CharacterAddress::focus_offset() {

    fz_rect character_rect = fz_rect_from_quad(character->quad);
	return doc->document_to_absolute_y(page, character_rect.y0);
}

void MainWidget::clear_selected_text() {
	main_document_view->selected_character_rects.clear();
	selected_text.clear();
}

bool MainWidget::is_rect_visible(int page, fz_rect rect) {
	fz_irect window_rect = main_document_view->document_to_window_irect(page, rect);
	if (window_rect.x0 > 0 && window_rect.x1 < main_window_width && window_rect.y0 > 0 && window_rect.y1 < main_window_height) {
        return true;
	}
    else {
        return false;
    }
}

bool MainWidget::is_point_visible(int page, fz_point point) {
    //Document
    DocumentPos docpos;
    docpos.page = page;
    docpos.x = point.x;
    docpos.y = point.y;
    WindowPos window_pos = main_document_view->document_to_window_pos_in_pixels(docpos);
	//fz_irect window_rect = main_document_view->document_to_window_pos(page, rect);
	if ((window_pos.x > 0)
        && (window_pos.x < main_window_width)
        && (window_pos.y > 0)
        && (window_pos.y < main_window_height)) {
        return true;
	}
    else {
        return false;
    }
}

void MainWidget::set_mark_in_current_location(char symbol) {
	// it is a global mark, we delete other marks with the same symbol from database and add the new mark
	if (isupper(symbol)) {
		db_manager->delete_mark_with_symbol(symbol);
		// we should also delete the cached marks
		document_manager->delete_global_mark(symbol);
		main_document_view->add_mark(symbol);
	}
	else {
		main_document_view->add_mark(symbol);
		validate_render();
	}
}

void MainWidget::goto_mark(char symbol) {
	if (symbol == '`' || symbol == '\'') {
		return_to_last_visual_mark();
	}
	else if (isupper(symbol)) { // global mark
		std::vector<std::pair<std::string, float>> mark_vector;
		db_manager->select_global_mark(symbol, mark_vector);
		if (mark_vector.size() > 0) {
			assert(mark_vector.size() == 1); // we can not have more than one global mark with the same name
			std::wstring doc_path = checksummer->get_path(mark_vector[0].first).value();
			open_document(doc_path, 0.0f, mark_vector[0].second);
		}

	}
	else {
		main_document_view->goto_mark(symbol);
	}
}

void MainWidget::advance_command(std::unique_ptr<Command> new_command){
	if (new_command) {
		if (!new_command->next_requirement(this).has_value()) {
			new_command->run(this);
            pending_command_instance = nullptr;
		}
		else {
			pending_command_instance = std::move(new_command);

			Requirement next_requirement = pending_command_instance->next_requirement(this).value();
			if (next_requirement.type == RequirementType::Text) {
				show_textbar(utf8_decode(next_requirement.name), true, pending_command_instance->get_text_default_value());
			}
			else if (next_requirement.type == RequirementType::Symbol) {
                if (TOUCH_MODE) {
                    show_mark_selector();
                }
			}
			else if (next_requirement.type == RequirementType::File) {
				std::wstring file_name = select_command_file_name(pending_command_instance->get_name());
#ifdef SIOYEK_ANDROID

//                if (file_name.size() > 0 && QString::fromStdWString(file_name).startsWith("content://")) {
//                    qDebug() << "sioyek: trying to convert " << file_name;
//                    file_name = android_file_uri_from_content_uri(QString::fromStdWString(file_name)).toStdWString();
//                    qDebug() << "sioyek: result was " << file_name;
//                }
#endif
                if (file_name.size() > 0) {
					pending_command_instance->set_file_requirement(file_name);
					advance_command(std::move(pending_command_instance));
                }
			}
			else if (next_requirement.type == RequirementType::Rect) {
                set_rect_select_mode(true);
			}
            if (pending_command_instance) {
				pending_command_instance->pre_perform(this);
            }

		}
	}
}

void MainWidget::perform_search(std::wstring text, bool is_regex) {

   // When searching, the start position before search is saved in a mark named '0'
   main_document_view->add_mark('/');

   int range_begin, range_end;
   std::wstring search_term;
   std::optional<std::pair<int, int>> search_range = {};
   if (parse_search_command(text, &range_begin, &range_end, &search_term)) {
	   search_range = std::make_pair(range_begin, range_end);
   }

   if (search_term.size() > 0) {
	   // in mupdf RTL documents are reversed, so we reverse the search string
	   //todo: better (or any!) handling of mixed RTL and LTR text
	   if (is_rtl(search_term[0])) {
		   search_term = reverse_wstring(search_term);
	   }
   }

   if (is_regex && (!SUPER_FAST_SEARCH)) {
	   show_error_message(L"regex search only works when super_fast_search is enabled in prefs_user.config");
   }
   opengl_widget->search_text(search_term, CASE_SENSITIVE_SEARCH, is_regex, search_range);
}

void MainWidget::overview_to_definition() {
	if (!opengl_widget->get_overview_page()) {
		std::vector<DocumentPos> defpos = main_document_view->find_line_definitions();
		if (defpos.size() > 0) {
			set_overview_position(defpos[0].page, defpos[0].y);
			smart_view_candidates = defpos;
			index_into_candidates = 0;
		}
	}
	else {
		opengl_widget->set_overview_page({});
	}
}

void MainWidget::portal_to_definition() {
	std::vector<DocumentPos> defpos = main_document_view->find_line_definitions();
	if (defpos.size() > 0) {
		AbsoluteDocumentPos abspos = doc()->document_to_absolute_pos(defpos[0], true);
		Portal link;
		link.dst.document_checksum = doc()->get_checksum();
		link.dst.book_state.offset_x = abspos.x;
		link.dst.book_state.offset_y = abspos.y;
		link.dst.book_state.zoom_level = main_document_view->get_zoom_level();
		link.src_offset_y = main_document_view->get_ruler_pos();
		doc()->add_portal(link, true);
	}
}

void MainWidget::move_visual_mark_command(int amount) {
    if (opengl_widget->get_overview_page()) {
        if (amount > 0) {
			scroll_overview(amount);
        }
        else {
			scroll_overview(amount);
        }
    }
    else if (is_visual_mark_mode()) {
        move_visual_mark(amount);
    }
    else {
        move_document(0.0f, 72.0f * amount * VERTICAL_MOVE_AMOUNT);
    }
    if (AUTOCENTER_VISUAL_SCROLL) {
        return_to_last_visual_mark();
    }
    validate_render();
}

void MainWidget::handle_vertical_move(int amount) {
    if (opengl_widget->get_overview_page()) {
        scroll_overview(amount);
    }
    else if (opengl_widget->is_presentation_mode()) {
        main_document_view->move_pages(amount);
    }
    else {
        move_document(0.0f, 72.0f * amount * VERTICAL_MOVE_AMOUNT);
    }
}

void MainWidget::handle_horizontal_move(int amount) {
    if (opengl_widget->get_overview_page()) {
        return;
    }
    else if (opengl_widget->is_presentation_mode()) {
        main_document_view->move_pages(-amount);
    }
    else {
        main_document_view->move(72.0f * amount * HORIZONTAL_MOVE_AMOUNT, 0.0f);
        last_smart_fit_page = {};
    }
}

void MainWidget::show_current_widget() {
    if (current_widget_stack.size() > 0) {
        current_widget_stack.back()->show();
    }
}

void MainWidget::handle_goto_bookmark() {
	std::vector<std::wstring> option_names;
	std::vector<std::wstring> option_location_strings;
	std::vector<float> option_locations;
	std::vector<BookMark> bookmarks;
	if (SORT_BOOKMARKS_BY_LOCATION) {
		bookmarks = main_document_view->get_document()->get_sorted_bookmarks();
	}
	else {
		bookmarks = main_document_view->get_document()->get_bookmarks();
	}

	for (auto bookmark : bookmarks) {
		option_names.push_back(ITEM_LIST_PREFIX + L" " + bookmark.description);
		option_locations.push_back(bookmark.y_offset);
		auto [page, _, __] = main_document_view->get_document()->absolute_to_page_pos({ 0, bookmark.y_offset });
		option_location_strings.push_back(get_page_formatted_string(page + 1));
	}

	int closest_bookmark_index = main_document_view->get_document()->find_closest_bookmark_index(bookmarks, main_document_view->get_offset_y());

    if (!TOUCH_MODE) {
        set_current_widget(new FilteredSelectTableWindowClass<float>(
            option_names,
            option_location_strings,
            option_locations,
            closest_bookmark_index,
            [&](float* offset_value) {
                if (offset_value) {
                    validate_render();
                    push_state();
                    main_document_view->set_offset_y(*offset_value);
                }
            },
            this,
                [&](float* offset_value) {
                if (offset_value) {
                    main_document_view->delete_closest_bookmark_to_offset(*offset_value);
                }
            }));
    }
    else {
        QStandardItemModel* model = create_table_model(option_names, option_location_strings);

        TouchFilteredSelectWidget<float>* widget = new TouchFilteredSelectWidget<float>(model, option_locations, closest_bookmark_index,
            [&](float* offset_value) {
                if (offset_value) {
                    validate_render();
                    push_state();
                    main_document_view->set_offset_y(*offset_value);
                }
                pop_current_widget();
            },
            [&](float* offset_value) {
                if (offset_value) {
                    main_document_view->delete_closest_bookmark_to_offset(*offset_value);
                }
            }, this);
        set_current_widget(widget);
    }

    show_current_widget();
}

void MainWidget::handle_goto_bookmark_global() {
	std::vector<std::pair<std::string, BookMark>> global_bookmarks;
	db_manager->global_select_bookmark(global_bookmarks);
	std::vector<std::wstring> descs;
	std::vector<std::wstring> file_names;
	std::vector<BookState> book_states;

	for (const auto& desc_bm_pair : global_bookmarks) {
		std::string checksum = desc_bm_pair.first;
		std::optional<std::wstring> path = checksummer->get_path(checksum);
		if (path) {
			BookMark bm = desc_bm_pair.second;
			std::wstring file_name = Path(path.value()).filename().value_or(L"");
			descs.push_back(ITEM_LIST_PREFIX + L" " + bm.description);
			file_names.push_back(truncate_string(file_name, 50));
			book_states.push_back({ path.value(), bm.y_offset });
		}
	}
    if (!TOUCH_MODE) {
		set_current_widget(new FilteredSelectTableWindowClass<BookState>(
			descs,
			file_names,
			book_states,
			-1,
			[&](BookState* book_state) {
				if (book_state) {
					validate_render();
					open_document(book_state->document_path, 0.0f, book_state->offset_y);
				}
			},
			this,
				[&](BookState* book_state) {
				if (book_state) {
					db_manager->delete_bookmark(checksummer->get_checksum(book_state->document_path), book_state->offset_y);
				}
			}));
    }
    else {
        QStandardItemModel* model = create_table_model(descs, file_names);
		set_current_widget(new TouchFilteredSelectWidget<BookState>(
			model,
			book_states,
            -1,
			[&](BookState* book_state) {
				if (book_state) {
					validate_render();
					open_document(book_state->document_path, 0.0f, book_state->offset_y);
				}
				pop_current_widget();
			},
			[&](BookState* book_state) {
				if (book_state) {
					db_manager->delete_bookmark(checksummer->get_checksum(book_state->document_path), book_state->offset_y);
				}
			},
            this));
    }
    show_current_widget();
}

void MainWidget::handle_add_highlight(char symbol) {
	if (main_document_view->selected_character_rects.size() > 0) {
		main_document_view->add_highlight(selection_begin, selection_end, symbol);
		main_document_view->selected_character_rects.clear();
		selected_text.clear();
	}
    else {
        change_selected_highlight_type(symbol);
    }
}

void MainWidget::change_selected_highlight_type(char new_type) {
	if (selected_highlight_index != -1) {
		Highlight new_highlight = main_document_view->get_highlight_with_index(selected_highlight_index);
		main_document_view->delete_highlight_with_index(selected_highlight_index);
		main_document_view->add_highlight(new_highlight.selection_begin, new_highlight.selection_end, new_type);
		selected_highlight_index = -1;
	}
}

char MainWidget::get_current_selected_highlight_type() {
    if (selected_highlight_index != -1) {
        return main_document_view->get_highlight_with_index(selected_highlight_index).type;
    }
    return 'a';
}

void MainWidget::handle_goto_highlight() {
	std::vector<std::wstring> option_names;
	std::vector<std::wstring> option_location_strings;
	std::vector<Highlight> highlights = main_document_view->get_document()->get_highlights_sorted();

	int closest_highlight_index = main_document_view->get_document()->find_closest_highlight_index(highlights, main_document_view->get_offset_y());

	for (auto highlight : highlights) {
		std::wstring type_name = L"a";
		type_name[0] = highlight.type;
		option_names.push_back(L"[" + type_name + L"] " + highlight.description + L"]");
		auto [page, _, __] = main_document_view->get_document()->absolute_to_page_pos(highlight.selection_begin);
		option_location_strings.push_back(get_page_formatted_string(page + 1));
	}

    if (!TOUCH_MODE) {

        set_current_widget(new FilteredSelectTableWindowClass<Highlight>(
            option_names,
            option_location_strings,
            highlights,
            closest_highlight_index,
            [&](Highlight* hl) {
                if (hl) {
                    validate_render();
                    push_state();
                    main_document_view->set_offset_y(hl->selection_begin.y);
                }
            },
            this,
                [&](Highlight* hl) {
                if (hl) {
                    main_document_view->delete_highlight(*hl);
                }
            }));
    }
    else {
        QAbstractItemModel* model = create_table_model(option_names, option_location_strings);

        
        TouchFilteredSelectWidget<Highlight>* widget = new TouchFilteredSelectWidget<Highlight>(model, highlights, closest_highlight_index,
            [&](Highlight* hl) {
                if (hl) {
                    validate_render();
                    push_state();
                    main_document_view->set_offset_y(hl->selection_begin.y);
                }
                pop_current_widget();
            },
            [&](Highlight* hl) {
                if (hl) {
                    main_document_view->delete_highlight(*hl);
                }
            }, this);
        set_current_widget(widget);
        //new TouchListView(model, )
    }
    show_current_widget();
}

void MainWidget::handle_goto_highlight_global() {
	std::vector<std::pair<std::string, Highlight>> global_highlights;
	db_manager->global_select_highlight(global_highlights);
	std::vector<std::wstring> descs;
	std::vector<std::wstring> file_names;
	std::vector<BookState> book_states;

	for (const auto& desc_hl_pair : global_highlights) {
		std::string checksum = desc_hl_pair.first;
		std::optional<std::wstring> path = checksummer->get_path(checksum);
		if (path) {
			Highlight hl = desc_hl_pair.second;

			std::wstring file_name = Path(path.value()).filename().value_or(L"");

			std::wstring highlight_type_string = L"a";
			highlight_type_string[0] = hl.type;

			//descs.push_back(L"[" + highlight_type_string + L"]" + hl.description + L" {" + file_name + L"}");
			descs.push_back(L"[" + highlight_type_string + L"]" + hl.description);

			file_names.push_back(truncate_string(file_name, 50));

			book_states.push_back({ path.value(), hl.selection_begin.y });

		}
	}
    if (!TOUCH_MODE) {

        set_current_widget(new FilteredSelectTableWindowClass<BookState>(
            descs,
            file_names,
            book_states,
            -1,
            [&](BookState* book_state) {
                if (book_state) {
                    validate_render();
                    open_document(book_state->document_path, 0.0f, book_state->offset_y);
                }
            },
            this));
    }
    else {
        QAbstractItemModel* model = create_table_model(descs, file_names);
        set_current_widget(new TouchFilteredSelectWidget<BookState>(
            model,
            book_states,
            -1,
            [&](BookState* book_state) {
                if (book_state) {
                    validate_render();
                    open_document(book_state->document_path, 0.0f, book_state->offset_y);
                }
                pop_current_widget();
            },
            [&](BookState* book_state) {
            },
            this));
    }
    show_current_widget();
}

void MainWidget::handle_goto_toc() {

	if (main_document_view->get_document()->has_toc()) {
        if (TOUCH_MODE) {
			std::vector<std::wstring> flat_toc;
			std::vector<int> current_document_toc_pages;
			get_flat_toc(main_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
            std::vector<std::wstring> page_strings;
            for (int i = 0; i < current_document_toc_pages.size(); i++) {
                page_strings.push_back(("[ " + QString::number(current_document_toc_pages[i] + 1) + " ]").toStdWString());
            }
            int closest_toc_index = current_document_toc_pages.size()-1;
            int current_page = get_current_page_number();
            for (int i = 0; i < current_document_toc_pages.size(); i++) {
                if (current_document_toc_pages[i] > current_page) {
					closest_toc_index = i-1;
					break;
				}
            }
            if (closest_toc_index == -1) {
                closest_toc_index = 0;
			}
            QAbstractItemModel* model = create_table_model(flat_toc, page_strings);

  //  TouchFilteredSelectWidget(std::vector<std::wstring> std_string_list,
		//std::vector<T> values_,
		//std::function<void(T*)> on_done_,
		//std::function<void(T*)> on_delete,
            set_current_widget(new TouchFilteredSelectWidget<int>(model,current_document_toc_pages, closest_toc_index, [&](int* page_value) {
                    if (page_value) {
                        validate_render();

                        push_state();
                        main_document_view->goto_page(*page_value);
                        if (TOC_JUMP_ALIGN_TOP) {
                            main_document_view->scroll_mid_to_top();
                        }
                    }
                    pop_current_widget();
                }, [&](int* page) {}, this));
            show_current_widget();
        }
        else {

            if (FLAT_TABLE_OF_CONTENTS) {
                std::vector<std::wstring> flat_toc;
                std::vector<int> current_document_toc_pages;
                get_flat_toc(main_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
                set_current_widget(new FilteredSelectWindowClass<int>(flat_toc, current_document_toc_pages, [&](int* page_value) {
                    if (page_value) {
                        validate_render();

                        push_state();
                        main_document_view->goto_page(*page_value);
                        if (TOC_JUMP_ALIGN_TOP) {
                            main_document_view->scroll_mid_to_top();
                        }

                    }
                    }, this));
                show_current_widget();
            }
            else {

                std::vector<int> selected_index = main_document_view->get_current_chapter_recursive_index();
                //if (!TOUCH_MODE) {
                set_current_widget(new FilteredTreeSelect<int>(main_document_view->get_document()->get_toc_model(),
                    [&](const std::vector<int>& indices) {
                        TocNode* toc_node = get_toc_node_from_indices(main_document_view->get_document()->get_toc(),
                        indices);
                if (toc_node) {
                    validate_render();
                    //main_document_view->goto_page(toc_node->page);
                    push_state();
                    main_document_view->goto_offset_within_page({ toc_node->page, toc_node->x, toc_node->y });
                    if (TOC_JUMP_ALIGN_TOP) {
                        main_document_view->scroll_mid_to_top();
                    }
                }
                    }, this, selected_index));
                //        }
                //        else {

                //            QStandardItemModel* model = main_document_view->get_document()->get_toc_model();
                //            MySortFilterProxyModel* proxy_model = new MySortFilterProxyModel();
                //            proxy_model->setSourceModel(model);
                            //proxy_model->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
                //            proxy_model->setRecursiveFilteringEnabled(true);


                            //set_current_widget(new TouchFilteredSelectWidget<int>(proxy_model,
                            //	[&](int* index) {
                            //		//TocNode* toc_node = get_toc_node_from_indices(main_document_view->get_document()->get_toc(),
                            //		//	indices);
                            //		//if (toc_node) {
                            //		//	validate_render();
                            //		//	//main_document_view->goto_page(toc_node->page);
                            //		//	push_state();
                            //		//	main_document_view->goto_offset_within_page({ toc_node->page, toc_node->x, toc_node->y });
                            //		//	if (TOC_JUMP_ALIGN_TOP) {
                            //		//		main_document_view->scroll_mid_to_top();
                            //		//	}
                            //		//}
                            //	}, this));
                //        }
                show_current_widget();
            }
        }

	}
	else {
		show_error_message(L"This document doesn't have a table of contents");
	}
}

void MainWidget::handle_open_prev_doc() {

	std::vector<std::wstring> opened_docs_names;
	std::vector<std::wstring> opened_docs_hashes_;
	std::vector<std::string> opened_docs_hashes;

	db_manager->select_opened_books_path_values(opened_docs_hashes_);

	for (const auto& doc_hash_ : opened_docs_hashes_) {
		std::optional<std::wstring> path = checksummer->get_path(utf8_encode(doc_hash_));
		if (path) {

			if (SHOW_DOC_PATH) {
				opened_docs_names.push_back(path.value_or(L"<ERROR>"));
			}
			else {
#ifdef SIOYEK_ANDROID
            std::wstring path_value = path.value();
            if (path_value.substr(0, 10) == L"content://"){
                path_value = android_file_name_from_uri(QString::fromStdWString(path_value)).toStdWString();
            }
            opened_docs_names.push_back(Path(path.value()).filename_no_ext().value_or(L"<ERROR>"));
#else
                opened_docs_names.push_back(Path(path.value()).filename().value_or(L"<ERROR>"));
#endif
            }
			opened_docs_hashes.push_back(utf8_encode(doc_hash_));
		}
	}

//    auto prev_done_handler = [&](std::string* doc_hash) {
//            if (doc_hash->size() > 0) {
//                validate_render();
//                open_document_with_hash(*doc_hash);
//            }
//        };

    if (!TOUCH_MODE){
        set_current_widget(new FilteredSelectWindowClass<std::string>(opened_docs_names,
                                      opened_docs_hashes,
                                      [&](std::string* doc_hash) {
                       if (doc_hash->size() > 0) {
                           validate_render();
                           open_document_with_hash(*doc_hash);
                       }
                   },
                   this,
                   [&](std::string* doc_hash) {
            db_manager->delete_opened_book(*doc_hash);
        }));
    }
    else{
		set_current_widget(new TouchFilteredSelectWidget<std::string>(opened_docs_names,
			opened_docs_hashes,
            -1,
			[&](std::string* doc_hash) {
				if (doc_hash->size() > 0) {
					validate_render();
					open_document_with_hash(*doc_hash);
				}
				//                   prev_done_handler(doc_hash);
                pop_current_widget();

			}, [&](std::string* doc_hash) {
				db_manager->delete_opened_book(*doc_hash);
			},
				this));
    }
    show_current_widget();
}

void MainWidget::handle_move_screen(int amount) {
	if (!opengl_widget->is_presentation_mode()) {
		move_document_screens(amount);
	}
	else {
		main_document_view->move_pages(amount);
	}
}

MainWidget* MainWidget::handle_new_window() {
	MainWidget* new_widget = new MainWidget(mupdf_context,
		db_manager,
		document_manager,
		config_manager,
		command_manager,
		input_handler,
		checksummer,
		should_quit);
	new_widget->open_document(main_document_view->get_state());
	new_widget->show();
    new_widget->apply_window_params_for_one_window_mode();
	//new_widget->run_multiple_commands(STARTUP_COMMANDS);
    auto startup_commands = command_manager->create_macro_command("", STARTUP_COMMANDS);
    startup_commands->run(new_widget);

	windows.push_back(new_widget);
    return new_widget;
}

std::optional<std::pair<int, fz_link*>> MainWidget::get_selected_link(const std::wstring& text) {
	std::vector<std::pair<int, fz_link*>> visible_page_links;
	if (ALPHABETIC_LINK_TAGS || is_string_numeric(text)) {

        int link_index = 0;

        if (ALPHABETIC_LINK_TAGS) {
            link_index = get_index_from_tag(utf8_encode(text));
        }
        else {
			link_index = std::stoi(text);
        }

        main_document_view->get_visible_links(visible_page_links);
		if ((link_index >= 0) && (link_index < static_cast<int>(visible_page_links.size()))) {
            return visible_page_links[link_index];
		}
        return {};
	}
    return {};
}

void MainWidget::handle_overview_link(const std::wstring& text) {

    auto selected_link_ = get_selected_link(text);
    if (selected_link_) {
        PdfLink pdf_link;
        pdf_link.rect = selected_link_.value().second->rect;
        pdf_link.uri = selected_link_.value().second->uri;
        set_overview_link(pdf_link);
    }
	reset_highlight_links();
}

void MainWidget::handle_portal_to_link(const std::wstring& text) {

    auto selected_link_ = get_selected_link(text);
    if (selected_link_) {
        auto [page, link] = selected_link_.value();
        PdfLink pdf_link;
        pdf_link.rect = link->rect;
        pdf_link.uri = link->uri;
        ParsedUri parsed_uri = parse_uri(mupdf_context, pdf_link.uri);

		//AbsoluteDocumentPos abspos = doc()->document_to_absolute_pos(defpos[0], true);
        DocumentPos link_source_document_pos;
        link_source_document_pos.page = page;
        link_source_document_pos.x = 0;
        link_source_document_pos.y = link->rect.x0;
        DocumentPos dst_docpos;
        dst_docpos.page = parsed_uri.page - 1;
        dst_docpos.x = parsed_uri.x;
        dst_docpos.y = parsed_uri.y;

        auto src_abspos = doc()->document_to_absolute_pos(link_source_document_pos, true);
        auto dst_abspos = doc()->document_to_absolute_pos(dst_docpos, true);

		Portal portal;
		portal.dst.document_checksum = doc()->get_checksum();
		portal.dst.book_state.offset_x = dst_abspos.x;
		portal.dst.book_state.offset_y = dst_abspos.y;
		portal.dst.book_state.zoom_level = main_document_view->get_zoom_level();
        portal.src_offset_y = src_abspos.y;
		doc()->add_portal(portal, true);
    }
	reset_highlight_links();
}

void MainWidget::handle_open_link(const std::wstring& text, bool copy) {

    auto selected_link_ = get_selected_link(text);
    if (selected_link_) {
        auto [selected_page, selected_link] = selected_link_.value();
		if (copy) {
			copy_to_clipboard(utf8_decode(selected_link->uri));
		}
		else {
			if (QString(selected_link->uri).startsWith("http")) {
				open_web_url(utf8_decode(selected_link->uri));
			}
			else {
				auto [page, offset_x, offset_y] = parse_uri(mupdf_context, selected_link->uri);
				long_jump_to_destination(page - 1, offset_y);
			}
		}
    }
	reset_highlight_links();
}

void MainWidget::handle_keys_user_all() {
	std::vector<Path> keys_paths = input_handler->get_all_user_keys_paths();
	std::vector<std::wstring> keys_paths_wstring;
	for (auto path : keys_paths) {
		keys_paths_wstring.push_back(path.get_path());
	}

	set_current_widget(new FilteredSelectWindowClass<std::wstring>(
		keys_paths_wstring,
		keys_paths_wstring,
		[&](std::wstring* path) {
			if (path) {
				open_file(*path);
			}
		},
		this));
    show_current_widget();
}

void MainWidget::handle_prefs_user_all() {
	std::vector<Path> prefs_paths = config_manager->get_all_user_config_files();
	std::vector<std::wstring> prefs_paths_wstring;
	for (auto path : prefs_paths) {
		prefs_paths_wstring.push_back(path.get_path());
	}

	set_current_widget(new FilteredSelectWindowClass<std::wstring>(
		prefs_paths_wstring,
		prefs_paths_wstring,
		[&](std::wstring* path) {
			if (path) {
				open_file(*path);
			}
		},
		this));
    show_current_widget();
}

void MainWidget::handle_portal_to_overview() {
	std::optional<DocumentPos> maybe_overview_position = get_overview_position();
	if (maybe_overview_position.has_value()) {
		AbsoluteDocumentPos abs_pos = doc()->document_to_absolute_pos(maybe_overview_position.value());
		std::string document_checksum = main_document_view->get_document()->get_checksum();
		Portal new_portal;
		new_portal.dst.document_checksum = document_checksum;
		new_portal.dst.book_state.offset_x = abs_pos.x;
		new_portal.dst.book_state.offset_y = abs_pos.y;
		new_portal.dst.book_state.zoom_level = main_document_view->get_zoom_level();
		new_portal.src_offset_y = main_document_view->get_offset_y();
		//new_portal.dst.book_state.
		add_portal(main_document_view->get_document()->get_path(), new_portal);
	}
}

void MainWidget::handle_focus_text(const std::wstring& text) {
	if ((text.size() > 0) && (text[0] == '#')) {
		std::wstringstream ss(text.substr(1, text.size() - 1));
		std::wstring actual_text;
		int page_number;
		ss >> page_number;
		std::getline(ss, actual_text);
		focus_text(page_number, actual_text);
	}
	else {
		int page_number = main_document_view->get_center_page_number();
		focus_text(page_number, text);
	}
	opengl_widget->set_should_draw_vertical_line(true);
}

void MainWidget::handle_goto_window() {
	std::vector<std::wstring> window_names;
	std::vector<int> window_ids;
	for (int i = 0; i < windows.size(); i++) {
		window_names.push_back(windows[i]->windowTitle().toStdWString());
		window_ids.push_back(i);
	}
	set_current_widget(new FilteredSelectWindowClass<int>(window_names,
		window_ids,
		[&](int* window_id) {
			if (*window_id < windows.size()) {
				windows[*window_id]->raise();
				windows[*window_id]->activateWindow();
			}
		},
		this));
    show_current_widget();
}

void MainWidget::handle_toggle_smooth_scroll_mode() {
    smooth_scroll_mode = !smooth_scroll_mode;

    if (smooth_scroll_mode) {
        validation_interval_timer->setInterval(16);
    }
    else {
        validation_interval_timer->setInterval(INTERVAL_TIME);
    }
}


void MainWidget::handle_overview_to_portal() {
	if (opengl_widget->get_overview_page()) {
		opengl_widget->set_overview_page({});
	}
	else {

		OverviewState overview_state;
		std::optional<Portal> portal_ = main_document_view->find_closest_portal();
		if (portal_) {
			Portal portal = portal_.value();
			auto destination_path = checksummer->get_path(portal.dst.document_checksum);
			if (destination_path) {
				Document* doc = document_manager->get_document(destination_path.value());
				if (doc) {
					overview_state.absolute_offset_y = portal.dst.book_state.offset_y;
					overview_state.doc = doc;
					opengl_widget->set_overview_page(overview_state);
				}
			}
		}
	}
}

void MainWidget::handle_toggle_typing_mode() {
    if (!typing_location.has_value()) {
        int page = main_document_view->get_center_page_number();
        CharacterAddress charaddr;
        charaddr.doc = main_document_view->get_document();
        charaddr.page = page - 1;
        charaddr.next_page();

        opengl_widget->set_typing_rect(charaddr.page, fz_rect_from_quad(charaddr.character->quad), {});

        typing_location = std::move(charaddr);
        main_document_view->set_offset_y(typing_location.value().focus_offset());

    }
}

void MainWidget::handle_delete_highlight_under_cursor() {
	QPoint mouse_pos = mapFromGlobal(QCursor::pos());
	WindowPos window_pos = WindowPos{ mouse_pos.x(), mouse_pos.y() };
	int sel_highlight = main_document_view->get_highlight_index_in_pos(window_pos);
	if (sel_highlight != -1) {
		main_document_view->delete_highlight_with_index(sel_highlight);
	}
}

void MainWidget::handle_delete_selected_highlight(){
    if (selected_highlight_index != -1) {
        main_document_view->delete_highlight_with_index(selected_highlight_index);
        selected_highlight_index = -1;
    }
    validate_render();
}

void MainWidget::synchronize_pending_link() {
    for (auto window : windows) {
        if (window != this) {
			window->pending_link = pending_link;
        }
    }
    refresh_all_windows();
}

void MainWidget::refresh_all_windows(){
    for (auto window : windows) {
        window->invalidate_ui();
    }
}


int MainWidget::num_visible_links() {
	std::vector<std::pair<int, fz_link*>> visible_page_links;
    main_document_view->get_visible_links(visible_page_links);
    return visible_page_links.size();
}
bool MainWidget::event(QEvent *event){


    if (TOUCH_MODE){
        if (event->type() == QEvent::Gesture){
            auto gesture = (static_cast<QGestureEvent*>(event));

            if (gesture->gesture(Qt::TapAndHoldGesture)){
                velocity_x = 0;
                velocity_y = 0;

                if (was_last_mouse_down_in_ruler_next_rect){
                    return true;
                }

                if ((mapFromGlobal(QCursor::pos()) - last_press_point).manhattanLength() > 10){
                    return QWidget::event(event);
                }

                // only show menu when there are no other widgets
                if (current_widget_stack.size() > 0){
                    return true;
                }

                last_hold_point = mapFromGlobal(QCursor::pos());
                WindowPos window_pos = WindowPos{ last_hold_point.x(), last_hold_point.y() };
                //opengl_widget->last_selected_block

                QTapAndHoldGesture *tapgest = static_cast<QTapAndHoldGesture *>(gesture->gesture(Qt::TapAndHoldGesture));
                if (tapgest->state() == Qt::GestureFinished){

                    is_dragging = false;

                    if (is_in_back_rect(window_pos)) {
                        handle_command_types(command_manager->get_command_with_name("goto_mark"), 0);
                        invalidate_render();
                        return true;
                    }
                    if (is_in_forward_rect(window_pos)) {
                        handle_command_types(command_manager->get_command_with_name("set_mark"), 0);
                        invalidate_render();
                        return true;
                    }

                    //                persist();
                    //                toggle_fullscreen();
                    //                toggle_statusbar();
                    //                std::unique_ptr<Command> command = this->command_manager->get_command_with_name("command");
                    //                handle_command_types(std::move(command), 0);
                    //                invalidate_render();

                    set_current_widget(new AndroidSelector(this));
                    show_current_widget();

                    //                set_current_widget(new MyList(this));
                    //                current_widget->show();
                    return true;
                }
            }
            if (gesture->gesture(Qt::PinchGesture)){
                pdf_renderer->no_rerender = true;
                QPinchGesture *pinch = static_cast<QPinchGesture *>(gesture->gesture(Qt::PinchGesture));
                float scale = pinch->scaleFactor();
                main_document_view->set_zoom_level(main_document_view->get_zoom_level() * scale, true);
                return true;
            }

            return QWidget::event(event);

        }
    }
    return QWidget::event(event);


}

void MainWidget::handle_mobile_selection(){
//    QPoint selection_position = last_hold_point;
    fz_stext_char* character_under = get_closest_character_to_cusrsor(last_hold_point);
    if (character_under){
        int current_page = get_current_page_number();
        fz_rect uncentered_rect =doc()->document_to_absolute_rect(current_page, fz_rect_from_quad(character_under->quad), false);
        fz_rect centered_rect =doc()->document_to_absolute_rect(current_page, fz_rect_from_quad(character_under->quad), true);
        main_document_view->selected_character_rects.push_back(centered_rect);


        fz_rect rect = centered_rect;
        int page;
        fz_rect document_rect = doc()->absolute_to_page_rect(rect, &page);
//        fz_rect window_rect = main_document_view->absolute_to_window_rect(rect);
        DocumentPos begin_document_pos, end_document_pos;

        begin_document_pos.x = document_rect.x0;
        begin_document_pos.y = document_rect.y0;
        begin_document_pos.page = page;
        end_document_pos.x = document_rect.x1;
        end_document_pos.y = document_rect.y1;
        end_document_pos.page = page;

        AbsoluteDocumentPos begin_abspos;
        begin_abspos.x = rect.x0;
        begin_abspos.y = rect.y0;

        AbsoluteDocumentPos end_abspos;
        end_abspos.x = rect.x1;
        end_abspos.y = rect.y1;

        WindowPos begin_window_pos =  main_document_view->document_to_window_pos_in_pixels(begin_document_pos);
        WindowPos end_window_pos =  main_document_view->document_to_window_pos_in_pixels(end_document_pos);

        QPoint begin_pos;
        begin_pos.setX(begin_window_pos.x - 40);
        begin_pos.setY(begin_window_pos.y - 40);

        QPoint end_pos;
        end_pos.setX(end_window_pos.x);
        end_pos.setY(end_window_pos.y);

        selection_begin_indicator = new SelectionIndicator(this, true, this, begin_abspos);
        selection_end_indicator = new SelectionIndicator(this, false, this, end_abspos);
//        text_selection_buttons = new TextSelectionButtons(this);


//        int window_width = width();
//        int window_height = height();
        float pixel_ratio = QGuiApplication::primaryScreen()->devicePixelRatio();

//        begin_pos = begin_pos / pixel_ratio;
//        end_pos = end_pos / pixel_ratio;

//        float real_height = pixel_ratio * window_height;
//        float real_width = pixel_ratio * window_width;

        selection_begin_indicator->move(begin_pos);
        selection_end_indicator->move(end_pos);

//        selection_begin_indicator->move(QPoint(0, 0));
//        selection_end_indicator->move(QPoint(20, 20));

        selection_begin_indicator->resize(40, 40);
        selection_end_indicator->resize(40, 40);

        selection_begin_indicator->raise();
        selection_end_indicator->raise();

        selection_begin_indicator->show();
        selection_end_indicator->show();
        text_selection_buttons->show();

        invalidate_render();
    }
//    selected_character
//    smart_jump_under_pos(window_pos);

//    main_document_view->window_to_absolute_document_pos();
}

void MainWidget::update_mobile_selection(){
//				handle_left_click(begin_window_pos, true, false, false, false);
//				handle_left_click(end_window_pos, false, false, false, false);
    DocumentPos begin = selection_begin_indicator->docpos;
    DocumentPos end = selection_end_indicator->docpos;
    AbsoluteDocumentPos begin_absolute = doc()->document_to_absolute_pos(begin, true);
    AbsoluteDocumentPos end_absolute = doc()->document_to_absolute_pos(end, true);

    main_document_view->get_text_selection(begin_absolute,
    end_absolute,
    false,
    main_document_view->selected_character_rects,
    selected_text);
    validate_render();
}

void MainWidget::clear_selection_indicators(){
    if (selection_begin_indicator){
        selection_begin_indicator->hide();
        selection_end_indicator->hide();
        text_selection_buttons->hide();
        delete selection_begin_indicator;
        delete selection_end_indicator;
        //delete text_selection_buttons;
        selection_begin_indicator = nullptr;
        selection_end_indicator = nullptr;
        //text_selection_buttons = nullptr;
    }
}

bool MainWidget::handle_quick_tap(WindowPos click_pos){
    // returns true if we double clicked

    QTime now = QTime::currentTime();

    if ((last_quick_tap_time.msecsTo(now) < 200) && (mapFromGlobal(QCursor::pos()) - last_quick_tap_position).manhattanLength() < 20){
        if (handle_double_tap(last_quick_tap_position)) {
			return true;
        }
    }

    //NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(click_pos);


    if (is_in_back_rect(click_pos)) {
		prev_state();
		return true;
    }
    if (is_in_forward_rect(click_pos)) {
		next_state();
		return true;
    }

    if (is_in_edit_portal_rect(click_pos)) {
        handle_command_types(command_manager->get_command_with_name("edit_portal"), 0);
		return true;
    }

    clear_selected_text();
    clear_selection_indicators();
    selected_highlight_index = -1;
    clear_highlight_buttons();
    clear_search_buttons();
    opengl_widget->cancel_search();
    is_dragging = false;

    pop_current_widget();
    //if (current_widget != nullptr) {
    //    delete current_widget;
    //    current_widget = nullptr;
    //}
    text_command_line_edit_container->hide();

    last_quick_tap_position = mapFromGlobal(QCursor::pos());
    last_quick_tap_time = now;
    return false;
}

//void MainWidget::applicationStateChanged(Qt::ApplicationState state){
//    qDebug() << "application state changed\n";
//    if ((state == Qt::ApplicationState::ApplicationSuspended) || (state == Qt::ApplicationState::ApplicationInactive)){
//        persist();
//    }
//}

void MainWidget::android_handle_visual_mode(){
//	last_hold_point
    if (is_visual_mark_mode()){
        opengl_widget->set_should_draw_vertical_line(false);
    }
    else{

        WindowPos pos;
        pos.x = last_hold_point.x();
        pos.y = last_hold_point.y();

        visual_mark_under_pos(pos);
    }
}

bool MainWidget::is_moving(){
    return (velocity_x != 0) || (velocity_y != 0);
}

void MainWidget::update_position_buffer(){
    QPoint pos = QCursor::pos();
    QTime time = QTime::currentTime();

    position_buffer.push_back(std::make_pair(time, pos));

    if (position_buffer.size() > 5){
        position_buffer.pop_front();
    }
}

bool MainWidget::is_flicking(QPointF* out_velocity){
    // we never flick the background when a widget is being shown on top
    if (current_widget_stack.size() > 0) {
        return false;
    }

    std::vector<float> speeds;
    std::vector<float> dts;
    std::vector<QPointF> velocities;
    if (position_buffer.size() == 0){
        *out_velocity = QPointF(0, 0);
        return false;
    }
    for (int i = 0; i < position_buffer.size()-1; i++){
        float dt = (static_cast<float>(position_buffer[i].first.msecsTo(position_buffer[i+1].first)) / 1000.0f);
//        dts.push_back(dt);
        if (dt < (1.0f / 120.0f)){
            dt = 1.0f / 120.0f;
        }

        if (dt != 0){
            QPointF velocity = (position_buffer[i+1].second - position_buffer[i].second).toPointF() /  dt;
            velocities.push_back(velocity);
            speeds.push_back(sqrt(QPointF::dotProduct(velocity, velocity)));
        }
    }
    if (speeds.size() == 0){
        *out_velocity = QPointF(0, 0);
        return false;
    }
    float average_speed = compute_average<float>(speeds);

    if (out_velocity){
        *out_velocity = 2 * compute_average<QPointF>(velocities);
    }

    if (average_speed > 500.0f){
        return true;
    }
    else{
        return false;
    }
}

bool MainWidget::handle_double_tap(QPoint pos){
    WindowPos position;
    position.x = pos.x();
    position.y = pos.y();

    if (is_visual_mark_mode()) {
        NormalizedWindowPos nmp = main_document_view->window_to_normalized_window_pos(position);
        // don't want to accidentally smart jump when quickly moving the ruler
        if (screen()->orientation() == Qt::PortraitOrientation) {
            if (PORTRAIT_VISUAL_MARK_NEXT.contains(nmp) || PORTRAIT_VISUAL_MARK_PREV.contains(nmp)) {
                return false;
            }
        }
        else {
            if (LANDSCAPE_VISUAL_MARK_NEXT.contains(nmp) || LANDSCAPE_VISUAL_MARK_PREV.contains(nmp)) {
                return false;
            }
        }
    }
    smart_jump_under_pos(position);
    return true;
}


void MainWidget::show_highlight_buttons(){
    //highlight_buttons = new HighlightButtons(this);
    highlight_buttons->highlight_buttons->setHighlightType(get_current_selected_highlight_type());
    update_highlight_buttons_position();
    highlight_buttons->show();
}

void MainWidget::clear_highlight_buttons(){
    if (highlight_buttons){
        highlight_buttons->hide();
        //delete highlight_buttons;
        //highlight_buttons = nullptr;
    }
}

void MainWidget::handle_touch_highlight(){

    DocumentPos begin_docpos = selection_begin_indicator->docpos;
    DocumentPos end_docpos = selection_end_indicator->docpos;

    AbsoluteDocumentPos begin_abspos = doc()->document_to_absolute_pos(begin_docpos, true);
    AbsoluteDocumentPos end_abspos = doc()->document_to_absolute_pos(end_docpos, true);

    main_document_view->add_highlight(begin_abspos, end_abspos, select_highlight_type);

    invalidate_render();
}

void MainWidget::show_search_buttons(){
    search_buttons->show();
}

void MainWidget::clear_search_buttons(){

	search_buttons->hide();
}

void MainWidget::restore_default_config(){
    config_manager->restore_default();
    config_manager->restore_defaults_in_memory();
	//config_manager->deserialize(default_config_path, auto_config_path, user_config_paths);
    invalidate_render();
}


void MainWidget::persist_config(){
    config_manager->persist_config();
}

int MainWidget::get_current_colorscheme_index(){
    auto colormode = opengl_widget->get_current_color_mode();
    if (colormode == PdfViewOpenGLWidget::ColorPalette::Normal){
        return 0;
    }
    if (colormode == PdfViewOpenGLWidget::ColorPalette::Dark){
        return 1;
    }
    if (colormode == PdfViewOpenGLWidget::ColorPalette::Custom){
        return 2;
    }
    return -1;
}

void MainWidget::set_dark_mode(){
    opengl_widget->set_dark_mode(true);
}

void MainWidget::set_light_mode(){
    opengl_widget->set_dark_mode(false);
}

void MainWidget::set_custom_color_mode(){
    opengl_widget->set_custom_color_mode(true);
}

void MainWidget::update_highlight_buttons_position() {
    if (selected_highlight_index != -1) {
		Highlight hl = main_document_view->get_highlight_with_index(selected_highlight_index);
		AbsoluteDocumentPos hlpos;
		hlpos.x = hl.selection_begin.x;
		hlpos.y = hl.selection_begin.y;
		DocumentPos docpos = doc()->absolute_to_page_pos(hlpos);

		WindowPos windowpos = main_document_view->document_to_window_pos_in_pixels(docpos);
		highlight_buttons->move(highlight_buttons->pos().x(), windowpos.y -  highlight_buttons->height());
    }
}

void MainWidget::handle_debug_command() {
    int page = get_current_page_number();
    std::vector<fz_rect> rects;
    doc()->get_page_bib_candidates(page, &rects);
    std::vector<MarkedDataRect> mdrs;
    for (auto r : rects) {
        MarkedDataRect mdr;
        mdr.page = page;
        mdr.rect = r;
        mdr.type = 0;
        mdrs.push_back(mdr);
    }
    opengl_widget->marked_data_rects = mdrs;
    invalidate_render();
}

void MainWidget::download_paper_under_cursor() {
	QPoint mouse_pos = mapFromGlobal(QCursor::pos());
    WindowPos pos(mouse_pos.x(), mouse_pos.y());
    DocumentPos docpos = main_document_view->window_to_document_pos(pos);
    auto [page, offset_x, offset_y] = docpos;
    //std::optional<std::wstring> reference_text_on_pointer = main_document_view->get_document()->get_reference_text_at_position(flat_chars, offset_x, offset_y);
    std::optional<PdfLink> pdf_link_ = doc()->get_link_in_pos(docpos);

    std::optional<std::wstring> bib_text_ = {};

    if (pdf_link_) {
        PdfLink pdf_link = pdf_link_.value();
        fz_rect pdf_rect = pdf_link.rect;
        auto [link_page,offset_x,offset_y] = parse_uri(mupdf_context, pdf_link.uri);
        std::wstring link_text = doc()->get_text_in_rect(page, pdf_rect);
        link_text = clean_link_source_text(link_text);
		bib_text_ = doc()->get_page_bib_with_reference(link_page-1, link_text);
    }
    else {
		auto ref_ = doc()->get_reference_text_at_position(page, offset_x, offset_y);
		int target_page = -1;
		float target_offset;
        if (find_location_of_text_under_pointer(pos, &target_page, &target_offset) == ReferenceType::Reference) {
			if (ref_) {
				std::wstring ref = ref_.value();
				bib_text_ = doc()->get_page_bib_with_reference(target_page, ref);
			}
        }
        else {
            std::optional<std::wstring> paper_name = get_paper_name_under_cursor();
            if (paper_name) {
                bib_text_ = paper_name;
            }
        }
    }

    if (bib_text_) {
		std::wstring bib_text = clean_bib_item(bib_text_.value());
		QUrl get_url = QString::fromStdWString(PAPER_SEARCH_URL).replace(
			"%{query}",
			QUrl::toPercentEncoding(QString::fromStdWString(bib_text))
		);

		QNetworkRequest req;
		req.setUrl(get_url);
		network_manager.get(req);
    }
}

void MainWidget::read_current_line() {
	std::wstring text = main_document_view->get_selected_line_text().value_or(L"");
    tts.setRate(TTS_RATE);
    is_reading = false;
    tts.stop();
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));
    tts.say(QString::fromStdWString(text));
    is_reading = true;
}

void MainWidget::handle_start_reading() {
    is_reading = true;
    read_current_line();
    if (TOUCH_MODE) {
        set_current_widget(new AudioUI(this));
        show_current_widget();
    }
}

void MainWidget::handle_stop_reading() {
    is_reading = false;
    tts.stop();
    if (TOUCH_MODE) {
        pop_current_widget();
    }
}

void MainWidget::handle_play() {
    is_reading = true;
    if (tts.state() == QTextToSpeech::Paused) {
		tts.resume();
    }
    else {
        move_visual_mark(1);
        invalidate_render();
    }
}

void MainWidget::handle_pause() {
    is_reading = false;
    tts.pause(QTextToSpeech::BoundaryHint::Immediate);
}
bool MainWidget::should_show_status_label() {
	float prog;
    if (TOUCH_MODE) {
        if (current_widget_stack.size() > 0 || opengl_widget->get_is_searching(&prog) || is_pending_link_source_filled()) {
            return true;
        }
        return false;
    }
    else {
        return should_show_status_label_ || opengl_widget->get_is_searching(&prog);
    }
}

bool MainWidget::is_in_back_rect(WindowPos pos) {
    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
		return PORTRAIT_BACK_UI_RECT.contains(nwp);
	}
    else {
		return LANDSCAPE_BACK_UI_RECT.contains(nwp);
	}
}

bool MainWidget::is_in_forward_rect(WindowPos pos) {
    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
		return PORTRAIT_FORWARD_UI_RECT.contains(nwp);
	}
    else {
		return LANDSCAPE_FORWARD_UI_RECT.contains(nwp);
	}
}

bool MainWidget::is_in_edit_portal_rect(WindowPos pos) {
    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
		return PORTRAIT_EDIT_PORTAL_UI_RECT.contains(nwp);
	}
    else {
		return LANDSCAPE_EDIT_PORTAL_UI_RECT.contains(nwp);
	}
}

void MainWidget::goto_page_with_label(std::wstring label) {
    int page = doc()->get_page_number_with_label(label);
    if (page > -1) {
		main_document_view->goto_page(page);
    }
}

void MainWidget::on_configs_changed(std::vector<std::string>* config_names) {
    bool should_reflow = false;
    for (int i = 0; i < config_names->size(); i++) {
        if (QString::fromStdString((*config_names)[i]).startsWith("epub")) {
            should_reflow = true;
        }
    }
    if (should_reflow) {
        bool flag = false;
		pdf_renderer->delete_old_pages(true, true);
        int new_page = doc()->reflow(get_current_page_number());
        main_document_view->goto_page(new_page);
    }
}

void MainWidget::on_config_changed(std::string config_name) {
    std::vector<std::string> config_names;
    config_names.push_back(config_name);
    on_configs_changed(&config_names);
}

void MainWidget::handle_undo_marked_data() {
    if (opengl_widget->marked_data_rects.size() > 0) {
        opengl_widget->marked_data_rects.pop_back();
    }
}

void MainWidget::handle_add_marked_data() {
    std::vector<fz_rect> local_selected_rects;
    std::wstring local_selected_text;

	main_document_view->get_text_selection(selection_begin,
		selection_end,
		is_word_selecting,
		local_selected_rects,
		local_selected_text);
    if (local_selected_rects.size() > 0) {
        int page = -1;
        fz_rect begin_docrect = doc()->absolute_to_page_rect(local_selected_rects[0], &page);
        fz_rect end_docrect = doc()->absolute_to_page_rect(local_selected_rects[local_selected_rects.size()-1], &page);

        MarkedDataRect begin_rect;
        begin_rect.rect = begin_docrect;
        begin_rect.page = page;
        begin_rect.type = 0;
        opengl_widget->marked_data_rects.push_back(begin_rect);

        MarkedDataRect end_rect;
        end_rect.rect = end_docrect;
        end_rect.page = page;
        end_rect.type = 1;
        opengl_widget->marked_data_rects.push_back(end_rect);

        invalidate_render();
    }
}

void MainWidget::handle_remove_marked_data() {
    opengl_widget->marked_data_rects.clear();
}


void MainWidget::handle_export_marked_data() {
    fz_stext_page* stext_page = doc()->get_stext_with_page_number(get_current_page_number());
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    auto export_path = qgetenv("SIOYEK_DATA_EXPORT").toStdString();
    QDir export_dir = QDir(QString::fromStdString(export_path));
    std::string checksum = checksummer->get_checksum_fast(doc()->get_path()).value();
    QString file_name = QString("%1_%2.json").arg(QString::fromStdString(checksum), QString::number(get_current_page_number()));
    auto file_path = export_dir.filePath(file_name);

    QJsonObject json;

    QJsonArray rects;
    for (auto [type, rect_objects] : opengl_widget->get_marked_data_rect_map()) {
        for (auto rect_object : rect_objects) {
			QJsonArray rect;
            rect.append(rect_object.type);
			rect.append(rect_object.rect.x0);
			rect.append(rect_object.rect.x1);
			rect.append(rect_object.rect.y0);
			rect.append(rect_object.rect.y1);
			rects.append(rect);
        }

    }
    //for (int i = 0; i < opengl_widget->marked_data_rects.size(); i++) {
    //    QJsonArray rect;
    //    rect.append(opengl_widget->marked_data_rects[i].first.x0);
    //    rect.append(opengl_widget->marked_data_rects[i].first.x1);
    //    rect.append(opengl_widget->marked_data_rects[i].first.y0);
    //    rect.append(opengl_widget->marked_data_rects[i].first.y1);
    //    rects.append(rect);
    //}

    json["selected_char_rects"] = rects;
    json["schema"] = 0;

    QJsonArray page_chars;

    for (auto chr : flat_chars) {
        fz_rect chr_rect = fz_rect_from_quad(chr->quad);
        QJsonArray chr_json;
        chr_json.append(chr->c);
        chr_json.append(chr_rect.x0);
        chr_json.append(chr_rect.x1);
        chr_json.append(chr_rect.y0);
        chr_json.append(chr_rect.y1);
        page_chars.append(chr_json);
    }
    json["page_characters"] = page_chars;

    QJsonDocument json_doc;
    json_doc.setObject(json);

    QFile json_file(file_path);
    json_file.open(QFile::WriteOnly);
    json_file.write(json_doc.toJson());
    json_file.close();

    opengl_widget->marked_data_rects.clear();
    invalidate_render();


}

void MainWidget::handle_goto_random_page() {
    int num_pages = doc()->num_pages();
    int random_page = rand() % num_pages;
    main_document_view->goto_page(random_page);
    invalidate_render();
}
void MainWidget::show_download_paper_menu(
    const std::vector<std::wstring>& paper_names,
    const std::vector<std::wstring>& contributor_names,
    const std::vector<std::wstring>& download_urls) {

	set_current_widget(new FilteredSelectTableWindowClass<std::wstring>(
		paper_names,
		contributor_names,
		download_urls,
		-1,
		[&](std::wstring* url) {
			qDebug() << *url;
            download_paper_with_url(*url);
		},
		this,
			[&](std::wstring* _) {
		}));
    show_current_widget();


}

void MainWidget::download_paper_with_url(std::wstring paper_url_) {
    QString paper_url = QString::fromStdWString(paper_url_);
    paper_url = paper_url.right(paper_url.size() - paper_url.lastIndexOf("http"));
	QNetworkRequest req;
	req.setUrl(paper_url);
	network_manager.get(req);
}

bool MainWidget::is_network_manager_running(bool* is_downloading){
    auto children = network_manager.findChildren<QNetworkReply*>();
    for (int i = 0; i < children.size(); i++) {
        if (children.at(i)->isRunning()) {
            if (is_downloading) {
                qDebug() << children.at(i)->url().toString();
                *is_downloading = children.at(i)->url().toString().endsWith(".pdf");
            }
            return true;
        }
    }
    return false;
}

void MainWidget::toggle_freehand_drawing_mode() {
    freehand_drawing_mode = !freehand_drawing_mode;
}

void MainWidget::handle_undo_drawing() {
    doc()->undo_freehand_drawing();
    invalidate_render();
}

