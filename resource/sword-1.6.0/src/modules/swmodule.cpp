/******************************************************************************
 *  swmodule.cpp - code for base class 'module'.  Module is the basis for all
 *		   types of modules (e.g. texts, commentaries, maps, lexicons,
 *		   etc.)
 *
 *
 * Copyright 2009 CrossWire Bible Society (http://www.crosswire.org)
 *	CrossWire Bible Society
 *	P. O. Box 2528
 *	Tempe, AZ  85280-2528
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */


#include <vector>

#include <swlog.h>
#include <sysdata.h>
#include <swmodule.h>
#include <utilstr.h>
#include <regex.h>	// GNU
#include <swfilter.h>
#include <versekey.h>	// KLUDGE for Search
#include <treekeyidx.h>	// KLUDGE for Search
#include <swoptfilter.h>
#include <filemgr.h>
#include <stringmgr.h>
#ifndef _MSC_VER
#include <iostream>
#endif

#ifdef USELUCENE
#include <CLucene.h>
#include <CLucene/CLBackwards.h>

//Lucence includes
//#include "CLucene.h"
//#include "CLucene/util/Reader.h"
//#include "CLucene/util/Misc.h"
//#include "CLucene/util/dirent.h"

using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::util;
using namespace lucene::store;
using namespace lucene::document;
using namespace lucene::queryParser;
using namespace lucene::search;
#endif

using std::vector;

SWORD_NAMESPACE_START

SWDisplay SWModule::rawdisp;

typedef std::list<SWBuf> StringList;

/******************************************************************************
 * SWModule Constructor - Initializes data for instance of SWModule
 *
 * ENT:	imodname - Internal name for module
 *	imoddesc - Name to display to user for module
 *	idisp	 - Display object to use for displaying
 *	imodtype - Type of Module (All modules will be displayed with
 *			others of same type under their modtype heading
 *	unicode  - if this module is unicode
 */

SWModule::SWModule(const char *imodname, const char *imoddesc, SWDisplay *idisp, const char *imodtype, SWTextEncoding encoding, SWTextDirection direction, SWTextMarkup markup, const char *imodlang) {
	key       = CreateKey();
	entryBuf  = "";
	config    = &ownConfig;
	modname   = 0;
	error     = 0;
	moddesc   = 0;
	modtype   = 0;
	modlang   = 0;
	this->encoding = encoding;
	this->direction = direction;
	this->markup  = markup;
	entrySize= -1;
	disp     = (idisp) ? idisp : &rawdisp;
	stdstr(&modname, imodname);
	stdstr(&moddesc, imoddesc);
	stdstr(&modtype, imodtype);
	stdstr(&modlang, imodlang);
	stripFilters = new FilterList();
	rawFilters = new FilterList();
	renderFilters = new FilterList();
	optionFilters = new OptionFilterList();
	encodingFilters = new FilterList();
	skipConsecutiveLinks = true;
	procEntAttr = true;
}


/******************************************************************************
 * SWModule Destructor - Cleans up instance of SWModule
 */

SWModule::~SWModule()
{
	if (modname)
		delete [] modname;
	if (moddesc)
		delete [] moddesc;
	if (modtype)
		delete [] modtype;
	if (modlang)
		delete [] modlang;

	if (key) {
		if (!key->Persist())
			delete key;
	}

	stripFilters->clear();
     rawFilters->clear();
     renderFilters->clear();
     optionFilters->clear();
     encodingFilters->clear();
	entryAttributes.clear();

     delete stripFilters;
     delete rawFilters;
     delete renderFilters;
     delete optionFilters;
     delete encodingFilters;
}


/******************************************************************************
 * SWModule::CreateKey - Allocates a key of specific type for module
 *
 * RET:	pointer to allocated key
 */

SWKey *SWModule::CreateKey() const
{
	return new SWKey();
}


/******************************************************************************
 * SWModule::Error - Gets and clears error status
 *
 * RET:	error status
 */

char SWModule::Error()
{
	char retval = error;

	error = 0;
	return retval;
}


/******************************************************************************
 * SWModule::Name - Sets/gets module name
 *
 * ENT:	imodname - value which to set modname
 *		[0] - only get
 *
 * RET:	pointer to modname
 */

char *SWModule::Name(const char *imodname) {
	return stdstr(&modname, imodname);
}

char *SWModule::Name() const {
	return modname;
}


/******************************************************************************
 * SWModule::Description - Sets/gets module description
 *
 * ENT:	imoddesc - value which to set moddesc
 *		[0] - only get
 *
 * RET:	pointer to moddesc
 */

char *SWModule::Description(const char *imoddesc) {
	return stdstr(&moddesc, imoddesc);
}

char *SWModule::Description() const {
	return moddesc;
}


/******************************************************************************
 * SWModule::Type - Sets/gets module type
 *
 * ENT:	imodtype - value which to set modtype
 *		[0] - only get
 *
 * RET:	pointer to modtype
 */

char *SWModule::Type(const char *imodtype) {
	return stdstr(&modtype, imodtype);
}

char *SWModule::Type() const {
	return modtype;
}

/******************************************************************************
 * SWModule::Direction - Sets/gets module direction
 *
 * ENT:	newdir - value which to set direction
 *		[-1] - only get
 *
 * RET:	char direction
 */
char SWModule::Direction(signed char newdir) {
        if (newdir != -1)
                direction = newdir;
        return direction;
}

/******************************************************************************
 * SWModule::Encoding - Sets/gets module encoding
 *
 * ENT:	newdir - value which to set direction
 *		[-1] - only get
 *
 * RET:	char encoding
 */
char SWModule::Encoding(signed char newenc) {
        if (newenc != -1)
                encoding = newenc;
        return encoding;
}

/******************************************************************************
 * SWModule::Markup - Sets/gets module markup
 *
 * ENT:	newdir - value which to set direction
 *		[-1] - only get
 *
 * RET:	char markup
 */
char SWModule::Markup(signed char newmark) {
        if (newmark != -1)
                markup = newmark;
        return markup;
}


/******************************************************************************
 * SWModule::Lang - Sets/gets module language
 *
 * ENT:	imodlang - value which to set modlang
 *		[0] - only get
 *
 * RET:	pointer to modname
 */

char *SWModule::Lang(const char *imodlang)
{
	if (imodlang) stdstr(&modlang, imodlang);
	return modlang;
}


/******************************************************************************
 * SWModule::Disp - Sets/gets display driver
 *
 * ENT:	idisp - value which to set disp
 *		[0] - only get
 *
 * RET:	pointer to disp
 */

SWDisplay *SWModule::getDisplay() const {
	return disp;
}

void SWModule::setDisplay(SWDisplay *idisp) {
	disp = idisp;
}


/******************************************************************************
 * SWModule::Display - Calls this modules display object and passes itself
 *
 * RET:	error status
 */

char SWModule::Display() {
	disp->Display(*this);
	return 0;
}


/******************************************************************************
 * SWModule::getKey - Gets the key from this module that points to the position
 *			record
 *
 * RET:	key object
 */

SWKey *SWModule::getKey() const {
	return key;
}


/******************************************************************************
 * SWModule::setKey - Sets a key to this module for position to a particular
 *			record
 *
 * ENT:	ikey - key with which to set this module
 *
 * RET:	error status
 */

char SWModule::setKey(const SWKey *ikey) {
	SWKey *oldKey = 0;

	if (key) {
		if (!key->Persist())	// if we have our own copy
			oldKey = key;
	}

	if (!ikey->Persist()) {		// if we are to keep our own copy
		 key = CreateKey();
		*key = *ikey;
	}
	else	 key = (SWKey *)ikey;		// if we are to just point to an external key

	if (oldKey)
		delete oldKey;

	return 0;
}


/******************************************************************************
 * SWModule::setPosition(SW_POSITION)	- Positions this modules to an entry
 *
 * ENT:	p	- position (e.g. TOP, BOTTOM)
 *
 * RET: *this
 */

void SWModule::setPosition(SW_POSITION p) {
	*key = p;
	char saveError = key->Error();

	switch (p) {
	case POS_TOP:
		(*this)++;
		(*this)--;
		break;

	case POS_BOTTOM:
		(*this)--;
		(*this)++;
		break;
	}

	error = saveError;
}


/******************************************************************************
 * SWModule::increment	- Increments module key a number of entries
 *
 * ENT:	increment	- Number of entries to jump forward
 *
 * RET: *this
 */

void SWModule::increment(int steps) {
	(*key) += steps;
	error = key->Error();
}


/******************************************************************************
 * SWModule::decrement	- Decrements module key a number of entries
 *
 * ENT:	decrement	- Number of entries to jump backward
 *
 * RET: *this
 */

void SWModule::decrement(int steps) {
	(*key) -= steps;
	error = key->Error();
}


/******************************************************************************
 * SWModule::Search 	- Searches a module for a string
 *
 * ENT:	istr		- string for which to search
 * 	searchType	- type of search to perform
 *				>=0 - regex
 *				-1  - phrase
 *				-2  - multiword
 *				-3  - entryAttrib (eg. Word//Lemma./G1234/)	 (Lemma with dot means check components (Lemma.[1-9]) also)
 *				-4  - clucene
 *				-5  - multilemma window; flags = window size
 * 	flags		- options flags for search
 *	justCheckIfSupported	- if set, don't search, only tell if this
 *							function supports requested search.
 *
 * RET: ListKey set to verses that contain istr
 */

ListKey &SWModule::search(const char *istr, int searchType, int flags, SWKey *scope, bool *justCheckIfSupported, void (*percent)(char, void *), void *percentUserData) {

	listKey.ClearList();
	SWBuf term = istr;
	bool includeComponents = false;	// for entryAttrib e.g., /Lemma.1/ 

#ifdef USELUCENE
	SWBuf target = getConfigEntry("AbsoluteDataPath");
	char ch = target.c_str()[strlen(target.c_str())-1];
	if ((ch != '/') && (ch != '\\'))
		target.append('/');
	target.append("lucene");
#endif
	if (justCheckIfSupported) {
		*justCheckIfSupported = (searchType >= -3);
#ifdef USELUCENE
		if ((searchType == -4) && (IndexReader::indexExists(target.c_str()))) {
			*justCheckIfSupported = true;
		}
#endif
		return listKey;
	}
	
	SWKey *saveKey = 0;
	SWKey *searchKey = 0;
	SWKey *resultKey = CreateKey();
	regex_t preg;
	vector<SWBuf> words;
	vector<SWBuf> window;
	const char *sres;
	terminateSearch = false;
	char perc = 1;
	bool savePEA = isProcessEntryAttributes();

	// determine if we might be doing special strip searches.  useful for knowing if we can use shortcuts
	bool specialStrips = (getConfigEntry("LocalStripFilter")
                       || (getConfig().has("GlobalOptionFilter", "UTF8GreekAccents"))
                       || (strchr(istr, '<')));

	processEntryAttributes(searchType == -3);
	

	if (!key->Persist()) {
		saveKey = CreateKey();
		*saveKey = *key;
	}
	else	saveKey = key;

	searchKey = (scope)?scope->clone():(key->Persist())?key->clone():0;
	if (searchKey) {
		searchKey->Persist(1);
		setKey(*searchKey);
	}

	(*percent)(perc, percentUserData);

	*this = BOTTOM;
	long highIndex = key->Index();
	if (!highIndex)
		highIndex = 1;		// avoid division by zero errors.
	*this = TOP;
	if (searchType >= 0) {
		flags |=searchType|REG_NOSUB|REG_EXTENDED;
		regcomp(&preg, istr, flags);
	}

	(*percent)(++perc, percentUserData);


#ifdef USELUCENE
	if (searchType == -4) {	// lucene
		//Buffers for the wchar<->utf8 char* conversion
		const unsigned short int MAX_CONV_SIZE = 2047;
		wchar_t wcharBuffer[MAX_CONV_SIZE + 1];
		char utfBuffer[MAX_CONV_SIZE + 1];
		
		lucene::index::IndexReader    *ir = 0;
		lucene::search::IndexSearcher *is = 0;
		Query                         *q  = 0;
		Hits                          *h  = 0;
		SWTRY {
			ir = IndexReader::open(target);
			is = new IndexSearcher(ir);
			(*percent)(10, percentUserData);

			standard::StandardAnalyzer analyzer;
			lucene_utf8towcs(wcharBuffer, istr, MAX_CONV_SIZE); //TODO Is istr always utf8?
			q = QueryParser::parse(wcharBuffer, _T("content"), &analyzer);
			(*percent)(20, percentUserData);
			h = is->search(q);
			(*percent)(80, percentUserData);

			// iterate thru each good module position that meets the search
			bool checkBounds = getKey()->isBoundSet();
			for (long i = 0; i < h->length(); i++) {
				Document &doc = h->doc(i);

				// set a temporary verse key to this module position
				lucene_wcstoutf8(utfBuffer, doc.get(_T("key")), MAX_CONV_SIZE);	
				*resultKey = utfBuffer; //TODO Does a key always accept utf8?

				// check to see if it sets ok (within our bounds) and if not, skip
				if (checkBounds) {
					*getKey() = *resultKey;
					if (*getKey() != *resultKey) {
						continue;
					}
				}
				listKey << *resultKey;
				listKey.GetElement()->userData = (void *)((__u32)(h->score(i)*100));
			}
			(*percent)(98, percentUserData);
		}
		SWCATCH (...) {
			q = 0;
			// invalid clucene query
		}
		delete h;
		delete q;

		delete is;
		if (ir) {
			ir->close();
		}
	}
#endif

	// some pre-loop processing
	switch (searchType) {

	// phrase
	case -1:
		// let's see if we're told to ignore case.  If so, then we'll touppstr our term
		if ((flags & REG_ICASE) == REG_ICASE) toupperstr(term);
		break;

	// multi-word
	case -2:
	case -5:
		// let's break the term down into our words vector
		while (1) {
			const char *word = term.stripPrefix(' ');
			if (!word) {
				words.push_back(term);
				break;
			}
			words.push_back(word);
		}
		if ((flags & REG_ICASE) == REG_ICASE) {
			for (unsigned int i = 0; i < words.size(); i++) {
				toupperstr(words[i]);
			}
		}
		break;

	// entry attributes
	case -3:
		// let's break the attribute segs down.  We'll reuse our words vector for each segment
		while (1) {
			const char *word = term.stripPrefix('/');
			if (!word) {
				words.push_back(term);
				break;
			}
			words.push_back(word);
		}
		if ((words.size()>2) && words[2].endsWith(".")) {
			includeComponents = true;
			words[2]--;
		}
		break;
	}


	// our main loop to iterate the module and find the stuff
	perc = 5;
	(*percent)(perc, percentUserData);

	
	while ((searchType != -4) && !Error() && !terminateSearch) {
		long mindex = key->Index();
		float per = (float)mindex / highIndex;
		per *= 93;
		per += 5;
		char newperc = (char)per;
		if (newperc > perc) {
			perc = newperc;
			(*percent)(perc, percentUserData);
		}
		else if (newperc < perc) {
#ifndef _MSC_VER
			std::cerr << "Serious error: new percentage complete is less than previous value\n";
			std::cerr << "index: " << (key->Index()) << "\n";
			std::cerr << "highIndex: " << highIndex << "\n";
			std::cerr << "newperc ==" << (int)newperc << "%" << "is smaller than\n";
			std::cerr << "perc == "  << (int )perc << "% \n";
#endif
		}
		if (searchType >= 0) {
			if (!regexec(&preg,  StripText(), 0, 0, 0)) {
				*resultKey = *getKey();
				resultKey->clearBound();
				listKey << *resultKey;
			}
		}

		// phrase
		else {
			SWBuf textBuf;
			switch (searchType) {

			// phrase
			case -1:
				textBuf = StripText();
				if ((flags & REG_ICASE) == REG_ICASE) toupperstr(textBuf);
				sres = strstr(textBuf.c_str(), term.c_str());
				if (sres) { //it's also in the StripText(), so we have a valid search result item now
					*resultKey = *getKey();
					resultKey->clearBound();
					listKey << *resultKey;
				}
				break;

			// multiword
			case -2: { // enclose our allocations
				int loopCount = 0;
				unsigned int foundWords = 0;
				do {
					textBuf = ((loopCount == 0)&&(!specialStrips)) ? getRawEntry() : StripText();
					foundWords = 0;
					
					for (unsigned int i = 0; i < words.size(); i++) {
						if ((flags & REG_ICASE) == REG_ICASE) toupperstr(textBuf);
						sres = strstr(textBuf.c_str(), words[i].c_str());
						if (!sres) {
							break; //for loop
						}
						foundWords++;
					}
					
					loopCount++;
				} while ( (loopCount < 2) && (foundWords == words.size()));
				
				if ((loopCount == 2) && (foundWords == words.size())) { //we found the right words in both raw and stripped text, which means it's a valid result item
					*resultKey = *getKey();
					resultKey->clearBound();
					listKey << *resultKey;
				}
				} break;

			// entry attributes
			case -3: {
				RenderText();	// force parse
				AttributeTypeList &entryAttribs = getEntryAttributes();
				AttributeTypeList::iterator i1Start, i1End;
				AttributeList::iterator i2Start, i2End;
				AttributeValue::iterator i3Start, i3End;

				if ((words.size()) && (words[0].length())) {
					i1Start = entryAttribs.find(words[0]);
					i1End = i1Start;
					if (i1End != entryAttribs.end())
					i1End++;
				}
				else {
					i1Start = entryAttribs.begin();
					i1End   = entryAttribs.end();
				}
				for (;i1Start != i1End; i1Start++) {
					if ((words.size()>1) && (words[1].length())) {
						i2Start = i1Start->second.find(words[1]);
						i2End = i2Start;
						if (i2End != i1Start->second.end())
							i2End++;
					}
					else {
						i2Start = i1Start->second.begin();
						i2End   = i1Start->second.end();
					}
					for (;i2Start != i2End; i2Start++) {
						if ((words.size()>2) && (words[2].length()) && (!includeComponents)) {
							i3Start = i2Start->second.find(words[2]);
							i3End = i3Start;
							if (i3End != i2Start->second.end())
								i3End++;
						}
						else {
							i3Start = i2Start->second.begin();
							i3End   = i2Start->second.end();
						}
						for (;i3Start != i3End; i3Start++) {
							if ((words.size()>3) && (words[3].length())) {
								if (includeComponents) {
									SWBuf key = i3Start->first.c_str();
									key = key.stripPrefix('.', true);
									// we're iterating all 3 level keys, so be sure we match our
									// prefix (e.g., Lemma, Lemma.1, Lemma.2, etc.)
									if (key != words[2]) continue;
								}
								if (flags & SEARCHFLAG_MATCHWHOLEENTRY) {
									bool found = !(((flags & REG_ICASE) == REG_ICASE) ? sword::stricmp(i3Start->second.c_str(), words[3]) : strcmp(i3Start->second.c_str(), words[3]));
									sres = (found) ? i3Start->second.c_str() : 0;
								}
								else {
									sres = ((flags & REG_ICASE) == REG_ICASE) ? stristr(i3Start->second.c_str(), words[3]) : strstr(i3Start->second.c_str(), words[3]);
								}
								if (sres) {
									*resultKey = *getKey();
									resultKey->clearBound();
									listKey << *resultKey;
									break;
								}
							}
						}
						if (i3Start != i3End)
							break;
					}
					if (i2Start != i2End)
						break;
				}
				break;
			}
			case -5:
				AttributeList &words = getEntryAttributes()["Word"];
				SWBuf kjvWord = "";
				SWBuf bibWord = "";
				for (AttributeList::iterator it = words.begin(); it != words.end(); it++) {
					int parts = atoi(it->second["PartCount"]);
					SWBuf lemma = "";
					SWBuf morph = "";
					for (int i = 1; i <= parts; i++) {
						SWBuf key = "";
						key = (parts == 1) ? "Lemma" : SWBuf().setFormatted("Lemma.%d", i).c_str();
						AttributeValue::iterator li = it->second.find(key);
						if (li != it->second.end()) {
							if (i > 1) lemma += " ";
							key = (parts == 1) ? "LemmaClass" : SWBuf().setFormatted("LemmaClass.%d", i).c_str();
							AttributeValue::iterator lci = it->second.find(key);
							if (lci != it->second.end()) {
								lemma += lci->second + ":";
							}
							lemma += li->second;
						}
						key = (parts == 1) ? "Morph" : SWBuf().setFormatted("Morph.%d", i).c_str();
						li = it->second.find(key);
						// silly.  sometimes morph counts don't equal lemma counts
						if (i == 1 && parts != 1 && li == it->second.end()) {
							li = it->second.find("Morph");
						}
						if (li != it->second.end()) {
							if (i > 1) morph += " ";
							key = (parts == 1) ? "MorphClass" : SWBuf().setFormatted("MorphClass.%d", i).c_str();
							AttributeValue::iterator lci = it->second.find(key);
							// silly.  sometimes morph counts don't equal lemma counts
							if (i == 1 && parts != 1 && lci == it->second.end()) {
								lci = it->second.find("MorphClass");
							}
							if (lci != it->second.end()) {
								morph += lci->second + ":";
							}
							morph += li->second;
						}
						// TODO: add src tags and maybe other attributes
					}
					while (window.size() < (unsigned)flags) {
						
					}
				}
				break;
			} // end switch
		}
		(*this)++;
	}
	

	// cleaup work
	if (searchType >= 0)
		regfree(&preg);

	setKey(*saveKey);

	if (!saveKey->Persist())
		delete saveKey;

	if (searchKey)
		delete searchKey;
	delete resultKey;

	listKey = TOP;
	processEntryAttributes(savePEA);


	(*percent)(100, percentUserData);


	return listKey;
}


/******************************************************************************
 * SWModule::StripText() 	- calls all stripfilters on current text
 *
 * ENT:	buf	- buf to massage instead of this modules current text
 * 	len	- max len of buf
 *
 * RET: this module's text at current key location massaged by Strip filters
 */

const char *SWModule::StripText(const char *buf, int len) {
	return RenderText(buf, len, false);
}


/******************************************************************************
 * SWModule::RenderText 	- calls all renderfilters on current text
 *
 * ENT:	buf	- buffer to Render instead of current module position
 *
 * RET: this module's text at current key location massaged by RenderText filters
 */

 const char *SWModule::RenderText(const char *buf, int len, bool render) {
	entryAttributes.clear();

	static SWBuf local;
	if (buf)
		local = buf;

	SWBuf &tmpbuf = (buf) ? local : getRawEntryBuf();
	SWKey *key = 0;
	static const char *null = "";

	if (tmpbuf) {
		unsigned long size = (len < 0) ? ((getEntrySize()<0) ? strlen(tmpbuf) : getEntrySize()) : len;
		if (size > 0) {
			key = (SWKey *)*this;

			optionFilter(tmpbuf, key);
	
			if (render) {
				renderFilter(tmpbuf, key);
				encodingFilter(tmpbuf, key);
			}
			else	stripFilter(tmpbuf, key);
		}
	}
	else {
		tmpbuf = null;
	}

	return tmpbuf;
}


/******************************************************************************
 * SWModule::RenderText 	- calls all renderfilters on current text
 *
 * ENT:	tmpKey	- key to use to grab text
 *
 * RET: this module's text at current key location massaged by RenderFilers
 */

 const char *SWModule::RenderText(const SWKey *tmpKey) {
	SWKey *saveKey;
	const char *retVal;

	if (!key->Persist()) {
		saveKey = CreateKey();
		*saveKey = *key;
	}
	else	saveKey = key;

	setKey(*tmpKey);

	retVal = RenderText();

	setKey(*saveKey);

	if (!saveKey->Persist())
		delete saveKey;

	return retVal;
}


/******************************************************************************
 * SWModule::StripText 	- calls all StripTextFilters on current text
 *
 * ENT:	tmpKey	- key to use to grab text
 *
 * RET: this module's text at specified key location massaged by Strip filters
 */

const char *SWModule::StripText(const SWKey *tmpKey) {
	SWKey *saveKey;
	const char *retVal;

	if (!key->Persist()) {
		saveKey = CreateKey();
		*saveKey = *key;
	}
	else	saveKey = key;

	setKey(*tmpKey);

	retVal = StripText();

	setKey(*saveKey);

	if (!saveKey->Persist())
		delete saveKey;

	return retVal;
}


const char *SWModule::getConfigEntry(const char *key) const {
	ConfigEntMap::iterator it = config->find(key);
	return (it != config->end()) ? it->second.c_str() : 0;
}


void SWModule::setConfig(ConfigEntMap *config) {
	this->config = config;
}


bool SWModule::hasSearchFramework() {
#ifdef USELUCENE
	return true;
#else
	return SWSearchable::hasSearchFramework();
#endif
}

void SWModule::deleteSearchFramework() {
#ifdef USELUCENE
	SWBuf target = getConfigEntry("AbsoluteDataPath");
	char ch = target.c_str()[strlen(target.c_str())-1];
	if ((ch != '/') && (ch != '\\'))
		target.append('/');
	target.append("lucene");

	FileMgr::removeDir(target.c_str());
#else
	SWSearchable::deleteSearchFramework();
#endif
}


signed char SWModule::createSearchFramework(void (*percent)(char, void *), void *percentUserData) {
#ifdef USELUCENE
	SWKey *saveKey = 0;
	SWKey *searchKey = 0;
	SWKey textkey;
	SWBuf c;


	// turn all filters to default values
	StringList filterSettings;
	for (OptionFilterList::iterator filter = optionFilters->begin(); filter != optionFilters->end(); filter++) {
		filterSettings.push_back((*filter)->getOptionValue());
		(*filter)->setOptionValue(*((*filter)->getOptionValues().begin()));

		if (!strcmp("Greek Accents", (*filter)->getOptionName())) {
			(*filter)->setOptionValue("Off");
		}
	}


	// be sure we give CLucene enough file handles
	FileMgr::getSystemFileMgr()->flush();

	// save key information so as not to disrupt original
	// module position
	if (!key->Persist()) {
		saveKey = CreateKey();
		*saveKey = *key;
	}
	else	saveKey = key;

	searchKey = (key->Persist())?key->clone():0;
	if (searchKey) {
		searchKey->Persist(1);
		setKey(*searchKey);
	}

	RAMDirectory *ramDir = NULL;
	IndexWriter *coreWriter = NULL;
	IndexWriter *fsWriter = NULL;
	Directory *d = NULL;

	standard::StandardAnalyzer *an = new standard::StandardAnalyzer();
	SWBuf target = getConfigEntry("AbsoluteDataPath");
	bool includeKeyInSearch = getConfig().has("SearchOption", "IncludeKeyInSearch");
	char ch = target.c_str()[strlen(target.c_str())-1];
	if ((ch != '/') && (ch != '\\'))
		target.append('/');
	target.append("lucene");
	FileMgr::createParent(target+"/dummy");

	ramDir = new RAMDirectory();
	coreWriter = new IndexWriter(ramDir, an, true);



	char perc = 1;
	VerseKey *vkcheck = 0;
	vkcheck = SWDYNAMIC_CAST(VerseKey, key);
	VerseKey *chapMax = 0;
        if (vkcheck) chapMax = (VerseKey *)vkcheck->clone();

	TreeKeyIdx *tkcheck = 0;
	tkcheck = SWDYNAMIC_CAST(TreeKeyIdx, key);


	*this = BOTTOM;
	long highIndex = key->Index();
	if (!highIndex)
		highIndex = 1;		// avoid division by zero errors.

	bool savePEA = isProcessEntryAttributes();
	processEntryAttributes(true);

	// prox chapter blocks
	// position module at the beginning
	*this = TOP;

	SWBuf proxBuf;
	SWBuf proxLem;
	SWBuf strong;

	const short int MAX_CONV_SIZE = 2047;
	wchar_t wcharBuffer[MAX_CONV_SIZE + 1];

	char err = Error();
	while (!err) {
		long mindex = key->Index();

		proxBuf = "";
		proxLem = "";

		// computer percent complete so we can report to our progress callback
		float per = (float)mindex / highIndex;
		// between 5%-98%
		per *= 93; per += 5;
		char newperc = (char)per;
		if (newperc > perc) {
			perc = newperc;
			(*percent)(perc, percentUserData);
		}

		// get "content" field
		const char *content = StripText();

		bool good = false;

		// start out entry
		Document *doc = new Document();
		// get "key" field
		SWBuf keyText = (vkcheck) ? vkcheck->getOSISRef() : getKeyText();
		if (content && *content) {
			good = true;


			// build "strong" field
			AttributeTypeList::iterator words;
			AttributeList::iterator word;
			AttributeValue::iterator strongVal;

			strong="";
			words = getEntryAttributes().find("Word");
			if (words != getEntryAttributes().end()) {
				for (word = words->second.begin();word != words->second.end(); word++) {
					int partCount = atoi(word->second["PartCount"]);
					if (!partCount) partCount = 1;
					for (int i = 0; i < partCount; i++) {
						SWBuf tmp = "Lemma";
						if (partCount > 1) tmp.appendFormatted(".%d", i+1);
						strongVal = word->second.find(tmp);
						if (strongVal != word->second.end()) {
							// cheeze.  skip empty article tags that weren't assigned to any text
							if (strongVal->second == "G3588") {
								if (word->second.find("Text") == word->second.end())
									continue;	// no text? let's skip
							}
							strong.append(strongVal->second);
							strong.append(' ');
						}
					}
				}
			}

			lucene_utf8towcs(wcharBuffer, keyText, MAX_CONV_SIZE); //keyText must be utf8
//			doc->add( *(new Field("key", wcharBuffer, Field::STORE_YES | Field::INDEX_TOKENIZED)));
			doc->add( *Field::Text(_T("key"), wcharBuffer ) );


			if (includeKeyInSearch) {
				c = keyText;
				c += " ";
				c += content;
				content = c.c_str();
			}

			lucene_utf8towcs(wcharBuffer, content, MAX_CONV_SIZE); //content must be utf8
			doc->add( *Field::UnStored(_T("content"), wcharBuffer) );

			if (strong.length() > 0) {
				lucene_utf8towcs(wcharBuffer, strong, MAX_CONV_SIZE);
				doc->add( *Field::UnStored(_T("lemma"), wcharBuffer) );
//printf("setting fields (%s).\ncontent: %s\nlemma: %s\n", (const char *)*key, content, strong.c_str());
			}

//printf("setting fields (%s).\n", (const char *)*key);
//fflush(stdout);
		}
		// don't write yet, cuz we have to see if we're the first of a prox block (5:1 or chapter5/verse1

		// for VerseKeys use chapter
		if (vkcheck) {
			*chapMax = *vkcheck;
			// we're the first verse in a chapter
			if (vkcheck->Verse() == 1) {
				*chapMax = MAXVERSE;
				VerseKey saveKey = *vkcheck;
				while ((!err) && (*vkcheck <= *chapMax)) {
//printf("building proxBuf from (%s).\nproxBuf.c_str(): %s\n", (const char *)*key, proxBuf.c_str());
//printf("building proxBuf from (%s).\n", (const char *)*key);

					content = StripText();
					if (content && *content) {
						// build "strong" field
						strong = "";
						AttributeTypeList::iterator words;
						AttributeList::iterator word;
						AttributeValue::iterator strongVal;

						words = getEntryAttributes().find("Word");
						if (words != getEntryAttributes().end()) {
							for (word = words->second.begin();word != words->second.end(); word++) {
								int partCount = atoi(word->second["PartCount"]);
								if (!partCount) partCount = 1;
								for (int i = 0; i < partCount; i++) {
									SWBuf tmp = "Lemma";
									if (partCount > 1) tmp.appendFormatted(".%d", i+1);
									strongVal = word->second.find(tmp);
									if (strongVal != word->second.end()) {
										// cheeze.  skip empty article tags that weren't assigned to any text
										if (strongVal->second == "G3588") {
											if (word->second.find("Text") == word->second.end())
												continue;	// no text? let's skip
										}
										strong.append(strongVal->second);
										strong.append(' ');
									}
								}
							}
						}
						proxBuf += content;
						proxBuf.append(' ');
						proxLem += strong;
						if (proxLem.length())
							proxLem.append("\n");
					}
					(*this)++;
					err = Error();
				}
				err = 0;
				*vkcheck = saveKey;
			}
		}

		// for TreeKeys use siblings if we have no children
		else if (tkcheck) {
			if (!tkcheck->hasChildren()) {
				if (!tkcheck->previousSibling()) {
					do {
//printf("building proxBuf from (%s).\n", (const char *)*key);
//fflush(stdout);

						content = StripText();
						if (content && *content) {
							// build "strong" field
							strong = "";
							AttributeTypeList::iterator words;
							AttributeList::iterator word;
							AttributeValue::iterator strongVal;

							words = getEntryAttributes().find("Word");
							if (words != getEntryAttributes().end()) {
								for (word = words->second.begin();word != words->second.end(); word++) {
									int partCount = atoi(word->second["PartCount"]);
									if (!partCount) partCount = 1;
									for (int i = 0; i < partCount; i++) {
										SWBuf tmp = "Lemma";
										if (partCount > 1) tmp.appendFormatted(".%d", i+1);
										strongVal = word->second.find(tmp);
										if (strongVal != word->second.end()) {
											// cheeze.  skip empty article tags that weren't assigned to any text
											if (strongVal->second == "G3588") {
												if (word->second.find("Text") == word->second.end())
													continue;	// no text? let's skip
											}
											strong.append(strongVal->second);
											strong.append(' ');
										}
									}
								}
							}

							proxBuf += content;
							proxBuf.append(' ');
							proxLem += strong;
							if (proxLem.length())
								proxLem.append("\n");
						}
					} while (tkcheck->nextSibling());
					tkcheck->parent();
					tkcheck->firstChild();
				}
				else tkcheck->nextSibling();	// reposition from our previousSibling test
			}
		}

		if (proxBuf.length() > 0) {

			lucene_utf8towcs(wcharBuffer, proxBuf, MAX_CONV_SIZE); //keyText must be utf8

//printf("proxBuf after (%s).\nprox: %s\nproxLem: %s\n", (const char *)*key, proxBuf.c_str(), proxLem.c_str());

			doc->add( *Field::UnStored(_T("prox"), wcharBuffer) );
			good = true;
		}
		if (proxLem.length() > 0) {
			lucene_utf8towcs(wcharBuffer, proxLem, MAX_CONV_SIZE); //keyText must be utf8
			doc->add( *Field::UnStored(_T("proxlem"), wcharBuffer) );
			good = true;
		}
		if (good) {
//printf("writing (%s).\n", (const char *)*key);
//fflush(stdout);
			coreWriter->addDocument(doc);
		}
		delete doc;

		(*this)++;
		err = Error();
	}

	// Optimizing automatically happens with the call to addIndexes
	//coreWriter->optimize();
	coreWriter->close();

	if (IndexReader::indexExists(target.c_str())) {
		d = FSDirectory::getDirectory(target.c_str(), false);
		if (IndexReader::isLocked(d)) {
			IndexReader::unlock(d);
		}

		fsWriter = new IndexWriter( d, an, false);
	} else {
		d = FSDirectory::getDirectory(target.c_str(), true);
		fsWriter = new IndexWriter( d ,an, true);
	}

	Directory *dirs[] = { ramDir, 0 };
	fsWriter->addIndexes(dirs);
	fsWriter->close();

	delete ramDir;
	delete coreWriter;
	delete fsWriter;
	delete an;

	// reposition module back to where it was before we were called
	setKey(*saveKey);

	if (!saveKey->Persist())
		delete saveKey;

	if (searchKey)
		delete searchKey;

        delete chapMax;

	processEntryAttributes(savePEA);

	// reset option filters back to original values
	StringList::iterator origVal = filterSettings.begin();
	for (OptionFilterList::iterator filter = optionFilters->begin(); filter != optionFilters->end(); filter++) {
		(*filter)->setOptionValue(*origVal++);
	}

	return 0;
#else
	return SWSearchable::createSearchFramework(percent, percentUserData);
#endif
}

/** OptionFilterBuffer a text buffer
 * @param filters the FilterList of filters to iterate
 * @param buf the buffer to filter
 * @param key key location from where this buffer was extracted
 */
void SWModule::filterBuffer(OptionFilterList *filters, SWBuf &buf, const SWKey *key) {
	OptionFilterList::iterator it;
	for (it = filters->begin(); it != filters->end(); it++) {
		(*it)->processText(buf, key, this);
	}
}

/** FilterBuffer a text buffer
 * @param filters the FilterList of filters to iterate
 * @param buf the buffer to filter
 * @param key key location from where this buffer was extracted
 */
void SWModule::filterBuffer(FilterList *filters, SWBuf &buf, const SWKey *key) {
	FilterList::iterator it;
	for (it = filters->begin(); it != filters->end(); it++) {
		(*it)->processText(buf, key, this);
	}
}

signed char SWModule::createModule(const char*) {
	return -1;
}

void SWModule::setEntry(const char*, long) {
}

void SWModule::linkEntry(const SWKey*) {
}


/******************************************************************************
 * SWModule::prepText	- Prepares the text before returning it to external
 *					objects
 *
 * ENT:	buf	- buffer where text is stored and where to store the prep'd
 *				text.
 */

void SWModule::prepText(SWBuf &buf) {
	unsigned int to, from; 
	char space = 0, cr = 0, realdata = 0, nlcnt = 0;
	char *rawBuf = buf.getRawData();
	for (to = from = 0; rawBuf[from]; from++) {
		switch (rawBuf[from]) {
		case 10:
			if (!realdata)
				continue;
			space = (cr) ? 0 : 1;
			cr = 0;
			nlcnt++;
			if (nlcnt > 1) {
//				*to++ = nl;
				rawBuf[to++] = 10;
//				*to++ = nl[1];
//				nlcnt = 0;
			}
			continue;
		case 13:
			if (!realdata)
				continue;
//			*to++ = nl[0];
			rawBuf[to++] = 10;
			space = 0;
			cr = 1;
			continue;
		}
		realdata = 1;
		nlcnt = 0;
		if (space) {
			space = 0;
			if (rawBuf[from] != ' ') {
				rawBuf[to++] = ' ';
				from--;
				continue;
			}
		}
		rawBuf[to++] = rawBuf[from];
	}
	buf.setSize(to);

	while (to > 1) {			// remove trailing excess
		to--;
		if ((rawBuf[to] == 10) || (rawBuf[to] == ' '))
			buf.setSize(to);
		else break;
	}
}

SWORD_NAMESPACE_END
