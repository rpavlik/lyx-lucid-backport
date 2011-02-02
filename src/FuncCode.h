// -*- C++ -*-
/**
 * \file FuncCode.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * Full author contact details are available in file CREDITS.
 *
 * To add a new function:
 * - add a new enum constant immediately before LFUN_LASTACTION
 * - add an appropriate line and documentation in LyXAction.cpp
 * - add a branch to a suitable ::doDispatch() method
 * - add correct test to the corresponding ::getStatus() method
 */

#ifndef FUNCCODE_H
#define FUNCCODE_H

namespace lyx {

/** These are all the lyx functions, the main mechanism
 * through which the frontends communicate with the core.
 *
 * They are managed in LyXAction.cpp and handled in various
 * ::dispatch() functions, starting with Application::dispatch(),
 * BufferView::dispatch(), Cursor::dispatch() and
 * Inset*::doDispatch();
 */
enum FuncCode
{
	LFUN_UNKNOWN_ACTION = -1,
	// 0
	LFUN_NOACTION = 0,
	LFUN_COMMAND_PREFIX,
	LFUN_SCREEN_RECENTER,
	LFUN_BUFFER_NEW,
	LFUN_BUFFER_WRITE,
	// 5
	LFUN_BUFFER_WRITE_AS,
	LFUN_BUILD_PROGRAM,
	LFUN_BUFFER_VIEW,
	LFUN_BUFFER_CLOSE,
	LFUN_LYX_QUIT,
	// 10
	LFUN_BUFFER_AUTO_SAVE,
	LFUN_UNDO,
	LFUN_REDO,
	LFUN_PASTE,
	LFUN_PRIMARY_SELECTION_PASTE,
	// 15
	LFUN_SELECTION_PASTE, // JMarc 2008/12/15
	LFUN_CUT,
	LFUN_COPY,
	LFUN_CLOSE_TAB_GROUP,
	LFUN_NOTE_NEXT,
	// 20
	LFUN_INSET_TOGGLE,
	LFUN_HFILL_INSERT,
	LFUN_TEXTSTYLE_APPLY,
	LFUN_TEXTSTYLE_UPDATE,
	LFUN_FONT_EMPH,
	// 25
	LFUN_FONT_BOLD,
	LFUN_FONT_BOLDSYMBOL,
	LFUN_FONT_ROMAN,
	LFUN_FONT_NOUN,
	LFUN_FONT_ITAL,
	// 30
	LFUN_FONT_FRAK,
	LFUN_CHAR_FORWARD,
	LFUN_CHAR_BACKWARD,
	LFUN_UP,
	LFUN_DOWN,
	// 35
	LFUN_SCREEN_UP,
	LFUN_SCREEN_DOWN,
	LFUN_LINE_BEGIN,
	LFUN_LINE_END,
	LFUN_CELL_FORWARD,
	// 40
	LFUN_CELL_BACKWARD,
	LFUN_WORD_FORWARD,
	LFUN_WORD_BACKWARD,
	LFUN_BUFFER_BEGIN,
	LFUN_BUFFER_END,
	// 45
	LFUN_CHAR_FORWARD_SELECT,
	LFUN_CHAR_BACKWARD_SELECT,
	LFUN_UP_SELECT,
	LFUN_DOWN_SELECT,
	LFUN_SCREEN_UP_SELECT,
	// 50
	LFUN_SCREEN_DOWN_SELECT,
	LFUN_LINE_BEGIN_SELECT,
	LFUN_LINE_END_SELECT,
	LFUN_WORD_FORWARD_SELECT,
	LFUN_WORD_BACKWARD_SELECT,
	// 55
	LFUN_WORD_SELECT,
	LFUN_BUFFER_BEGIN_SELECT,
	LFUN_BUFFER_END_SELECT,
	LFUN_SPACE_INSERT,              // JSpitzm 20030520
	LFUN_SPECIALCHAR_INSERT,        // JSpitzm, 20071206
	// 60
	LFUN_MARK_TOGGLE,
	LFUN_CHAR_DELETE_FORWARD,
	LFUN_CHAR_DELETE_BACKWARD,
	LFUN_NEWLINE_INSERT,		// renamed: JSpitzm, 20080325
	LFUN_BREAK_PARAGRAPH,
	// 65
	LFUN_QUOTE_INSERT,
	LFUN_ACCENT_CIRCUMFLEX,
	LFUN_MATH_SUBSCRIPT,
	LFUN_MATH_SUPERSCRIPT,
	LFUN_ACCENT_GRAVE,
	// 70
	LFUN_ACCENT_ACUTE,
	LFUN_ACCENT_TILDE,
	LFUN_ACCENT_CEDILLA,
	LFUN_ACCENT_MACRON,
	LFUN_ACCENT_UNDERBAR,
	// 75
	LFUN_ACCENT_UNDERDOT,
	LFUN_ACCENT_CIRCLE,
	LFUN_ACCENT_TIE,
	LFUN_ACCENT_BREVE,
	LFUN_ACCENT_CARON,
	// 80
	LFUN_ACCENT_HUNGARIAN_UMLAUT,
	LFUN_ACCENT_UMLAUT,
	LFUN_ACCENT_DOT,
	LFUN_ACCENT_OGONEK,
	LFUN_SELF_INSERT,
	// 85
	LFUN_GETBUFNAME,
	LFUN_SERVER_GET_XY,
	LFUN_SERVER_SET_XY,
	LFUN_SPLIT_VIEW,
	LFUN_LINEATCURSOR,
	// 90
	LFUN_SERVER_GET_LAYOUT,
	LFUN_SERVER_GET_FILENAME,
	LFUN_SERVER_NOTIFY,
	LFUN_SERVER_GOTO_FILE_ROW,
	LFUN_NOTE_INSERT,
	// 95
	LFUN_UI_TOGGLE,
	LFUN_KEYMAP_OFF,
	LFUN_KEYMAP_PRIMARY,
	LFUN_KEYMAP_SECONDARY,
	LFUN_KEYMAP_TOGGLE,
	// 100
	LFUN_MATH_INSERT,
	LFUN_MATH_MATRIX,
	LFUN_MATH_LIMITS,
	LFUN_MATH_DELIM,                // Alejandro 180696
	LFUN_MATH_DISPLAY,              // Alejandro 180696
	// 105
	LFUN_MATH_MODE,                 // Alejandro 040696
	LFUN_MATH_NUMBER_TOGGLE,
	LFUN_MATH_NUMBER_LINE_TOGGLE,
	LFUN_MATH_SIZE,                 // Alejandro 150896
	LFUN_MATH_MACRO,                // ale970510
	// 110
	LFUN_MATH_EXTERN,               // André 20010424
	LFUN_MATH_MUTATE,               // André 20010523
	LFUN_MATH_SPACE,                // André 20010725
	LFUN_WORD_DELETE_FORWARD,
	LFUN_WORD_DELETE_BACKWARD,
	// 115
	LFUN_LINE_DELETE,
	LFUN_DEBUG_LEVEL_SET,
	LFUN_MARK_OFF,
	LFUN_MARK_ON,
	LFUN_LAYOUT,
	// 120
	LFUN_LAYOUT_PARAGRAPH,
	LFUN_DROP_LAYOUTS_CHOICE,       // used in bindings as of 20071228
	LFUN_FONT_TYPEWRITER,           // changed from FONT_CODE 20070920
	LFUN_FONT_SANS,
	LFUN_FONT_DEFAULT,
	// 125
	LFUN_FONT_SIZE,
	LFUN_FONT_STATE,
	LFUN_MATH_FONT_STYLE,
	LFUN_LABEL_INSERT,
	LFUN_WORD_UPCASE,
	// 130
	LFUN_WORD_LOWCASE,
	LFUN_WORD_CAPITALIZE,
	LFUN_DEPTH_DECREMENT,
	LFUN_DEPTH_INCREMENT,
	LFUN_MENU_OPEN,                 // used in bindings as of 20060905
	// 135
	LFUN_CANCEL,
	LFUN_META_PREFIX,
	LFUN_COMMAND_EXECUTE,
	LFUN_FILE_INSERT,
	LFUN_FILE_INSERT_PLAINTEXT,     // CFO-G 1997-11-19
	// 140
	LFUN_FILE_INSERT_PLAINTEXT_PARA,// Levon 2001-02-14
	LFUN_FILE_OPEN,
	LFUN_PARAGRAPH_UP,              // Asger 1996-10-01
	LFUN_PARAGRAPH_UP_SELECT,       // Asger 1996-10-01
	LFUN_PARAGRAPH_DOWN,            // Asger 1996-10-01
	// 145
	LFUN_PARAGRAPH_DOWN_SELECT,     // Asger 1996-10-01
	LFUN_INSET_FORALL,		// lasgouttes, 20091127
	LFUN_BUFFER_NEW_TEMPLATE,       // Asger 1997-02-02
	LFUN_BUFFER_RELOAD,             // Asger 1997-02-02
	LFUN_RECONFIGURE,
	// 150
	LFUN_BRANCH_ACTIVATE,
	LFUN_BRANCH_DEACTIVATE,
	LFUN_INDEX_PRINT,               // Lgb 97-02-27
	LFUN_CELL_SPLIT,
	LFUN_BUFFER_CHILD_OPEN,         // Ale 970528
	// 155
	LFUN_INSET_COPY_AS,             // vfr, 20100419
	LFUN_FLOAT_LIST_INSERT,         // Lgb 20010503
	LFUN_BUFFER_TOGGLE_READ_ONLY,   // Lgb 97-05-27
	LFUN_VC_REGISTER,               // Lgb 97-07-01
	LFUN_VC_CHECK_IN,               // Lgb 97-07-01
	// 160
	LFUN_VC_CHECK_OUT,              // Lgb 97-07-01
	LFUN_VC_REVERT,                 // Lgb 97-07-01
	LFUN_VC_UNDO_LAST,              // Lgb 97-07-01
	LFUN_VC_COMMAND,
	LFUN_VC_LOCKING_TOGGLE,
	// 165
	LFUN_VC_REPO_UPDATE,
	LFUN_VC_COMPARE,
	LFUN_HYPERLINK_INSERT,          // CFO-G 971121
	LFUN_WORD_FIND_FORWARD,         // Etienne 980216
	LFUN_WORD_FIND_BACKWARD,        // Etienne 980220
	// 170
	LFUN_APPENDIX,                  // ettrich 980505
	LFUN_BUFFER_IMPORT,             // Asger 980724
	LFUN_COMMAND_SEQUENCE,          // Andre' 991111
	LFUN_PREFERENCES_SAVE,
	LFUN_HELP_OPEN,                 // Jug 990627
	// 175
	LFUN_DATE_INSERT,               // jdblair 20000131
	LFUN_LANGUAGE,                  // Dekel 20000203
	LFUN_ERT_INSERT,                // Jug 20000218
	LFUN_FOOTNOTE_INSERT,           // Jug 20000307
	LFUN_MATH_AMS_MATRIX,           // uwestoehr 12-07-2009
	// 180
	LFUN_TABULAR_INSERT,            // Jug 20000412
	LFUN_LOFVIEW,                   // Dekel 20000519
	LFUN_LOTVIEW,                   // Dekel 20000519
	LFUN_LOAVIEW,                   // Dekel 20000519
	LFUN_SET_COLOR,                 // SLior 20000611
	// 185
	LFUN_MARGINALNOTE_INSERT,       // Lgb 20000626
	LFUN_FLOAT_INSERT,              // Lgb 20000627
	LFUN_FLOAT_WIDE_INSERT,         // Lgb 20010531
	LFUN_CAPTION_INSERT,            // Lgb 20000718; inactive as of 20060905
	LFUN_BUFFER_SWITCH,
	// 190
	LFUN_BUFFER_CHKTEX,             // Asger 971030
	LFUN_LAYOUT_TABULAR,            // Jug 20000731
	LFUN_BUFFER_UPDATE,             // Dekel 20000805
	LFUN_INDEX_INSERT,              // Angus 20000803
	LFUN_SCREEN_FONT_UPDATE,        // ARRae 20000813
	// 195
	LFUN_PARAGRAPH_GOTO,            // Dekel 20000826
	LFUN_REFERENCE_NEXT,            // Dekel 20010114
	LFUN_BOOKMARK_SAVE,
	LFUN_BOOKMARK_GOTO,
	LFUN_SELECT_FILE_SYNC,          // Levon 20010214
	// 200
	LFUN_MESSAGE,                   // Lgb 20010408; for scripting purposes, output in minibuffer
	LFUN_CHARS_TRANSPOSE,           // Lgb 20010425
	LFUN_ESCAPE,                    // Lgb 20010517
	LFUN_THESAURUS_ENTRY,           // Levon 20010720
	LFUN_ARGUMENT_INSERT,           // Martin 12 Aug 2002
	// 205
	LFUN_MOUSE_PRESS,               // André 9 Aug 2002
	LFUN_MOUSE_MOTION,              // André 9 Aug 2002
	LFUN_MOUSE_RELEASE,             // André 9 Aug 2002
	LFUN_MOUSE_DOUBLE,              // André 9 Aug 2002
	LFUN_MOUSE_TRIPLE,              // André 9 Aug 2002
	// 210
	LFUN_WRAP_INSERT,               // Dekel 7 Apr 2002
	LFUN_CHANGES_TRACK,             // Levon 20021001 (cool date !)
	LFUN_CHANGES_MERGE,             // Levon 20021016
	LFUN_CHANGE_ACCEPT,             // Levon 20021016
	LFUN_CHANGE_REJECT,             // Levon 20021016
	// 215
	LFUN_ALL_CHANGES_ACCEPT,        // Levon 20021016
	LFUN_ALL_CHANGES_REJECT,        // Levon 20021016
	LFUN_SCROLL,
	LFUN_DIALOG_SHOW,
	LFUN_DIALOG_SHOW_NEW_INSET,
	// 220
	LFUN_DIALOG_UPDATE,
	LFUN_DIALOG_HIDE,
	LFUN_DIALOG_TOGGLE,             // JSpitzm 20070430
	LFUN_DIALOG_DISCONNECT_INSET,
	LFUN_INSET_APPLY,
	// 225
	LFUN_INSET_INSERT,
	LFUN_INSET_MODIFY,
	LFUN_INSET_DIALOG_UPDATE,
	LFUN_INSET_SETTINGS,
	LFUN_PARAGRAPH_PARAMS_APPLY,
	// 230
	LFUN_PARAGRAPH_UPDATE,
	LFUN_INSET_EDIT,                // JSpitzm 20080427
	LFUN_BRANCH_INSERT,
	LFUN_BOX_INSERT,
	LFUN_BUFFER_TOGGLE_OUTPUT_SYNC,
	// 235
	LFUN_NEWPAGE_INSERT,            // uwestoehr, 20071124
	LFUN_REPEAT,
	LFUN_FINISHED_LEFT,
	LFUN_FINISHED_RIGHT,
	LFUN_FLEX_INSERT,
	// 240
	LFUN_WORD_FIND,
	LFUN_WORD_REPLACE,
	LFUN_BUFFER_EXPORT_CUSTOM,
	LFUN_BUFFER_PRINT,
	LFUN_BUFFER_CLOSE_ALL,          // vfr 20090806
	// 245
	LFUN_BUFFER_LANGUAGE,
	LFUN_TEXTCLASS_APPLY,
	LFUN_TEXTCLASS_LOAD,
	LFUN_BUFFER_SAVE_AS_DEFAULT,
	LFUN_BUFFER_PARAMS_APPLY,
	// 250
	LFUN_LYXRC_APPLY,
	LFUN_BUFFER_NEXT,
	LFUN_BUFFER_PREVIOUS,
	LFUN_STATISTICS,
	LFUN_CHANGES_OUTPUT,            // jspitzm 20050121
	// 255
	LFUN_BIBTEX_DATABASE_ADD,
	LFUN_BIBTEX_DATABASE_DEL,
	LFUN_CITATION_INSERT,
	LFUN_OUTLINE_UP,
	LFUN_OUTLINE_DOWN,
	// 260
	LFUN_OUTLINE_IN,
	LFUN_OUTLINE_OUT,
	LFUN_PARAGRAPH_MOVE_DOWN,
	LFUN_PARAGRAPH_MOVE_UP,
	LFUN_CURSOR_FOLLOWS_SCROLLBAR_TOGGLE, // ARRae 971202
	// 265
	LFUN_CLIPBOARD_PASTE,
	LFUN_INSET_DISSOLVE,            // jspitzm 20060807
	LFUN_CHANGE_NEXT,
	LFUN_CHANGE_PREVIOUS,           // vfr 20090404
	LFUN_WINDOW_NEW,                // Abdel 20061021
	// 270
	LFUN_WINDOW_CLOSE,              // Abdel 20061023
	LFUN_UNICODE_INSERT,            // Lgb 20061022
	LFUN_BOOKMARK_CLEAR,            // bpeng 20061031
	LFUN_NOMENCL_INSERT,            // Ugras
	LFUN_NOMENCL_PRINT,             // Ugras
	// 275
	LFUN_LISTING_INSERT,            // Herbert 20011110, bpeng 20070502
	LFUN_TOOLBAR_TOGGLE,            // Edwin 20070521
	LFUN_BUFFER_WRITE_ALL,          // rgh, gpothier 200707XX
	LFUN_PARAGRAPH_PARAMS,
	LFUN_LAYOUT_MODULES_CLEAR,
	// 280
	LFUN_LAYOUT_MODULE_ADD,
	LFUN_LAYOUT_RELOAD,
	LFUN_MASTER_BUFFER_VIEW,        // Tommaso, 20070920
	LFUN_MASTER_BUFFER_UPDATE,      // Tommaso, 20070920
	LFUN_INFO_INSERT,               // bpeng, 20071007
	// 285
	LFUN_CALL,                      // broider, 20071002
	LFUN_CHAR_LEFT,	                // dov, 20071022
	LFUN_CHAR_LEFT_SELECT,          // dov, 20071022
	LFUN_CHAR_RIGHT,                // dov, 20071022
	LFUN_CHAR_RIGHT_SELECT,         // dov, 20071022
	// 290
	LFUN_FINISHED_BACKWARD,         // dov, 20071022
	LFUN_FINISHED_FORWARD,          // dov, 20071022
	LFUN_WORD_LEFT,	                // dov, 20071028
	LFUN_WORD_LEFT_SELECT,          // dov, 20071028
	LFUN_WORD_RIGHT,                // dov, 20071028
	// 295
	LFUN_WORD_RIGHT_SELECT,         // dov, 20071028
	LFUN_MATH_MACRO_FOLD,
	LFUN_MATH_MACRO_UNFOLD,
	LFUN_MATH_MACRO_ADD_PARAM,
	LFUN_MATH_MACRO_REMOVE_PARAM,
	// 300
	LFUN_MATH_MACRO_APPEND_GREEDY_PARAM,
	LFUN_MATH_MACRO_REMOVE_GREEDY_PARAM,
	LFUN_MATH_MACRO_MAKE_OPTIONAL,
	LFUN_MATH_MACRO_MAKE_NONOPTIONAL,
	LFUN_MATH_MACRO_ADD_OPTIONAL_PARAM,
	// 305
	LFUN_MATH_MACRO_REMOVE_OPTIONAL_PARAM,
	LFUN_MATH_MACRO_ADD_GREEDY_OPTIONAL_PARAM,
	LFUN_IN_MATHMACROTEMPLATE,
	LFUN_COMPLETION_POPUP,
	LFUN_COMPLETION_INLINE,
	// 310
	LFUN_COMPLETION_COMPLETE,
	LFUN_SET_GRAPHICS_GROUP,
	LFUN_COMPLETION_CANCEL,
	LFUN_COMPLETION_ACCEPT,
 	LFUN_COMMAND_ALTERNATIVES,
	// 315
	LFUN_TAB_INSERT,
	LFUN_TAB_DELETE,
	LFUN_WORD_FINDADV,              // Tommaso, 20081003
	LFUN_REGEXP_MODE,               // Tommaso, 20081003
	LFUN_LABEL_GOTO,                // Ale 970806
	// 320
	LFUN_LABEL_COPY_AS_REF,         // sts, 20081116
	LFUN_LABEL_INSERT_AS_REF,       // vfr, 20090407
	LFUN_PHANTOM_INSERT,            // uwestoehr, 20090130
	LFUN_INSET_BEGIN,               // JMarc, 20090316
	LFUN_INSET_END,                 // JMarc, 20090316
	// 325
	LFUN_INSET_BEGIN_SELECT,        // JMarc, 20090316
	LFUN_INSET_END_SELECT,          // JMarc, 20090316
	LFUN_INSET_SELECT_ALL,			// vfr, 20090822
	LFUN_BUFFER_ZOOM_IN,            // vfr, 20090330
	LFUN_BUFFER_ZOOM_OUT,           // vfr, 20090330
	// 330
	LFUN_MATH_BIGDELIM,
	LFUN_SECTION_SELECT,            // vfr, 20090503
	LFUN_FONT_UNDERLINE,
	LFUN_FONT_STRIKEOUT,
	LFUN_FONT_UULINE,
	// 335
	LFUN_FONT_UWAVE,
	LFUN_BUFFER_EXPORT,             // Lgb 97-07-29
	LFUN_BUFFER_TOGGLE_COMPRESSION, // bpeng 20060427
	LFUN_BRANCH_ADD,                // spitz 20090707
	LFUN_BRANCH_ADD_INSERT,
	// 340
	LFUN_BRANCHES_RENAME,           // spitz 20090709
	LFUN_GRAPHICS_RELOAD,           // vfr 20090810
	LFUN_SCREEN_SHOW_CURSOR,        // vfr, 20090325
	LFUN_SPELLING_ADD,              // spitz 20100118
	LFUN_SPELLING_IGNORE,           // spitz 20100118
	// 345
	LFUN_SPELLING_REMOVE,           // switt 20100728
	LFUN_PREVIEW_INSERT,            // vfr, 20100328
	LFUN_FORWARD_SEARCH,
	LFUN_SCRIPT_INSERT,             // gb, 20101123
	// 350

	LFUN_LASTACTION                 // end of the table
};


} // namespace lyx

#endif // FUNCCODE_H
