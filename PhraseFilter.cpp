#include "PhraseFilter.h"

using namespace std;

CPhraseFilter::CPhraseFilter()
{
	m_bIgnoreCase=false;
}

CPhraseFilter::~CPhraseFilter()
{
	//dtor
}

void CPhraseFilter::IgnoreCase(const bool bIgnoreCase)
{
	m_bIgnoreCase=bIgnoreCase;
}

bool CPhraseFilter::AddPhrase(const string &strPhrase)
{
	T_PhraseInfo phrase_info;

	phrase_info.strOriginalPhrase=strPhrase;

	size_t stPos;
	T_UnitInfo unit_info;

	//first unit
	stPos=0;
	if(!GetUTF8Unit(strPhrase, stPos, &unit_info))
	{
//		logw("In " << __FUNCTION__ << "(): failed to get first unit");
		return false;
	}

	if(m_bIgnoreCase)
		ToLower(unit_info.strUnit);

	phrase_info.strFirstUnit=unit_info.strUnit;

	//unit list
	stPos=0;
	while(GetUTF8Unit(strPhrase, stPos, &unit_info))
	{
		if(m_bIgnoreCase)
			ToLower(unit_info.strUnit);

		phrase_info.vecUnit.push_back(unit_info.strUnit);
	}

	//unit count
	phrase_info.iUnitCount=phrase_info.vecUnit.size();

	//add!!!
	if(m_micPhrase.get<IndexTag_OriginalPhrase>().insert(phrase_info).second==false)
	{
//		logw("In " << __FUNCTION__ << "(): failed to add phrase (" << phrase_info.strOriginalPhrase << ")");
		return false;
	}

//	logd("Phase (" << phrase_info.strOriginalPhrase << ") was added, (FirstUnit=" << phrase_info.strFirstUnit << ")");

	return true;
}

bool CPhraseFilter::CheckText(const std::string &strInput) const
{
	size_t stPos;
	vector<T_UnitInfo> vecUnit;

	{
		stPos=0;
		T_UnitInfo unit_info;
		while(GetUTF8Unit(strInput, stPos, &unit_info))
		{
			if(m_bIgnoreCase)
				ToLower(unit_info.strUnit);

			vecUnit.push_back(unit_info);
		}
	}

	for(size_t i=0; i<vecUnit.size(); i++)
	{
		T_UnitInfo &unit_info=vecUnit[i];
		size_t stRemainingUnitCount = vecUnit.size() - i;

		pair
		<
			PhraseContainer::index<IndexTag_FirstUnit_UnitCount>::type::const_iterator,
			PhraseContainer::index<IndexTag_FirstUnit_UnitCount>::type::const_iterator
		> pairRange =
		m_micPhrase.get<IndexTag_FirstUnit_UnitCount>().equal_range(unit_info.strUnit);

		for(PhraseContainer::index<IndexTag_FirstUnit_UnitCount>::type::const_iterator it=pairRange.first; it!=pairRange.second; it++)
		{
			if(it->vecUnit.size() > stRemainingUnitCount)
				continue;

			bool bAllMatch=true;
			for(size_t j=0; j<it->vecUnit.size(); j++)
			{
				if(it->vecUnit[j]!=vecUnit[i+j].strUnit)
				{
					bAllMatch=false;
					break;
				}
			}

			if(bAllMatch)
				return false;
		}
	}

	return true;
}

string CPhraseFilter::ProcessText(const string &strInput) const
{
	size_t stPos;
	vector<T_UnitInfo> vecUnit;

	{
		stPos=0;
		T_UnitInfo unit_info;
		while(GetUTF8Unit(strInput, stPos, &unit_info))
		{
			if(m_bIgnoreCase)
				ToLower(unit_info.strUnit);

			vecUnit.push_back(unit_info);
		}
	}

	list<std::pair<size_t, size_t>> /* <<StartPos, EndPos>> */ lstBadPhrase; //we mark each bad phrase's start/end position
	for(size_t i=0; i<vecUnit.size(); i++)
	{
		T_UnitInfo &unit_info=vecUnit[i];
		size_t stRemainingUnitCount = vecUnit.size() - i;

		pair
		<
			PhraseContainer::index<IndexTag_FirstUnit_UnitCount>::type::const_iterator,
			PhraseContainer::index<IndexTag_FirstUnit_UnitCount>::type::const_iterator
		> pairRange =
		m_micPhrase.get<IndexTag_FirstUnit_UnitCount>().equal_range(unit_info.strUnit);

		for(PhraseContainer::index<IndexTag_FirstUnit_UnitCount>::type::const_iterator it=pairRange.first; it!=pairRange.second; it++)
		{
			if(it->vecUnit.size() > stRemainingUnitCount)
				continue;

			bool bAllMatch=true;
			for(size_t j=0; j<it->vecUnit.size(); j++)
			{
				if(it->vecUnit[j]!=vecUnit[i+j].strUnit)
				{
					bAllMatch=false;
					break;
				}
			}

			if(bAllMatch)
			{
				lstBadPhrase.push_back(std::make_pair(unit_info.stStartPos, vecUnit[i+it->vecUnit.size()-1].stEndPos));
				i+=it->vecUnit.size()-1;
				break;
			}
		}
	}

	if(lstBadPhrase.size() > 0)
	{
		stPos=0;
		string strResult;

		for(auto bad_info : lstBadPhrase)
		{
			strResult += strInput.substr(stPos, bad_info.first-stPos);
			strResult += string(GetUTF8CharacterCount(strInput.substr(bad_info.first, bad_info.second - bad_info.first + 1)), '*');
			stPos = bad_info.second+1;
		}

		strResult += strInput.substr(stPos, string::npos);

		return strResult;
	}
	else
	{
		return strInput;
	}
}

int CPhraseFilter::GetUTF8CharacterCount(const string &strInput)
{
	if(strInput.size()==0)
		return 0;

	int cnt=0;
	size_t stMaxPos=strInput.size()-1;

	for(size_t st=0; st<=stMaxPos; )
	{
		st += GetUTF8CharacterByteIndicator(strInput[st]);
		++cnt;

		if(st>stMaxPos)
			break;
	}

	return cnt;
}

void CPhraseFilter::GetUTF8CharacterCount(const string &strInput, int *piSingleByteCharacterCount, int *piMultiByteCharacterCount)
{
	if(piMultiByteCharacterCount)
		*piMultiByteCharacterCount=0;

	if(piSingleByteCharacterCount)
		*piSingleByteCharacterCount=0;

	if(strInput.size()==0)
		return;

	size_t stMaxPos=strInput.size()-1;

	for(size_t st=0; st<=stMaxPos; )
	{
		int iByteCnt=GetUTF8CharacterByteIndicator(strInput[st]);

		if(iByteCnt>1 && piMultiByteCharacterCount)
			++(*piMultiByteCharacterCount);
		else if(iByteCnt==1 && piSingleByteCharacterCount)
			++(*piSingleByteCharacterCount);
		else
			return;

		st += iByteCnt;

		if(st>stMaxPos)
			return;
	}
}

/*
	UTF8 encoding rule:
	0000 0000-0000 007F | 0XXXXXXX
	0000 0080-0000 07FF | 110XXXXX 10XXXXXX
	0000 0800-0000 FFFF | 1110XXXX 10XXXXXX 10XXXXXX
	0001 0000-0010 FFFF | 11110XXX 10XXXXXX 10XXXXXX 10XXXXXX
*/
inline int CPhraseFilter::GetUTF8CharacterByteIndicator(const unsigned char uc)
{
	if((uc & 0xF0) == 0xE0)
		return 3;
	else if((uc & 0x80) == 0x00)
		return 1;
	else if((uc & 0xF8) == 0xF0)
		return 4;
	else if((uc & 0xE0) == 0xC0)
		return 2;
	else
		return 1;
}

void CPhraseFilter::ToLower(string &strInput)
{
	for(auto &it : strInput)
		if(it>='A' && it<='Z')
			it+=32;
}

bool CPhraseFilter::GetUTF8Unit(const std::string &strInput, size_t &stIteratorPos, T_UnitInfo *pUnitInfo)
{
	if(pUnitInfo)
	{
		pUnitInfo->strUnit="";
		pUnitInfo->stStartPos=0;
		pUnitInfo->stEndPos=0;
	}

	if(strInput.size()==0)
		return false;

	size_t stMaxPos=strInput.size()-1;

	if(stIteratorPos>stMaxPos)
		return false;

    unsigned char uc=strInput[stIteratorPos];

	//leading no-meaning bytes will be skipped
	while
	(
		(GetUTF8CharacterByteIndicator(uc)==1) &&
		(! ((uc>='0' && uc<='9') || (uc>='a' && uc<='z') || (uc>='A' && uc<='Z')) )
	)
	{
		++stIteratorPos;

		if(stIteratorPos>stMaxPos)
			return false;

		uc=strInput[stIteratorPos];
	}

	/*
		after above step, the cursor is now point to a real character!
	*/

	if(pUnitInfo)
		pUnitInfo->stStartPos=stIteratorPos;

	size_t stStartPos=stIteratorPos;
	int iFirstCharacterByteLen=GetUTF8CharacterByteIndicator(strInput[stIteratorPos]);

	if(iFirstCharacterByteLen>1) //unit consist of a multi-byte character, such as Chinese character, like "你"
	{
		if(pUnitInfo)
			pUnitInfo->stEndPos = pUnitInfo->stStartPos + iFirstCharacterByteLen - 1;

		while(--iFirstCharacterByteLen)
		{
			++stIteratorPos;

			if(stIteratorPos>stMaxPos) //the multi-byte character terminated? not expect!
				return false;

			if((strInput[stIteratorPos] & 0xC0) == 0x80) //expect 10xxxxxx, perfect!
				continue;
			else //not expect :(, so next byte may go into mess, let's terminate the parse
				return false;
		}

		++stIteratorPos;

		if(pUnitInfo)
			pUnitInfo->strUnit=strInput.substr(stStartPos, stIteratorPos-stStartPos);

		return true;
	}
	else if(iFirstCharacterByteLen==1) //unit consist of single-byte WORD, such as English, like "Hello"
	{
		while(1)
		{
			if(pUnitInfo)
				pUnitInfo->stEndPos=stIteratorPos;

			++stIteratorPos;

			if(stIteratorPos>stMaxPos)
			{
				if(pUnitInfo)
					pUnitInfo->strUnit=strInput.substr(stStartPos, stIteratorPos-stStartPos);

				return true;
			}

			uc=strInput[stIteratorPos];

			if(GetUTF8CharacterByteIndicator(uc)>1)
			{
				if(pUnitInfo)
					pUnitInfo->strUnit=strInput.substr(stStartPos, stIteratorPos-stStartPos);

				return true;
			}

			if( ! ((uc>='0' && uc<='9') || (uc>='a' && uc<='z') || (uc>='A' && uc<='Z')) )
			{
				if(pUnitInfo)
					pUnitInfo->strUnit=strInput.substr(stStartPos, stIteratorPos-stStartPos);

				return true;
			}
		}
	}
	else
	{
		return false;
	}
}
