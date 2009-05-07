//===========================================================================
//                                                                          =
//   This program is free software; you can redistribute it and/or modify   =
//   it under the terms of the GNU General Public License as published by   =
//   the Free Software Foundation; either version 2 of the License, or      =
//   (at your option) any later version.                                    =
// ------------------------------------------------------------------------ =
//                  TTTTT    OOO    PPPP    EEEE    DDDD                    =
//                  T T T   O   O   P   P   E       D   D                   =
//                    T    O     O  PPPP    EEE     D    D                  =
//                    T     O   O   P       E       D   D                   =
//                    T      OOO    P       EEEEE   DDDD                    =
//                                                                          =
//   This file is a part of Toped project (C) 2001-2007 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Mon Aug 11 2003
//        $Author$
//    Description: Reader of Mentor Graphics Calibre drc errors files
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
//      Comments :
//===========================================================================

#if !defined(CALBR_READER_H_INCLUDED)
#define CALBR_READER_H_INCLUDED

#include <fstream>
#include <string>

namespace Calbr
{

struct coord
{
	long x, y;
};

struct edge
{
	long x1, y1, x2, y2;
};

typedef std::vector <Calbr::coord> CoordsVector;
typedef std::vector <Calbr::edge> EdgesVector;

class drcEdge
{
public:
	void				addCoord(long x1, long y1, long x2, long y2);
	EdgesVector*	coords() {return &_coords;};
	void				showError(void);
	static long		_precision;
private:
	EdgesVector		_coords;
};

class drcPolygon
{
public:
	void				addCoord(long x, long y);
	CoordsVector*	coords() {return &_coords;};
	void				showError(void);
	static long		_precision;
private:
	CoordsVector	_coords;

};

class drcRuleCheck
{
public:
	drcRuleCheck(const std::string &name);

	std::string ruleCheckName()	const {return _ruleCheckName;}
	std::string timeStamp()			const {return _timeStamp;}
	long			curResCount()		const {return  _curResCount;}
	long			origResCount()		const {return _origResCount;}
	std::vector <Calbr::drcPolygon>* polygons() {return &_polygons;};
	std::vector <Calbr::edge>* edges() {return &_edges;};
	void			setTimeStamp(const std::string &timeStamp);
	void			setCurResCount(int curResCount);
	void			setOrigResCount(int origResCount);
	void			addDescrString(const std::string & str);
	void			addPolygon(const Calbr::drcPolygon &poly);
	void			addEdge(const Calbr::edge &theEdge);
private:
	long			_curResCount; //current result count
	long			_origResCount;//original result count
	std::string _ruleCheckName;
	std::string _timeStamp;
	std::string _header;
	std::vector <std::string> _descrStrings;
	std::vector <Calbr::drcPolygon> _polygons;
	std::vector <Calbr::edge> _edges;
};

typedef std::vector <Calbr::drcRuleCheck*> RuleChecksVector;

class CalbrFile
{
public:
	CalbrFile(const std::string &fileName);
	~CalbrFile();

	void					ShowResults();
	RuleChecksVector* Results() {return &_RuleChecks;};
	bool					isOk(void)	{return _ok;}
private:
	FILE*          _calbrFile;
	std::string    _fileName;
	std::string    _cellName;
	long				_precision;

	std::ifstream	_inFile;
	bool				parse();
	RuleChecksVector _RuleChecks;
	bool				_ok;

};

wxString convert(int number, long precision);
}
#endif //CALBR_READER_H_INCLUDED