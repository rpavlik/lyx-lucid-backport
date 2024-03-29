
/**
 * \file GuiCitation.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Angus Leeming
 * \author Kalle Dalheimer
 * \author Abdelrazak Younes
 * \author Richard Heck
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "GuiCitation.h"

#include "GuiSelectionManager.h"
#include "qt_helpers.h"

#include "Buffer.h"
#include "BufferView.h"
#include "BiblioInfo.h"
#include "BufferParams.h"
#include "FuncRequest.h"

#include "insets/InsetCommand.h"

#include "support/debug.h"
#include "support/docstring.h"
#include "support/gettext.h"
#include "support/lstrings.h"

#include <QCloseEvent>
#include <QSettings>
#include <QShowEvent>
#include <QVariant>

#include <vector>
#include <string>

#undef KeyPress

#include "support/regex.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace std;
using namespace lyx::support;

namespace lyx {
namespace frontend {

static vector<CiteStyle> citeStyles_;


template<typename String>
static QStringList to_qstring_list(vector<String> const & v)
{
	QStringList qlist;

	for (size_t i = 0; i != v.size(); ++i) {
		if (v[i].empty())
			continue;
		qlist.append(lyx::toqstr(v[i]));
	}
	return qlist;
}


static vector<lyx::docstring> to_docstring_vector(QStringList const & qlist)
{
	vector<lyx::docstring> v;
	for (int i = 0; i != qlist.size(); ++i) {
		if (qlist[i].isEmpty())
			continue;
		v.push_back(lyx::qstring_to_ucs4(qlist[i]));
	}
	return v;
}


GuiCitation::GuiCitation(GuiView & lv)
	: DialogView(lv, "citation", qt_("Citation")),
	  params_(insetCode("citation"))
{
	setupUi(this);

	connect(citationStyleCO, SIGNAL(activated(int)),
		this, SLOT(on_citationStyleCO_currentIndexChanged(int)));
	connect(fulllistCB, SIGNAL(clicked()),
		this, SLOT(changed()));
	connect(forceuppercaseCB, SIGNAL(clicked()),
		this, SLOT(changed()));
	connect(textBeforeED, SIGNAL(textChanged(QString)),
		this, SLOT(changed()));
	connect(textAfterED, SIGNAL(textChanged(QString)),
		this, SLOT(changed()));
	connect(findLE, SIGNAL(returnPressed()), 
		this, SLOT(on_searchPB_clicked()));
	connect(textBeforeED, SIGNAL(returnPressed()),
		this, SLOT(on_okPB_clicked()));
	connect(textAfterED, SIGNAL(returnPressed()),
		this, SLOT(on_okPB_clicked()));

	selectionManager = new GuiSelectionManager(availableLV, selectedLV, 
			addPB, deletePB, upPB, downPB, &available_model_, &selected_model_);
	connect(selectionManager, SIGNAL(selectionChanged()),
		this, SLOT(setCitedKeys()));
	connect(selectionManager, SIGNAL(updateHook()),
		this, SLOT(updateControls()));
	connect(selectionManager, SIGNAL(okHook()),
		this, SLOT(on_okPB_clicked()));

	setFocusProxy(availableLV);

	// FIXME: the sizeHint() for this is _way_ too high
	infoML->setFixedHeight(60);
}


GuiCitation::~GuiCitation()
{
	delete selectionManager;
}


void GuiCitation::closeEvent(QCloseEvent * e)
{
	clearSelection();
	DialogView::closeEvent(e);
}


void GuiCitation::applyView()
{
	int const choice = max(0, citationStyleCO->currentIndex());
	style_ = choice;
	bool const full  = fulllistCB->isChecked();
	bool const force = forceuppercaseCB->isChecked();

	QString const before = textBeforeED->text();
	QString const after = textAfterED->text();

	apply(choice, full, force, before, after);
}


void GuiCitation::showEvent(QShowEvent * e)
{
	findLE->clear();
	availableLV->setFocus();
	DialogView::showEvent(e);
}


void GuiCitation::on_okPB_clicked()
{
	applyView();
	clearSelection();
	hide();
}


void GuiCitation::on_cancelPB_clicked()
{
	clearSelection();
	hide();
}


void GuiCitation::on_applyPB_clicked()
{
	applyView();
}


void GuiCitation::on_restorePB_clicked()
{
	init();
}


void GuiCitation::updateControls()
{
	BiblioInfo const & bi = bibInfo();
	updateControls(bi);
}


// The main point of separating this out is that the fill*() methods
// called in update() do not need to be called for INTERNAL updates,
// such as when addPB is pressed, as the list of fields, entries, etc,
// will not have changed. At the moment, however, the division between
// fillStyles() and updateStyle() doesn't lend itself to dividing the
// two methods, though they should be divisible. That is, we should
// not have to call fillStyles() every time through here.
void GuiCitation::updateControls(BiblioInfo const & bi)
{
	QModelIndex idx = selectionManager->getSelectedIndex();
	updateInfo(bi, idx);
	setButtons();

	textBeforeED->setText(toqstr(params_["before"]));
	textAfterED->setText(toqstr(params_["after"]));
	fillStyles(bi);
	updateStyle();
}


void GuiCitation::updateFormatting(CiteStyle currentStyle)
{
	CiteEngine const engine = citeEngine();
	bool const natbib_engine =
		engine == ENGINE_NATBIB_AUTHORYEAR ||
		engine == ENGINE_NATBIB_NUMERICAL;
	bool const basic_engine = engine == ENGINE_BASIC;

	bool const haveSelection = 
		selectedLV->model()->rowCount() > 0;

	bool const isNocite = currentStyle == NOCITE;

	bool const isCiteyear =
		currentStyle == CITEYEAR ||
		currentStyle == CITEYEARPAR;

	fulllistCB->setEnabled(natbib_engine && haveSelection && !isNocite
		&& !isCiteyear);
	forceuppercaseCB->setEnabled(natbib_engine && haveSelection
		&& !isNocite && !isCiteyear);
	textBeforeED->setEnabled(!basic_engine && haveSelection && !isNocite);
	textBeforeLA->setEnabled(!basic_engine && haveSelection && !isNocite);
	textAfterED->setEnabled(haveSelection && !isNocite);
	textAfterLA->setEnabled(haveSelection && !isNocite);
	citationStyleCO->setEnabled(haveSelection);
	citationStyleLA->setEnabled(haveSelection);
}


void GuiCitation::updateStyle()
{
	string const & command = params_.getCmdName();

	// Find the style of the citekeys
	vector<CiteStyle> const & styles = citeStyles_;
	CitationStyle const cs = citationStyleFromString(command);

	vector<CiteStyle>::const_iterator cit =
		std::find(styles.begin(), styles.end(), cs.style);

	if (cit != styles.end()) {
		fulllistCB->setChecked(cs.full);
		forceuppercaseCB->setChecked(cs.forceUpperCase);
	} else {
		// restore the last used natbib style
		if (style_ >= 0 && style_ < citationStyleCO->count()) {
			// the necessary update will be performed later
			citationStyleCO->blockSignals(true);
			citationStyleCO->setCurrentIndex(style_);
			citationStyleCO->blockSignals(false);
		}
		fulllistCB->setChecked(false);
		forceuppercaseCB->setChecked(false);
	}
	updateFormatting(cs.style);
}


// This one needs to be called whenever citationStyleCO needs
// to be updated---and this would be on anything that changes the
// selection in selectedLV, or on a general update.
void GuiCitation::fillStyles(BiblioInfo const & bi)
{
	QStringList selected_keys = selected_model_.stringList();
	int curr = selectedLV->model()->rowCount() - 1;

	if (curr < 0 || selected_keys.empty()) {
		citationStyleCO->clear();
		citationStyleCO->setEnabled(false);
		citationStyleLA->setEnabled(false);
		return;
	}

	if (!selectedLV->selectionModel()->selectedIndexes().empty())
		curr = selectedLV->selectionModel()->selectedIndexes()[0].row();

	QStringList sty = citationStyles(bi, curr);

	if (sty.isEmpty()) { 
		// some error
		citationStyleCO->setEnabled(false);
		citationStyleLA->setEnabled(false);
		citationStyleCO->clear();
		return;
	}
	
	citationStyleCO->blockSignals(true);
	
	// save old index
	int const oldIndex = citationStyleCO->currentIndex();
	citationStyleCO->clear();
	citationStyleCO->insertItems(0, sty);
	citationStyleCO->setEnabled(true);
	citationStyleLA->setEnabled(true);
	// restore old index
	if (oldIndex != -1 && oldIndex < citationStyleCO->count())
		citationStyleCO->setCurrentIndex(oldIndex);

	citationStyleCO->blockSignals(false);
}


void GuiCitation::fillFields(BiblioInfo const & bi)
{
	fieldsCO->blockSignals(true);
	int const oldIndex = fieldsCO->currentIndex();
	fieldsCO->clear();
	QStringList const fields = to_qstring_list(bi.getFields());
	fieldsCO->insertItem(0, qt_("All fields"));
	fieldsCO->insertItem(1, qt_("Keys"));
	fieldsCO->insertItems(2, fields);
	if (oldIndex != -1 && oldIndex < fieldsCO->count())
		fieldsCO->setCurrentIndex(oldIndex);
	fieldsCO->blockSignals(false);
}


void GuiCitation::fillEntries(BiblioInfo const & bi)
{
	entriesCO->blockSignals(true);
	int const oldIndex = entriesCO->currentIndex();
	entriesCO->clear();
	QStringList const entries = to_qstring_list(bi.getEntries());
	entriesCO->insertItem(0, qt_("All entry types"));
	entriesCO->insertItems(1, entries);
	if (oldIndex != -1 && oldIndex < entriesCO->count())
		entriesCO->setCurrentIndex(oldIndex);
	entriesCO->blockSignals(false);
}


bool GuiCitation::isSelected(QModelIndex const & idx)
{
	QString const str = idx.data().toString();
	return selected_model_.stringList().contains(str);
}


void GuiCitation::setButtons()
{
	selectionManager->update();
	int const srows = selectedLV->model()->rowCount();
	applyPB->setEnabled(srows > 0);
	okPB->setEnabled(srows > 0);
}


void GuiCitation::updateInfo(BiblioInfo const & bi, QModelIndex const & idx)
{
	if (!idx.isValid() || bi.empty()) {
		infoML->document()->clear();
		return;
	}

	QString const keytxt = toqstr(
		bi.getInfo(qstring_to_ucs4(idx.data().toString()), buffer(), true));
	infoML->document()->setHtml(keytxt);
}


void GuiCitation::findText(QString const & text, bool reset)
{
	//"All Fields" and "Keys" are the first two
	int index = fieldsCO->currentIndex() - 2; 
	BiblioInfo const & bi = bibInfo();
	vector<docstring> const & fields = bi.getFields();
	docstring field;
	
	if (index <= -1 || index >= int(fields.size()))
		//either "All Fields" or "Keys" or an invalid value
		field = from_ascii("");
	else
		field = fields[index];
	
	//Was it "Keys"?
	bool const onlyKeys = index == -1;
	
	//"All Entry Types" is first.
	index = entriesCO->currentIndex() - 1; 
	vector<docstring> const & entries = bi.getEntries();
	docstring entry_type;
	if (index < 0 || index >= int(entries.size()))
		entry_type = from_ascii("");
	else 
		entry_type = entries[index];
	
	bool const case_sentitive = caseCB->checkState();
	bool const reg_exp = regexCB->checkState();
	findKey(bi, text, onlyKeys, field, entry_type, 
	               case_sentitive, reg_exp, reset);
	//FIXME
	//It'd be nice to save and restore the current selection in 
	//availableLV. Currently, we get an automatic reset, since the
	//model is reset.
	
	updateControls(bi);
}


void GuiCitation::on_fieldsCO_currentIndexChanged(int /*index*/)
{
	findText(findLE->text(), true);
}


void GuiCitation::on_entriesCO_currentIndexChanged(int /*index*/)
{
	findText(findLE->text(), true);
}


void GuiCitation::on_citationStyleCO_currentIndexChanged(int index)
{
	if (index >= 0 && index < citationStyleCO->count()) {
		vector<CiteStyle> const & styles = citeStyles_;
		updateFormatting(styles[index]);
	}
}


void GuiCitation::on_findLE_textChanged(const QString & text)
{
	bool const searchAsWeGo = (asTypeCB->checkState() == Qt::Checked);
	searchPB->setDisabled(text.isEmpty() || searchAsWeGo);
	if (!text.isEmpty()) {
		if (searchAsWeGo)
			findText(findLE->text());
		return;
	}
	findText(findLE->text());
	findLE->setFocus();
}

void GuiCitation::on_searchPB_clicked()
{
	findText(findLE->text(), true);
}


void GuiCitation::on_caseCB_stateChanged(int)
{
	findText(findLE->text());
}


void GuiCitation::on_regexCB_stateChanged(int)
{
	findText(findLE->text());
}


void GuiCitation::on_asTypeCB_stateChanged(int)
{
	bool const searchAsWeGo = (asTypeCB->checkState() == Qt::Checked);
	searchPB->setDisabled(findLE->text().isEmpty() || searchAsWeGo);
	if (searchAsWeGo)
		findText(findLE->text(), true);
}


void GuiCitation::changed()
{
	setButtons();
}


void GuiCitation::apply(int const choice, bool full, bool force,
	QString before, QString after)
{
	if (cited_keys_.isEmpty())
		return;

	vector<CiteStyle> const & styles = citeStyles_;
	if (styles[choice] == NOCITE) {
		full = false;
		force = false;
		before.clear();
		after.clear();
	}
	
	CitationStyle s;
	s.style = styles[choice];
	s.full = full;
	s.forceUpperCase = force;
	string const command = citationStyleToString(s);

	params_.setCmdName(command);
	params_["key"] = qstring_to_ucs4(cited_keys_.join(","));
	params_["before"] = qstring_to_ucs4(before);
	params_["after"] = qstring_to_ucs4(after);
	dispatchParams();
}


void GuiCitation::clearSelection()
{
	cited_keys_.clear();
	selected_model_.setStringList(cited_keys_);
}


void GuiCitation::init()
{
	// Make the list of all available bibliography keys
	BiblioInfo const & bi = bibInfo();
	all_keys_ = to_qstring_list(bi.getKeys());
	available_model_.setStringList(all_keys_);

	// Ditto for the keys cited in this inset
	QString str = toqstr(params_["key"]);
	if (str.isEmpty())
		cited_keys_.clear();
	else
		cited_keys_ = str.split(",");
	selected_model_.setStringList(cited_keys_);
	if (selected_model_.rowCount()) {
		selectedLV->blockSignals(true);
		selectedLV->setFocus();
		QModelIndex idx = selected_model_.index(0, 0);
		selectedLV->selectionModel()->select(idx, 
				QItemSelectionModel::ClearAndSelect);
		selectedLV->blockSignals(false);
		
		// set the style combo appropriately
		string const & command = params_.getCmdName();
		vector<CiteStyle> const & styles = citeStyles_;
		CitationStyle const cs = citationStyleFromString(command);
	
		vector<CiteStyle>::const_iterator cit =
			std::find(styles.begin(), styles.end(), cs.style);
		if (cit != styles.end()) {
			int const i = int(cit - styles.begin());
			// the necessary update will be performed later
			citationStyleCO->blockSignals(true);
			citationStyleCO->setCurrentIndex(i);
			citationStyleCO->blockSignals(false);
		}
	} else
		availableLV->setFocus();
	fillFields(bi);
	fillEntries(bi);
	updateControls(bi);
}


void GuiCitation::findKey(BiblioInfo const & bi,
	QString const & str, bool only_keys,
	docstring field, docstring entry_type,
	bool case_sensitive, bool reg_exp, bool reset)
{
	// Used for optimisation: store last searched string.
	static QString last_searched_string;
	// Used to disable the above optimisation.
	static bool last_case_sensitive;
	static bool last_reg_exp;
	// Reset last_searched_string in case of changed option.
	if (last_case_sensitive != case_sensitive
		|| last_reg_exp != reg_exp) {
			LYXERR(Debug::GUI, "GuiCitation::findKey: optimisation disabled!");
		last_searched_string.clear();
	}
	// save option for next search.
	last_case_sensitive = case_sensitive;
	last_reg_exp = reg_exp;

	Qt::CaseSensitivity qtcase = case_sensitive ?
			Qt::CaseSensitive: Qt::CaseInsensitive;
	QStringList keys;
	// If new string (str) contains the last searched one...
	if (!reset &&
		!last_searched_string.isEmpty() &&
		str.size() > 1 &&
		str.contains(last_searched_string, qtcase))
		// ... then only search within already found list.
		keys = available_model_.stringList();
	else
		// ... else search all keys.
		keys = all_keys_;
	// save searched string for next search.
	last_searched_string = str;

	QStringList result;
	
	// First, filter by entry_type, which will be faster than 
	// what follows, so we may get to do that on less.
	vector<docstring> keyVector = to_docstring_vector(keys);
	filterByEntryType(bi, keyVector, entry_type);
	
	if (str.isEmpty())
		result = to_qstring_list(keyVector);
	else
		result = to_qstring_list(searchKeys(bi, keyVector, only_keys, 
			qstring_to_ucs4(str), field, case_sensitive, reg_exp));
	
	available_model_.setStringList(result);
}


QStringList GuiCitation::citationStyles(BiblioInfo const & bi, int sel)
{
	docstring const key = qstring_to_ucs4(cited_keys_[sel]);
	return to_qstring_list(bi.getCiteStrings(key, buffer()));
}


void GuiCitation::setCitedKeys() 
{
	cited_keys_ = selected_model_.stringList();
}


bool GuiCitation::initialiseParams(string const & data)
{
	InsetCommand::string2params(data, params_);
	CiteEngine const engine = citeEngine();
	citeStyles_ = citeStyles(engine);
	init();
	return true;
}


void GuiCitation::clearParams()
{
	params_.clear();
}


void GuiCitation::filterByEntryType(BiblioInfo const & bi,
	vector<docstring> & keyVector, docstring entry_type) 
{
	if (entry_type.empty())
		return;
	
	vector<docstring>::iterator it = keyVector.begin();
	vector<docstring>::iterator end = keyVector.end();

	vector<docstring> result;
	for (; it != end; ++it) {
		docstring const key = *it;
		BiblioInfo::const_iterator cit = bi.find(key);
		if (cit == bi.end())
			continue;
		if (cit->second.entryType() == entry_type)
			result.push_back(key);
	}
	keyVector = result;
}


CiteEngine GuiCitation::citeEngine() const
{
	return buffer().params().citeEngine();
}


// Escape special chars.
// All characters are literals except: '.|*?+(){}[]^$\'
// These characters are literals when preceded by a "\", which is done here
// @todo: This function should be moved to support, and then the test in tests
//        should be moved there as well.
static docstring escape_special_chars(docstring const & expr)
{
	// Search for all chars '.|*?+(){}[^$]\'
	// Note that '[' and '\' must be escaped.
	// This is a limitation of lyx::regex, but all other chars in BREs
	// are assumed literal.
	static const lyx::regex reg("[].|*?+(){}^$\\[\\\\]");

	// $& is a perl-like expression that expands to all
	// of the current match
	// The '$' must be prefixed with the escape character '\' for
	// boost to treat it as a literal.
	// Thus, to prefix a matched expression with '\', we use:
	// FIXME: UNICODE
	return from_utf8(lyx::regex_replace(to_utf8(expr), reg, string("\\\\$&")));
}


vector<docstring> GuiCitation::searchKeys(BiblioInfo const & bi,	
	vector<docstring> const & keys_to_search, bool only_keys,
 	docstring const & search_expression, docstring field,
	bool case_sensitive, bool regex)
{
	vector<docstring> foundKeys;

	docstring expr = trim(search_expression);
	if (expr.empty())
		return foundKeys;

	if (!regex)
		// We must escape special chars in the search_expr so that
		// it is treated as a simple string by lyx::regex.
		expr = escape_special_chars(expr);

	lyx::regex reg_exp;
	try {
		reg_exp.assign(to_utf8(expr), case_sensitive ?
			lyx::regex_constants::ECMAScript : lyx::regex_constants::icase);
	} catch (lyx::regex_error & e) {
		// lyx::regex throws an exception if the regular expression is not
		// valid.
		LYXERR(Debug::GUI, e.what());
		return vector<docstring>();
	}

	vector<docstring>::const_iterator it = keys_to_search.begin();
	vector<docstring>::const_iterator end = keys_to_search.end();
	for (; it != end; ++it ) {
		BiblioInfo::const_iterator info = bi.find(*it);
		if (info == bi.end())
			continue;
		
		BibTeXInfo const & kvm = info->second;
		string data;
		if (only_keys)
			data = to_utf8(*it);
		else if (field.empty())
			data = to_utf8(*it) + ' ' + to_utf8(kvm.allData());
		else 
			data = to_utf8(kvm[field]);
		
		if (data.empty())
			continue;

		try {
			if (lyx::regex_search(data, reg_exp))
				foundKeys.push_back(*it);
		}
		catch (lyx::regex_error & e) {
			LYXERR(Debug::GUI, e.what());
			return vector<docstring>();
		}
	}
	return foundKeys;
}


void GuiCitation::dispatchParams()
{
	std::string const lfun = InsetCommand::params2string(params_);
	dispatch(FuncRequest(getLfun(), lfun));
}


BiblioInfo const & GuiCitation::bibInfo() const
{
	Buffer const & buf = buffer();
	buf.reloadBibInfoCache();
	return buf.masterBibInfo();
}


Buffer const & GuiCitation::buffer() const
{
	if (lyxview().documentBufferView())
		return lyxview().documentBufferView()->buffer();
	else
		return lyxview().currentBufferView()->buffer();
}


void GuiCitation::saveSession() const
{
	Dialog::saveSession();
	QSettings settings;
	settings.setValue(
		sessionKey() + "/regex", regexCB->isChecked());
	settings.setValue(
		sessionKey() + "/casesensitive", caseCB->isChecked());
	settings.setValue(
		sessionKey() + "/autofind", asTypeCB->isChecked());
}


void GuiCitation::restoreSession()
{
	Dialog::restoreSession();
	QSettings settings;
	regexCB->setChecked(
		settings.value(sessionKey() + "/regex").toBool());
	caseCB->setChecked(
		settings.value(sessionKey() + "/casesensitive").toBool());
	asTypeCB->setChecked(
		settings.value(sessionKey() + "/autofind").toBool());
}


Dialog * createGuiCitation(GuiView & lv) { return new GuiCitation(lv); }


} // namespace frontend
} // namespace lyx

#include "moc_GuiCitation.cpp"

