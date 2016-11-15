/*
	Author: Richard Zhang.
	Mail: 89205975@qq.com

	This library filters sensitive phrases by user's configuration.
	Currently, only support UTF8 & ANSI encoded strings.

	The matching rule is max-length-matching, the library tries to match sensitive phrase as long as possible.
		For example:
		"damn fucker" and "damn" are all in sensitive dictionary, the sentence "he's a damn fucker" will be processed to "he's a ***********".

	Even user insert some spaces or non-letter characters between sensitive words, the library is also able to deal with it.
		For example:
		"Bad boy" is added to sensitive dictionary, "Bad.boy", "Bad     boy", "Bad/boy" can also be filtered.
		"你去死" is added to sensitive dictionary, "你 去    死", "你/去    死", "你 去 .死" can also be filtered.



	Compiling requirement:
		1. STL C++11
		2. BOOST multi_index_container



	Performance test condition:
		1. Giving a sentence around 100 bytes (English & Chinese mixed)
		2. Dirty phrases around 10,000
		3. Do 1,000 loop test
		4. Intel I7 CPU

	Test result:
		For each loop, it cost around 100us
*/



#ifndef CPHRASEFILTER_H
#define CPHRASEFILTER_H

#include <string>
#include <vector>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>

class CPhraseFilter
{
public:
	CPhraseFilter();
	virtual ~CPhraseFilter();



public:
	/*
		set whether we should ignore letter case during filtering. default is "false"
	*/
	void IgnoreCase(const bool bIgnoreCase);



	/*
		Add a new sensitive phrase to library
	*/
	bool AddPhrase(const std::string &strPhrase);



	/*
		Check whether the input text has sensitive phrase
		Return true if text passed check, return false if text has sensitive phrase(s)
	*/
	bool CheckText(const std::string &strInput) const;



	/*
		Process a string using library filter, sensitive characters are replaced by '*'
	*/
	std::string ProcessText(const std::string &strInput) const;



	/*
		Get character count in the string.
		A Chinese (multi-byte) character stands for 1 character, an English (single-byte) letter stands for 1 character.
		This function is useful when performing user-input-text length test
	*/
	static int GetUTF8CharacterCount(const std::string &strInput);



	/*
		Get character count in the string, summarize [single-byte / multi-byte] UTF8 character count separately
		A Chinese (multi-byte) character stands for 1 character, an English (single-byte) letter stands for 1 character.
		This function is also useful when performing user-input-text length test which requires distinguish Chinese/English
	*/
	static void GetUTF8CharacterCount(const std::string &strInput, int *piSingleByteCharacterCount, int *piMultiByteCharacterCount);



private:
	struct T_PhraseInfo
	{
		std::string strOriginalPhrase;

		std::string strFirstUnit;

		std::vector<std::string> vecUnit;
		int iUnitCount;
	};

	struct IndexTag_OriginalPhrase {};
	struct IndexTag_FirstUnit_UnitCount {};

	typedef
	boost::multi_index::multi_index_container
	<
        T_PhraseInfo,

        boost::multi_index::indexed_by
		<
			boost::multi_index::hashed_unique
			<
				boost::multi_index::tag<IndexTag_OriginalPhrase>,
				boost::multi_index::member<T_PhraseInfo, std::string, &T_PhraseInfo::strOriginalPhrase>
			>,

			boost::multi_index::ordered_non_unique
			<
				boost::multi_index::tag<IndexTag_FirstUnit_UnitCount>,
				boost::multi_index::composite_key
				<
					T_PhraseInfo,
					boost::multi_index::member<T_PhraseInfo, std::string, &T_PhraseInfo::strFirstUnit>,
					boost::multi_index::member<T_PhraseInfo, int, &T_PhraseInfo::iUnitCount>
				>,
				boost::multi_index::composite_key_compare
				<
					std::less<std::string>,
					std::greater<int>
				>
			>
		>
	>
	PhraseContainer;

	PhraseContainer m_micPhrase;

	bool m_bIgnoreCase;

private:
	static inline int GetUTF8CharacterByteIndicator(const unsigned char uc);
	static void ToLower(std::string &strInput);

	struct T_UnitInfo
	{
		std::string strUnit;
		size_t stStartPos;
		size_t stEndPos;
	};
	static bool GetUTF8Unit(const std::string &strInput, size_t &stIteratorPos, T_UnitInfo *pUnitInfo);
};

#endif // CPHRASEFILTER_H
