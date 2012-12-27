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
//   This file is a part of Toped project (C) 2001-2009 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Wed Sep 12 BST 2012
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Base class for all openGL renderers
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include <sstream>
#include "tenderer.h"
#include "viewprop.h"
#include "trend.h"

GLUtriangulatorObj*  TessellPoly::tenderTesel = NULL;
extern trend::TrendCenter*            TRENDC;


//=============================================================================
//
TeselChunk::TeselChunk(const TeselVertices& data, GLenum type, unsigned offset)
{
   _size = data.size();
   _index_seq = DEBUG_NEW unsigned[_size];
   word li = 0;
   for(TeselVertices::const_iterator CVX = data.begin(); CVX != data.end(); CVX++)
      _index_seq[li++] = *CVX + offset;
   _type = type;
}

TeselChunk::TeselChunk(const TeselChunk* data, unsigned offset)
{
   _size = data->size();
   _type = data->type();
   _index_seq = DEBUG_NEW unsigned[_size];
   const unsigned* copy_seq = data->index_seq();
   for(unsigned i = 0; i < _size; i++)
      _index_seq[i] = copy_seq[i] + offset;
}

TeselChunk::TeselChunk(const int* data, unsigned size, unsigned offset)
{ // used for wire tesselation explicitly
   _size = size;
   _type = GL_QUAD_STRIP;
   assert(0 ==(size % 2));
   _index_seq = DEBUG_NEW unsigned[_size];
   word findex = 0;     // forward  index
   word bindex = _size; // backward index
   for (word i = 0; i < _size / 2; i++)
   {
      _index_seq[2*i  ] = (findex++) + offset;
      _index_seq[2*i+1] = (--bindex) + offset;
   }
}

TeselChunk::TeselChunk(const TeselChunk& tcobj)
{
   _size = tcobj._size;
   _type = tcobj._type;
   _index_seq = DEBUG_NEW unsigned[_size];
   memcpy(_index_seq, tcobj._index_seq, sizeof(unsigned) * _size);
}

TeselChunk::~TeselChunk()
{
   delete [] _index_seq;
}
//=============================================================================
//

TeselTempData::TeselTempData(unsigned offset) :
   _the_chain   (    NULL),
   _ctype       (      0 ),
   _cindexes    (        ),
   _all_ftrs    (      0 ),
   _all_ftfs    (      0 ),
   _all_ftss    (      0 ),
   _offset      ( offset )
{}

TeselTempData::TeselTempData(TeselChain* tc) :
   _the_chain   ( tc     ),
   _ctype       (      0 ),
   _cindexes    (        ),
   _all_ftrs    (      0 ),
   _all_ftfs    (      0 ),
   _all_ftss    (      0 ),
   _offset      (      0 )
{}

void TeselTempData::storeChunk()
{
   _the_chain->push_back(TeselChunk(_cindexes, _ctype, _offset));
   switch (_ctype)
   {
      case GL_TRIANGLE_FAN   : _all_ftfs++; break;
      case GL_TRIANGLE_STRIP : _all_ftss++; break;
      case GL_TRIANGLES      : _all_ftrs++; break;
      default: assert(0);break;
   }
}

//=============================================================================
// TessellPoly

TessellPoly::TessellPoly() : _tdata(), _all_ftrs(0), _all_ftfs(0), _all_ftss(0)
{
}

void TessellPoly::tessellate(const int4b* pdata, unsigned psize)
{
   _tdata.clear();
   TeselTempData ttdata( &_tdata );
   // Start tessellation
   gluTessBeginPolygon(tenderTesel, &ttdata);
   GLdouble pv[3];
   pv[2] = 0;
   word* index_arr = DEBUG_NEW word[psize];
   for (unsigned i = 0; i < psize; i++ )
   {
      pv[0] = pdata[2*i]; pv[1] = pdata[2*i+1];
      index_arr[i] = i;
      gluTessVertex(tenderTesel,pv, &(index_arr[i]));
   }
   gluTessEndPolygon(tenderTesel);
   delete [] index_arr;
   _all_ftrs = ttdata.num_ftrs();
   _all_ftfs = ttdata.num_ftfs();
   _all_ftss = ttdata.num_ftss();
}


GLvoid TessellPoly::teselBegin(GLenum type, GLvoid* ttmp)
{
   TeselTempData* ptmp = static_cast<TeselTempData*>(ttmp);
   ptmp->newChunk(type);
}

GLvoid TessellPoly::teselVertex(GLvoid *pindex, GLvoid* ttmp)
{
   TeselTempData* ptmp = static_cast<TeselTempData*>(ttmp);
   ptmp->newIndex(*(static_cast<word*>(pindex)));
}

GLvoid TessellPoly::teselEnd(GLvoid* ttmp)
{
   TeselTempData* ptmp = static_cast<TeselTempData*>(ttmp);
   ptmp->storeChunk();
}

void TessellPoly::num_indexs(unsigned& iftrs, unsigned& iftfs, unsigned& iftss) const
{
   for (TeselChain::const_iterator CCH = _tdata.begin(); CCH != _tdata.end(); CCH++)
   {
      switch (CCH->type())
      {
         case GL_TRIANGLE_FAN   : iftfs += CCH->size(); break;
         case GL_TRIANGLE_STRIP : iftss += CCH->size(); break;
         case GL_TRIANGLES      : iftrs += CCH->size(); break;
         default: assert(0);break;
      }
   }
}

//=============================================================================
//
// TrendCnvx
//
unsigned trend::TrendCnvx::cDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
   assert(_csize);
#ifdef TENDERER_USE_FLOATS
   for (unsigned i = 0; i < 2 * _csize; i++)
      array[pindex+i] = (TNDR_GLDATAT) _cdata[i];
#else
   memcpy(&(array[pindex]), _cdata, 2 * sizeof(TNDR_GLDATAT) * _csize);
#endif
   pindex += 2 * _csize;
   return _csize;
}

void trend::TrendCnvx::drctDrawContour()
{
   glBegin(GL_LINE_LOOP);
   for (unsigned i = 0; i < _csize; i++)
      glVertex2i(_cdata[2*i], _cdata[2*i+1]);
   glEnd();
}

void trend::TrendCnvx::drctDrawFill()
{
   assert(false);//How did we get here?
}

//=============================================================================
//
// TrendBox
//
unsigned  trend::TrendBox::cDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
   assert(_csize);
   array[pindex++] = (TNDR_GLDATAT)_cdata[0];array[pindex++] = (TNDR_GLDATAT)_cdata[1];
   array[pindex++] = (TNDR_GLDATAT)_cdata[2];array[pindex++] = (TNDR_GLDATAT)_cdata[1];
   array[pindex++] = (TNDR_GLDATAT)_cdata[2];array[pindex++] = (TNDR_GLDATAT)_cdata[3];
   array[pindex++] = (TNDR_GLDATAT)_cdata[0];array[pindex++] = (TNDR_GLDATAT)_cdata[3];
   return _csize;
}

void trend::TrendBox::drctDrawContour()
{
   glBegin(GL_LINE_LOOP);
      glVertex2i(_cdata[0], _cdata[1]);
      glVertex2i(_cdata[2], _cdata[1]);
      glVertex2i(_cdata[2], _cdata[3]);
      glVertex2i(_cdata[0], _cdata[3]);
   glEnd();
}

void trend::TrendBox::drctDrawFill()
{
   glBegin(GL_POLYGON);
      glVertex2i(_cdata[0], _cdata[1]);
      glVertex2i(_cdata[2], _cdata[1]);
      glVertex2i(_cdata[2], _cdata[3]);
      glVertex2i(_cdata[0], _cdata[3]);
   glEnd();
}

//=============================================================================
//
// TrendTBox
//
trend::TrendTBox::TrendTBox(const TP& p1, const CTM& rmm) :
   TrendBox(NULL)
{
   TP p2 = p1 * rmm;
   int4b* contData = DEBUG_NEW int4b[4];
   contData[0] = p1.x();
   contData[1] = p1.y();
   contData[2] = p2.x();
   contData[3] = p2.y();
   _cdata = contData;
}

trend::TrendTBox::~TrendTBox()
{
   assert(_cdata);
   delete [] (_cdata);
}

//=============================================================================
//
// TrendNcvx
//
void trend::TrendNcvx::drctDrawFill()
{
   for ( TeselChain::const_iterator CCH = _tdata->tdata()->begin(); CCH != _tdata->tdata()->end(); CCH++ )
   {
      glBegin(CCH->type());
      for(unsigned cindx = 0 ; cindx < CCH->size(); cindx++)
      {
         unsigned vindex = CCH->index_seq()[cindx];
         glVertex2i(_cdata[2*vindex], _cdata[2*vindex+1]);
      }
      glEnd();
   }
}

//=============================================================================
//
// TrendTNcvx
//
trend::TrendTNcvx::TrendTNcvx(const PointVector& plist, const CTM& rmm) :
   TrendNcvx( NULL, plist.size()+1)
{
   dword numpnts = plist.size();
   int4b* contData = DEBUG_NEW int4b[2*(numpnts+1)];
   for (word i = 0; i < numpnts; i++)
   {
      contData[2*i  ] = plist[i].x();
      contData[2*i+1] = plist[i].y();
   }
   TP newp(plist[numpnts-1] * rmm);
   contData[2*numpnts  ] = newp.x();
   contData[2*numpnts+1] = newp.y();
   _cdata = contData;
}

trend::TrendTNcvx::~TrendTNcvx()
{
   assert(_cdata);
   delete [] (_cdata);
}

//=============================================================================
//
// TrendWire
//
trend::TrendWire::TrendWire(int4b* pdata, unsigned psize, WireWidth width, bool clo) :
   TrendNcvx (NULL, 0),
   _ldata    (pdata  ),
   _lsize    (psize  ),
   _celno    (clo    ),
   _tdata    (NULL   )
{
   if (!_celno)
   {
      laydata::WireContour wcontour(_ldata, _lsize, width);
      _csize = wcontour.csize();
      // this intermediate variable is just to deal with the const-ness of _cdata;
      int4b* contData = DEBUG_NEW int4b[ 2 * _csize];
      wcontour.getArrayData(contData);
      _cdata = contData;
   }
}

trend::TrendWire::TrendWire(unsigned psize, const WireWidth width, bool clo) :
   TrendNcvx (NULL, 0),
   _ldata    (NULL   ),
   _lsize    (psize  ),
   _celno    (clo    ),
   _tdata    (NULL   )
{
}

unsigned trend::TrendWire::lDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
   assert(_lsize);
#ifdef TENDERER_USE_FLOATS
   for (unsigned i = 0; i < 2 * _lsize; i++)
      array[pindex+i] = (TNDR_GLDATAT) _ldata[i];
#else
   memcpy(&(array[pindex]), _ldata, 2 * sizeof(TNDR_GLDATAT) * _lsize);
#endif
   pindex += 2 * _lsize;
   return _lsize;
}

void trend::TrendWire::drctDrawCLine()
{
   glBegin(GL_LINE_STRIP);
   for (unsigned i = 0; i < _lsize; i++)
      glVertex2i(_ldata[2*i], _ldata[2*i+1]);
   glEnd();
}

void trend::TrendWire::drctDrawFill()
{
   for ( TeselChain::const_iterator TCH = _tdata->begin(); TCH != _tdata->end(); TCH++ )
   {
      glBegin(TCH->type());
      for(unsigned cindx = 0 ; cindx < TCH->size(); cindx++)
      {
         unsigned vindex = TCH->index_seq()[cindx];
         glVertex2i(_cdata[2*vindex], _cdata[2*vindex+1]);
      }
      glEnd();
   }
}

/** For wire tessellation we can use the common polygon tessellation procedure.
    This could be a huge overhead though. The thing is that we've
    already been trough the precalc procedure and we know that wire object is
    very specific non-convex polygon. Using this knowledge the tessallation
    is getting really trivial. All we have to do is to list the contour points
    indexes in pairs - one from the front, and the other from the back of the
    array. Then this can be drawn as GL_QUAD_STRIP
*/
void trend::TrendWire::Tesselate()
{
   _tdata = DEBUG_NEW TeselChain();
   _tdata->push_back( TeselChunk(_cdata, _csize, 0));
}

trend::TrendWire::~TrendWire()
{
   if (NULL != _cdata) delete [] _cdata;
   if (NULL != _tdata) delete _tdata;
}

//=============================================================================
//
// TrendTWire
//
trend::TrendTWire::TrendTWire(const PointVector& plist, WireWidth width, bool clo, const CTM& rmm) :
   TrendWire(plist.size()+1, width, clo)
{
   dword num_points = plist.size();
   assert(0 < num_points);
   laydata::WireContourAux wcontour(plist, width, TP(plist[num_points-1] * rmm));

   int4b* centData = DEBUG_NEW int4b[ 2 * _lsize];
   wcontour.getArrayLData(centData);
   _ldata = centData;

   _csize = wcontour.csize();
   int4b* contData = DEBUG_NEW int4b[ 2 * _csize];
   wcontour.getArrayCData(contData);
   _cdata = contData;
}

trend::TrendTWire::~TrendTWire()
{
   delete [] _ldata;
}
//=============================================================================
//
// TextSOvlBox
//
unsigned trend::TextSOvlBox::cDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
   _offset = pindex/2;
   return TextOvlBox::cDataCopy(array, pindex);
}

unsigned trend::TextSOvlBox::sDataCopy(unsigned* array, unsigned& pindex)
{
   assert (NULL == _slist);
   for (unsigned i = 0; i < 4; i++)
      array[pindex++] = _offset + i;
   return ssize();
}

void trend::TextSOvlBox::drctDrawSlctd()
{
   //TODO
}
//=============================================================================
//
// TrendSCnvx
//
unsigned trend::TrendSCnvx::ssize()
{
   if (NULL == _slist) return _csize;
   // get the number of selected segments first - don't forget that here
   // we're using GL_LINE_STRIP which means that we're counting selected
   // line segments and for each segment we're going to store two indexes
   unsigned ssegs = 0;
   word  ssize = _slist->size();
   for (word i = 0; i < _csize; i++)
      if (_slist->check(i) && _slist->check((i+1)% ssize )) ssegs +=2;
   return ssegs;
}

unsigned trend::TrendSCnvx::cDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
   _offset = pindex/2;
   return TrendCnvx::cDataCopy(array, pindex);
}

unsigned trend::TrendSCnvx::sDataCopy(unsigned* array, unsigned& pindex)
{
   if (NULL != _slist)
   { // shape is partially selected
      // copy the indexes of the selected segment points
      for (unsigned i = 0; i < _csize; i++)
      {
         if (_slist->check(i) && _slist->check((i+1)%_csize))
         {
            array[pindex++] = _offset + i;
            array[pindex++] = _offset + ((i+1)%_csize);
         }
      }
   }
   else
   {
      for (unsigned i = 0; i < _csize; i++)
         array[pindex++] = _offset + i;
   }
   return ssize();
}

void trend::TrendSCnvx::drctDrawSlctd()
{// same as for non-convex polygon
   if (NULL == _slist)
   {
      glBegin(GL_LINE_LOOP);
      for (unsigned i = 0; i < _csize; i++)
         glVertex2i(_cdata[2*i], _cdata[2*i+1]);
      glEnd();
   }
   else
   {// shape is partially selected
      glBegin(GL_LINES);
      for (unsigned i = 0; i < _csize; i++)
      {
         if (_slist->check(i) && _slist->check((i+1)%_csize))
         {
            glVertex2i(_cdata[2*i], _cdata[2*i+1]);
            glVertex2i(_cdata[2*((i+1)%_csize)], _cdata[2*((i+1)%_csize)+1]);
         }
      }
      glEnd();
   }
}

//=============================================================================
//
// TrendSBox
//
unsigned trend::TrendSBox::ssize()
{
   if (NULL == _slist) return _csize;
   // get the number of selected segments first - don't forget that here
   // we're using GL_LINE_STRIP which means that we're counting selected
   // line segments and for each segment we're going to store two indexes
   unsigned ssegs = 0;
   word  ssize = _slist->size();
   for (word i = 0; i < _csize; i++)
      if (_slist->check(i) && _slist->check((i+1)% ssize )) ssegs +=2;
   return ssegs;
}

unsigned trend::TrendSBox::cDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
   _offset = pindex/2;
   return TrendBox::cDataCopy(array, pindex);
}

unsigned trend::TrendSBox::sDataCopy(unsigned* array, unsigned& pindex)
{
   if (NULL != _slist)
   { // shape is partially selected
      // copy the indexes of the selected segment points
      for (unsigned i = 0; i < _csize; i++)
      {
         if (_slist->check(i) && _slist->check((i+1)%_csize))
         {
            array[pindex++] = _offset + i;
            array[pindex++] = _offset + ((i+1)%_csize);
         }
      }
   }
   else
   {
      for (unsigned i = 0; i < _csize; i++)
         array[pindex++] = _offset + i;
   }
   return ssize();
}

void trend::TrendSBox::drctDrawSlctd()
{
   if (NULL == _slist)
   {// shape is fully selected
      glBegin(GL_LINE_LOOP);
         glVertex2i(_cdata[0], _cdata[1]);
         glVertex2i(_cdata[2], _cdata[1]);
         glVertex2i(_cdata[2], _cdata[3]);
         glVertex2i(_cdata[0], _cdata[3]);
      glEnd();
   }
   else
   {// shape is partially selected
      glBegin(GL_LINES);
      for (unsigned i = 0; i < _csize; i++)
      {
         if (_slist->check(i) && _slist->check((i+1)%_csize))
         {
            switch(i)
            {
               case 0: glVertex2i(_cdata[0], _cdata[1]);glVertex2i(_cdata[2], _cdata[1]); break;
               case 1: glVertex2i(_cdata[2], _cdata[1]);glVertex2i(_cdata[2], _cdata[3]); break;
               case 2: glVertex2i(_cdata[2], _cdata[3]);glVertex2i(_cdata[0], _cdata[3]); break;
               case 3: glVertex2i(_cdata[0], _cdata[3]);glVertex2i(_cdata[0], _cdata[1]); break;
               default: assert(false); break;
            }
         }
      }
      glEnd();
   }
}

//=============================================================================
//
// TrendSNcvx
//
unsigned trend::TrendSNcvx::ssize()
{
   if (NULL == _slist) return _csize;
   // get the number of selected segments first - don't forget that here
   // we're using GL_LINE_STRIP which means that we're counting selected
   // line segments and for each segment we're going to store two indexes
   unsigned ssegs = 0;
   word  ssize = _slist->size();
   for (word i = 0; i < _csize; i++)
      if (_slist->check(i) && _slist->check((i+1)% ssize )) ssegs +=2;
   return ssegs;
}

unsigned trend::TrendSNcvx::cDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
   _offset = pindex/2;
   return TrendCnvx::cDataCopy(array, pindex);
}

unsigned trend::TrendSNcvx::sDataCopy(unsigned* array, unsigned& pindex)
{
   if (NULL != _slist)
   { // shape is partially selected
      // copy the indexes of the selected segment points
      for (unsigned i = 0; i < _csize; i++)
      {
         if (_slist->check(i) && _slist->check((i+1)%_csize))
         {
            array[pindex++] = _offset + i;
            array[pindex++] = _offset + ((i+1)%_csize);
         }
      }
   }
   else
   {
      for (unsigned i = 0; i < _csize; i++)
         array[pindex++] = _offset + i;
   }
   return ssize();
}

void trend::TrendSNcvx::drctDrawSlctd()
{// same as for convex polygons
   if (NULL == _slist)
   {
      glBegin(GL_LINE_LOOP);
      for (unsigned i = 0; i < _csize; i++)
         glVertex2i(_cdata[2*i], _cdata[2*i+1]);
      glEnd();
   }
   else
   {// shape is partially selected
      glBegin(GL_LINES);
      for (unsigned i = 0; i < _csize; i++)
      {
         if (_slist->check(i) && _slist->check((i+1)%_csize))
         {
            glVertex2i(_cdata[2*i], _cdata[2*i+1]);
            glVertex2i(_cdata[2*((i+1)%_csize)], _cdata[2*((i+1)%_csize)+1]);
         }
      }
      glEnd();
   }
}

//=============================================================================
//
// TrendSWire
//
unsigned trend::TrendSWire::cDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
   _offset = pindex/2;
   return TrendCnvx::cDataCopy(array, pindex);
}

unsigned trend::TrendSWire::lDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
   _loffset = pindex/2;
   return TrendWire::lDataCopy(array, pindex);
}

unsigned trend::TrendSWire::ssize()
{
   if (NULL == _slist) return _lsize;
   // get the number of selected segments first - don't forget that here
   // we're using GL_LINE_STRIP which means that we're counting selected
   // line segments and for each segment we're going to store two indexes
   unsigned ssegs = 0;
   word  ssize = _slist->size();
   for (word i = 0; i < _lsize - 1; i++)
      if (_slist->check(i) && _slist->check((i+1)% ssize )) ssegs +=2;
   // Don't forget the edge points. This tiny little special case was dictating
   // the rules when the selected objects were fitted into the whole TrendBase
   // class hierarchy.
   if (!_celno)
   {
      if (_slist->check(0)         ) ssegs +=2;
      if (_slist->check(_lsize-1)  ) ssegs +=2;
   }
   return ssegs;
}

unsigned trend::TrendSWire::sDataCopy(unsigned* array, unsigned& pindex)
{
   if (NULL != _slist)
   { // shape is partially selected
      // copy the indexes of the selected segment points
      for (unsigned i = 0; i < _lsize - 1; i++)
      {
         if (_slist->check(i) && _slist->check(i+1))
         {
            array[pindex++] = _loffset + i;
            array[pindex++] = _loffset + i + 1;
         }
      }
      if (!_celno)
      {
         // And the edge points!
         if (_slist->check(0)       ) // if first point is selected
         {
            array[pindex++] = _offset + (_csize/2) -1;
            array[pindex++] = _offset + (_csize/2);
         }
         if (_slist->check(_lsize-1))// if last point is selected
         {
            array[pindex++] = _offset;
            array[pindex++] = _offset + _csize -1;
         }
      }
   }
   else
   {
      for (unsigned i = 0; i < _lsize; i++)
         array[pindex++] = _loffset + i;
   }
   return ssize();
}

void trend::TrendSWire::drctDrawSlctd()
{
   if (NULL == _slist)
   {
      glBegin(GL_LINE_STRIP);
      for (unsigned i = 0; i < _lsize; i++)
         glVertex2i(_ldata[2*i], _ldata[2*i+1]);
      glEnd();
   }
   else
   {// shape is partially selected
      glBegin(GL_LINES);
      for (unsigned i = 0; i < _lsize - 1; i++)
      {
         if (_slist->check(i) && _slist->check(i+1))
         {
            glVertex2i(_ldata[2*i], _ldata[2*i+1]);
            glVertex2i(_ldata[2*(i+1)], _ldata[2*(i+1)+1]);
         }
      }
      if (!_celno)
      {
         // And the edge points!
         if (_slist->check(0)       ) // if first point is selected
         {
            glVertex2i(_cdata[_csize-2], _cdata[_csize-1]);
            glVertex2i(_cdata[_csize], _cdata[_csize+1]);
         }
         if (_slist->check(_lsize-1))// if last point is selected
         {
            glVertex2i(_cdata[0], _cdata[1]);
            glVertex2i(_cdata[(2*_csize)-2], _cdata[(2*_csize)-1]);
         }
      }
      glEnd();
   }
}

//=============================================================================
//
// class TextOvlBox
//
trend::TextOvlBox::TextOvlBox(const DBbox& obox, const CTM& ctm)
{
   // Don't get confused here! It's not a stupid code. Think about
   // boxes rotated to 45 deg for example and you'll see why
   // obox * ctm is not possible
   TP tp = TP(obox.p1().x(), obox.p1().y()) * ctm;
   _obox[0] = tp.x();_obox[1] = tp.y();
   tp = TP(obox.p2().x(), obox.p1().y()) * ctm;
   _obox[2] = tp.x();_obox[3] = tp.y();
   tp = TP(obox.p2().x(), obox.p2().y()) * ctm;
   _obox[4] = tp.x();_obox[5] = tp.y();
   tp = TP(obox.p1().x(), obox.p2().y()) * ctm;
   _obox[6] = tp.x();_obox[7] = tp.y();
}

unsigned trend::TextOvlBox::cDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
#ifdef TENDERER_USE_FLOATS
   for (unsigned i = 0; i <  8; i++)
      array[pindex+i] = (TNDR_GLDATAT) _obox[i];
#else
   memcpy(&(array[pindex]), _obox, sizeof(TNDR_GLDATAT) * 8);
#endif
   pindex += 8;
   return 4;
}

void trend::TextOvlBox::drctDrawContour()
{
   glBegin(GL_LINE_LOOP);
   for (unsigned i = 0; i < 4; i++)
      glVertex2i(_obox[2*i], _obox[2*i+1]);
   glEnd();
}


//=============================================================================
//
// class TrendSMBox
//

trend::TrendSMBox::TrendSMBox(const int4b* pdata, const SGBitSet* slist, const CTM& rmm) :
   TrendSBox(pdata, slist)
{
   //   PointVector* nshape = movePointsSelected(*_slist, _rmm->Reversed(), strans);
   //   pt1 = (*nshape)[0]*(*_rmm); pt2 = (*nshape)[2]*(*_rmm);
   // The lines above - just a reminder of what the line below is actually doing
   PointVector* nshape = movePointsSelected(*_slist, CTM(), rmm);
   int4b* mdata = DEBUG_NEW int4b[4];
   mdata[0] = (*nshape)[0].x();
   mdata[1] = (*nshape)[0].y();
   mdata[2] = (*nshape)[2].x();
   mdata[3] = (*nshape)[2].y();
   _cdata = mdata;
   nshape->clear(); delete nshape;
}

trend::TrendSMBox::~TrendSMBox()
{
   delete [] _cdata;
}

PointVector* trend::TrendSMBox::movePointsSelected(const SGBitSet& pset,
                                    const CTM&  movedM, const CTM& stableM) const {
  // convert box to polygon
   PointVector* mlist = DEBUG_NEW PointVector();
   mlist->push_back(TP(_cdata[p1x], _cdata[p1y]));
   mlist->push_back(TP(_cdata[p2x], _cdata[p1y]));
   mlist->push_back(TP(_cdata[p2x], _cdata[p2y]));
   mlist->push_back(TP(_cdata[p1x], _cdata[p2y]));

   word size = mlist->size();
   PSegment seg1,seg0;
   // Things to remember in this algo...
   // Each of the points in the initial mlist is recalculated in the seg1.crossP
   // method. This actually means that on pass 0 (i == 0), no points are
   // recalculated because seg0 at that moment is empty. On pass 1 (i == 1),
   // point mlist[1] is recalculated etc. The catch comes on the last pass
   // (i == size) when constructing the seg1, we need mlist[0] and mlist[1], but
   // mlist[1] has been already recalculated and multiplying it with CTM
   // matrix again has pretty funny effect.
   // That's why another condition is introduced -> if (i == size)
   for (unsigned i = 0; i <= size; i++) {
      if (i == size)
         if (pset.check(i%size) && pset.check((i+1) % size))
            seg1 = PSegment((*mlist)[(i  ) % size] * movedM,
                            (*mlist)[(i+1) % size]         );
         else
            seg1 = PSegment((*mlist)[(i  ) % size] * stableM,
                            (*mlist)[(i+1) % size]          );
      else
         if (pset.check(i%size) && pset.check((i+1) % size))
            seg1 = PSegment((*mlist)[(i  ) % size] * movedM,
                            (*mlist)[(i+1) % size] * movedM);
         else
            seg1 = PSegment((*mlist)[(i  ) % size] * stableM,
                            (*mlist)[(i+1) % size] * stableM);
      if (!seg0.empty()) {
         seg1.crossP(seg0,(*mlist)[i%size]);
      }
      seg0 = seg1;
   }
   return mlist;
}

//=============================================================================
//
// class TrendSMNcvx
//
trend::TrendSMNcvx::TrendSMNcvx(const int4b* pdata, unsigned psize, const SGBitSet* slist, const CTM& rmm) :
   TrendSNcvx(pdata, psize, slist)
{
   PointVector* nshape = movePointsSelected(*_slist, CTM(), rmm);
   int4b* mdata = DEBUG_NEW int4b[2*_csize];

   for (unsigned i = 0; i < _csize; i++)
   {
      mdata[2*i  ] = (*nshape)[i].x();
      mdata[2*i+1] = (*nshape)[i].y();
   }
   _cdata = mdata;
   nshape->clear(); delete nshape;
}

trend::TrendSMNcvx::~TrendSMNcvx()
{
   delete [] _cdata;
}

PointVector* trend::TrendSMNcvx::movePointsSelected(const SGBitSet& pset,
                                    const CTM&  movedM, const CTM& stableM) const {
   PointVector* mlist = DEBUG_NEW PointVector();
   mlist->reserve(_csize);
   for (unsigned i = 0 ; i < _csize; i++ )
      mlist->push_back(TP(_cdata[2*i], _cdata[2*i+1]));

   PSegment seg1,seg0;
   // See the note about this algo in TdtBox::movePointsSelected above
   for (unsigned i = 0; i <= _csize; i++) {
      if (i == _csize)
         if (pset.check(i % _csize) && pset.check((i+1) % _csize))
            seg1 = PSegment((*mlist)[(i  ) % _csize] * movedM,
                            (*mlist)[(i+1) % _csize]         );
         else
            seg1 = PSegment((*mlist)[(i  ) % _csize] * stableM,
                            (*mlist)[(i+1) % _csize]         );
      else
         if (pset.check(i % _csize) && pset.check((i+1) % _csize))
            seg1 = PSegment((*mlist)[(i  ) % _csize] * movedM,
                            (*mlist)[(i+1) % _csize] * movedM);
         else
            seg1 = PSegment((*mlist)[(i  ) % _csize] * stableM,
                            (*mlist)[(i+1) % _csize] * stableM);
      if (!seg0.empty()) {
         seg1.crossP(seg0,(*mlist)[ i % _csize]);
      }
      seg0 = seg1;
   }
   return mlist;
}

//=============================================================================
//
// class TrendSMWire
//
trend::TrendSMWire::TrendSMWire(int4b* pdata, unsigned psize, const WireWidth width, bool clo, const SGBitSet* slist, const CTM& rmm) :
   TrendSWire(psize, width, clo, slist)
{
   _ldata = pdata;
   PointVector* nshape = movePointsSelected(*_slist, CTM(), rmm);
   int4b* mdata = DEBUG_NEW int4b[2*_lsize];

   for (unsigned i = 0; i < _lsize; i++)
   {
      mdata[2*i  ] = (*nshape)[i].x();
      mdata[2*i+1] = (*nshape)[i].y();
   }
   _ldata = mdata;
   nshape->clear(); delete nshape;

   if (!_celno)
   {
      laydata::WireContour wcontour(_ldata, _lsize, width);
      _csize = wcontour.csize();
      int4b* contData = DEBUG_NEW int4b[ 2 * _csize];
      wcontour.getArrayData(contData);
      _cdata = contData;
   }
}

trend::TrendSMWire::~TrendSMWire()
{
   delete [] _ldata;
}

PointVector* trend::TrendSMWire::movePointsSelected(const SGBitSet& pset,
                                    const CTM& movedM, const CTM& stableM) const
{
   PointVector* mlist = DEBUG_NEW PointVector();
   mlist->reserve(_lsize);
   for (unsigned i = 0 ; i < _lsize; i++ )
      mlist->push_back(TP(_ldata[2*i], _ldata[2*i+1]));

   PSegment* seg1 = NULL;
   PSegment* seg0 = NULL;
   for (unsigned i = 0; i < _lsize; i++)
   {
      if ((_lsize-1) == i)
      {
         if (pset.check(_lsize-1))
            seg1 = seg1->ortho((*mlist)[_lsize-1] * movedM);
         else
            seg1 = seg1->ortho((*mlist)[_lsize-1] * stableM);
      }
      else
      {
         const CTM& transM = ((pset.check(i) && pset.check(i+1))) ?
                                                               movedM : stableM;
         seg1 = DEBUG_NEW PSegment((*mlist)[(i  )] * transM, (*mlist)[(i+1)] * transM);
         if (0 == i)
         {
            if (pset.check(0))
               seg0 = seg1->ortho((*mlist)[i] * movedM);
            else
               seg0 = seg1->ortho((*mlist)[i] * stableM);
         }
      }
      if (!seg0->empty()) seg1->crossP(*seg0,(*mlist)[i]);
      if (NULL != seg0) delete seg0;
      seg0 = seg1;
   }
   if (NULL != seg0) delete seg0;
   return mlist;
}

//=============================================================================
//
// class TrendRef
//
trend::TrendRef::TrendRef(std::string name, const CTM& ctm, const DBbox& obox,
                   word alphaDepth) :
   _name          ( name         ),
   _ctm           ( ctm          ),
   _alphaDepth    ( alphaDepth   )
{
   _ctm.oglForm(_translation);
   TP tp = TP(obox.p1().x(), obox.p1().y()) * _ctm;
   _obox[0] = tp.x();_obox[1] = tp.y();
   tp = TP(obox.p2().x(), obox.p1().y()) * _ctm;
   _obox[2] = tp.x();_obox[3] = tp.y();
   tp = TP(obox.p2().x(), obox.p2().y()) * _ctm;
   _obox[4] = tp.x();_obox[5] = tp.y();
   tp = TP(obox.p1().x(), obox.p2().y()) * _ctm;
   _obox[6] = tp.x();_obox[7] = tp.y();
}


trend::TrendRef::TrendRef() :
   _name       ( ""   ),
   _ctm        (      ),
   _alphaDepth ( 0    )
{
   _ctm.oglForm(_translation);
   for (word i = 0; i < 8; _obox[i++] = 0);
}

unsigned trend::TrendRef::cDataCopy(TNDR_GLDATAT* array, unsigned& pindex)
{
#ifdef TENDERER_USE_FLOATS
   for (unsigned i = 0; i <  8; i++)
      array[pindex+i] = (TNDR_GLDATAT) _obox[i];
#else
   memcpy(&(array[pindex]), _obox, sizeof(TNDR_GLDATAT) * 8);
#endif
   pindex += 8;
   return 4;
}

void trend::TrendRef::drctDrawContour()
{
   glBegin(GL_LINE_LOOP);
   for (unsigned i = 0; i < 4; i++)
      glVertex2i(_obox[2*i], _obox[2*i+1]);
   glEnd();
}

//=============================================================================
//
// class TrendText
//
trend::TrendText::TrendText(const std::string* text, const CTM& ctm) : 
   _text ( text   ),
   _ctm  ( ctm    )
{
   //ctm.oglForm(_ftm);
}

void trend::TrendText::draw(bool fill, layprop::DrawProperties* drawprop)
{
   //glPushMatrix();
   //glMultMatrixd(_ftm);
   //glScalef(OPENGL_FONT_UNIT, OPENGL_FONT_UNIT, 1);
   TRENDC->drawString(*_text, fill, drawprop);
   //glPopMatrix();
}

//=============================================================================
//
// class TrendTV
//
trend::TrendTV::TrendTV(TrendRef* const refCell, bool filled, bool reusable,
                   unsigned parray_offset, unsigned iarray_offset) :
   _refCell             ( refCell         ),
   _num_total_strings   ( 0u              ),
   _filled              ( filled          ),
   _reusable            ( reusable        )
{
   for (int i = fqss; i <= ftss; i++)
   {
      _alobjix[i] = 0u;
      _alindxs[i] = 0u;
   }
   for (int i = cont; i <= ncvx; i++)
   {
      _alobjvx[i] = 0u;
      _alvrtxs[i] = 0u;
   }
}

void trend::TrendTV::registerBox (TrendCnvx* cobj)
{
   unsigned allpoints = cobj->csize();
   if (_filled)
   {
      _cnvx_data.push_back(cobj);
      _alvrtxs[cnvx] += allpoints;
      _alobjvx[cnvx]++;
   }
   else
   {
      _cont_data.push_back(cobj);
      _alvrtxs[cont] += allpoints;
      _alobjvx[cont]++;
   }
}

void trend::TrendTV::registerPoly (TrendNcvx* cobj, const TessellPoly* tchain)
{
   unsigned allpoints = cobj->csize();
   if (_filled && tchain && tchain->valid())
   {
      cobj->setTeselData(tchain);
      _ncvx_data.push_back(cobj);
      _alvrtxs[ncvx] += allpoints;
      _alobjix[ftrs] += tchain->num_ftrs();
      _alobjix[ftfs] += tchain->num_ftfs();
      _alobjix[ftss] += tchain->num_ftss();
      tchain->num_indexs(_alindxs[ftrs], _alindxs[ftfs], _alindxs[ftss]);
      _alobjvx[ncvx]++;
   }
   else
   {
      _cont_data.push_back(cobj);
      _alvrtxs[cont] += allpoints;
      _alobjvx[cont]++;
   }
}

void trend::TrendTV::registerWire (TrendWire* cobj)
{
   unsigned allpoints = cobj->csize();
   _line_data.push_back(cobj);
   _alvrtxs[line] += cobj->lsize();
   _alobjvx[line]++;
   if ( !cobj->center_line_only() )
   {
       if (_filled)
       {
         cobj->Tesselate();
         _ncvx_data.push_back(cobj);
         _alvrtxs[ncvx] += allpoints;
         _alindxs[fqss] += allpoints;
         _alobjvx[ncvx]++;
         _alobjix[fqss]++;
       }
       else
       {
          _cont_data.push_back(cobj);
          _alobjvx[cont] ++;
          _alvrtxs[cont] += allpoints;
       }
   }
}

void trend::TrendTV::registerText (TrendText* cobj, TextOvlBox* oobj)
{
   _text_data.push_back(cobj);
   _num_total_strings++;
   if (NULL != oobj)
   {
      _txto_data.push_back(oobj);
      _alvrtxs[cont] += 4;
      _alobjvx[cont]++;
   }
}

unsigned trend::TrendTV::num_total_points()
{
   return ( _alvrtxs[cont] +
            _alvrtxs[line] +
            _alvrtxs[cnvx] +
            _alvrtxs[ncvx]
          );
}

unsigned trend::TrendTV::num_total_indexs()
{
   return ( _alindxs[fqss] +
            _alindxs[ftrs] +
            _alindxs[ftfs] +
            _alindxs[ftss]
          );
}

trend::TrendRef* trend::TrendTV::swapRefCells(TrendRef* newRefCell)
{
   TrendRef* the_swap = _refCell;
   _refCell = newRefCell;
   return the_swap;
}

void trend::TrendTV::setAlpha(layprop::DrawProperties* drawprop)
{
   layprop::tellRGB tellColor;
   if (drawprop->getAlpha(_refCell->alphaDepth() - 1, tellColor))
   {
      glColor4ub(tellColor.red(), tellColor.green(), tellColor.blue(), tellColor.alpha());
   }
}

trend::TrendTV::~TrendTV()
{
   for (SliceWires::const_iterator CSO = _line_data.begin(); CSO != _line_data.end(); CSO++)
      if ((*CSO)->center_line_only()) delete (*CSO);
   for (SliceObjects::const_iterator CSO = _cnvx_data.begin(); CSO != _cnvx_data.end(); CSO++)
      delete (*CSO);
   for (SliceObjects::const_iterator CSO = _cont_data.begin(); CSO != _cont_data.end(); CSO++)
      delete (*CSO);
   for (SlicePolygons::const_iterator CSO = _ncvx_data.begin(); CSO != _ncvx_data.end(); CSO++)
      delete (*CSO);
   for (TrendStrings::const_iterator CSO = _text_data.begin(); CSO != _text_data.end(); CSO++)
      delete (*CSO);
   for (RefTxtList::const_iterator CSO = _txto_data.begin(); CSO != _txto_data.end(); CSO++)
      delete (*CSO);
   // Don't delete  _tmatrix. It's only a reference to it here
}

//=============================================================================
//
// class TrendLay
//
trend::TrendLay::TrendLay():
   _cslice               (        NULL ),
   _num_total_points     (          0u ),
   _num_total_indexs     (          0u ),
   _num_total_slctdx     (          0u ),
   _num_total_strings    (          0u )
{
   for (int i = lstr; i <= lnes; i++)
   {
      _asindxs[i] = 0u;
      _asobjix[i] = 0u;
   }
}

/** Add the current slice object (_cslice) to the list of slices _layData but
only if it's not empty. Also track the total number of vertexes in the layer
*/
void trend::TrendLay::ppSlice()
{
   if (NULL != _cslice)
   {
      unsigned num_points  = _cslice->num_total_points();
      unsigned num_strings = _cslice->num_total_strings();
      if ((num_points > 0) || (num_strings > 0))
      {
         _layData.push_back(_cslice);
         _num_total_points  += num_points;
         _num_total_strings += num_strings;
         _num_total_indexs  += _cslice->num_total_indexs();
         if (_cslice->reusable())
         {
            if (_cslice->filled())
            {
               assert(_reusableFData.end() == _reusableFData.find(_cslice->cellName()));
               _reusableFData[_cslice->cellName()] = _cslice;
            }
            else
            {
               assert(_reusableCData.end() == _reusableCData.find(_cslice->cellName()));
               _reusableCData[_cslice->cellName()] = _cslice;
            }
         }
      }
      else
         delete _cslice;
      _cslice = NULL;
   }
}

void trend::TrendLay::box  (const int4b* pdata)
{
   _cslice->registerBox(DEBUG_NEW TrendBox(pdata));
}

void trend::TrendLay::box  (const TP& p1, const CTM& rmm)
{
   _cslice->registerBox(DEBUG_NEW TrendTBox(p1, rmm));
}

void trend::TrendLay::box (const int4b* pdata, const SGBitSet* ss)
{
   TrendSBox* sobj = DEBUG_NEW TrendSBox(pdata, ss);
   registerSBox(sobj);
   _cslice->registerBox(sobj);
}

void trend::TrendLay::box (const int4b* pdata, const SGBitSet* ss, const CTM& rmm)
{
   TrendSBox* sobj = DEBUG_NEW TrendSMBox(pdata, ss, rmm);
   registerSBox(sobj);
   _cslice->registerBox(sobj);
}

void trend::TrendLay::poly (const int4b* pdata, unsigned psize, const TessellPoly* tpoly)
{
   _cslice->registerPoly(DEBUG_NEW TrendNcvx(pdata, psize), tpoly);
}

void trend::TrendLay::poly (const PointVector& pdata, const CTM& rmm)
{
   _cslice->registerPoly(DEBUG_NEW TrendTNcvx(pdata, rmm), NULL);
}

void trend::TrendLay::poly (const int4b* pdata, unsigned psize, const TessellPoly* tpoly, const SGBitSet* ss)
{
   TrendSNcvx* sobj = DEBUG_NEW TrendSNcvx(pdata, psize, ss);
   registerSPoly(sobj);
   _cslice->registerPoly(sobj, tpoly);
}

void trend::TrendLay::poly (const int4b* pdata, unsigned psize, const TessellPoly* tpoly, const SGBitSet* ss, const CTM& rmm)
{
   TrendSNcvx* sobj = DEBUG_NEW TrendSMNcvx(pdata, psize, ss, rmm);
   registerSPoly(sobj);
   _cslice->registerPoly(sobj, tpoly);
}

void trend::TrendLay::wire (int4b* pdata, unsigned psize, WireWidth width, bool center_only)
{
   _cslice->registerWire(DEBUG_NEW TrendWire(pdata, psize, width, center_only));
}

void trend::TrendLay::wire (const PointVector& pdata, WireWidth width, bool center_only, const CTM& rmm)
{
   _cslice->registerWire(DEBUG_NEW TrendTWire(pdata, width, center_only, rmm));

}

void trend::TrendLay::wire (int4b* pdata, unsigned psize, WireWidth width, bool center_only, const SGBitSet* ss)
{
   TrendSWire* sobj = DEBUG_NEW TrendSWire(pdata, psize, width, center_only, ss);
   registerSWire(sobj);
   _cslice->registerWire(sobj);
}

void trend::TrendLay::wire (int4b* pdata, unsigned psize, WireWidth width, bool center_only, const SGBitSet* ss, const CTM& rmm)
{
   TrendSWire* sobj = DEBUG_NEW TrendSMWire(pdata, psize, width, center_only, ss, rmm);
   registerSWire(sobj);
   _cslice->registerWire(sobj);
}

void trend::TrendLay::text (const std::string* txt, const CTM& ftmtrx, const DBbox* ovl, const TP& cor, bool sel)
{
   // Make sure that selected shapes don't come unexpected
   TextOvlBox* cobj = NULL;
   if (sel)
   {
      assert(ovl);
      TextSOvlBox* sobj = DEBUG_NEW TextSOvlBox((*ovl) , ftmtrx);
      registerSOBox(sobj);
      cobj = sobj;
   }
   else if (ovl)
   {
      cobj = DEBUG_NEW TextOvlBox((*ovl) , ftmtrx);
   }

   CTM ftm(ftmtrx.a(), ftmtrx.b(), ftmtrx.c(), ftmtrx.d(), 0, 0);
   ftm.Translate(cor * ftmtrx);
   _cslice->registerText(DEBUG_NEW TrendText(txt, ftm), cobj);
}


void trend::TrendLay::registerSBox (TrendSBox* sobj)
{
   _slct_data.push_back(sobj);
   if ( sobj->partSelected() )
   {
      _asindxs[lnes] += sobj->ssize();
      _asobjix[lnes]++;
   }
   else
   {
      _asindxs[llps] += sobj->csize();
      _asobjix[llps]++;
   }
}

void trend::TrendLay::registerSOBox (TextSOvlBox* sobj)
{
   _slct_data.push_back(sobj);
   _asindxs[llps] += 4;
   _asobjix[llps]++;
}


void trend::TrendLay::registerSPoly (TrendSNcvx* sobj)
{
   _slct_data.push_back(sobj);
   if ( sobj->partSelected() )
   {
      _asindxs[lnes] += sobj->ssize();
      _asobjix[lnes]++;
   }
   else
   {
      _asindxs[llps] += sobj->csize();
      _asobjix[llps]++;
   }
}

void trend::TrendLay::registerSWire (TrendSWire* sobj)
{
   _slct_data.push_back(sobj);
   if ( sobj->partSelected() )
   {
      _asindxs[lnes] += sobj->ssize();
      _asobjix[lnes]++;
   }
   else
   {
      _asindxs[lstr] += sobj->lsize();
      _asobjix[lstr]++;
   }
}

unsigned trend::TrendLay::total_slctdx()
{
   return ( _asindxs[lstr] +
            _asindxs[llps] +
            _asindxs[lnes]
          );
}

trend::TrendLay::~TrendLay()
{
   for (TrendTVList::const_iterator TLAY = _layData.begin(); TLAY != _layData.end(); TLAY++)
      delete (*TLAY);
   for (TrendReTVList::const_iterator TLAY = _reLayData.begin(); TLAY != _reLayData.end(); TLAY++)
      delete (*TLAY);
// not required?!
//   for (SliceSelected::const_iterator TLAY = _slct_data.begin(); TLAY != _slct_data.end(); TLAY++)
//      delete (*TLAY);
}


//=============================================================================
//
// class TrendRefLay
//
trend::TrendRefLay::TrendRefLay() :
   _alvrtxs    (      0u),
   _alobjvx    (      0u),
   _asindxs    (      0u),
   _asobjix    (      0u)
   {
}

void trend::TrendRefLay::addCellOBox(TrendRef* cRefBox, word alphaDepth, bool selected)
{
   if (selected)
   {
      _cellSRefBoxes.push_back(cRefBox);
//      assert(2 == alphaDepth); <-- @TODO (Alpha depth dependent rendering). Why this is hit during edit in place?
      _asindxs += 4;
      _asobjix++;
   }
   else
   {
      _cellRefBoxes.push_back(cRefBox);
      if (1 < alphaDepth)
      {
         // The meaning of this condition is to prevent rendering of the overlapping box of the
         // top visible cell. Only the top visible cell has alphaDepth parameter equals to 1
         _alvrtxs += 4;
         _alobjvx++;
      }
   }
}

unsigned trend::TrendRefLay::total_points()
{
   return (_alvrtxs + _asindxs);
}

unsigned trend::TrendRefLay::total_indexes()
{
   return (_alobjvx + _asobjix);
}

trend::TrendRefLay::~TrendRefLay()
{
   for (RefBoxList::const_iterator CSH = _cellRefBoxes.begin(); CSH != _cellRefBoxes.end(); CSH++)
      delete (*CSH);
   for (RefBoxList::const_iterator CSH = _cellSRefBoxes.begin(); CSH != _cellSRefBoxes.end(); CSH++)
      delete (*CSH);
}

//=============================================================================
//
// class TrendBase
//
trend::TrendBase::TrendBase( layprop::DrawProperties* drawprop, real UU ) :
   _drawprop           ( drawprop  ),
   _UU                 (        UU ),
   _clayer             (      NULL ),
   _grcLayer           (      NULL ),
   _refLayer           (      NULL ),
   _cslctd_array_offset(        0u ),
   _activeCS           (      NULL ),
   _dovCorrection      (         0 ),
   _rmm                (      NULL )
{
   // Initialize the cell (CTM) stack
   _cellStack.push(DEBUG_NEW TrendRef());
}

void trend::TrendBase::setRmm(const CTM& mm)
{
   _rmm = DEBUG_NEW CTM(mm.Reversed());
}

void trend::TrendBase::pushCell(std::string cname, const CTM& trans, const DBbox& overlap, bool active, bool selected)
{
   TrendRef* cRefBox = DEBUG_NEW TrendRef(cname,
                                          trans * _cellStack.top()->ctm(),
                                          overlap,
                                          _cellStack.size()
                                         );
   if (selected || (!_drawprop->isCellBoxHidden()))
      _refLayer->addCellOBox(cRefBox, _cellStack.size(), selected);
   else
      // This list is to keep track of the hidden cRefBox - so we can clean
      // them up. Don't get confused - we need cRefBox during the collecting
      // and drawing phase so we can't really delete them here or after they're
      // poped-up from _cellStack. The confusion is coming from the "duality"
      // of the TrendRef - once as a cell reference with CTM, view depth etc.
      // and then as a placeholder of the overlapping reference box
      _hiddenRefBoxes.push_back(cRefBox);

   _cellStack.push(cRefBox);
   if (active)
   {
      assert(NULL == _activeCS);
      _activeCS = cRefBox;
   }
}

void trend::TrendBase::grcpoly(int4b* pdata, unsigned psize)
{
   assert(_grcLayer);
   _grcLayer->poly(pdata, psize, NULL);
}

void trend::TrendBase::wire (int4b* pdata, unsigned psize, WireWidth width)
{
   // first check whether to draw only the center line
   DBbox wsquare = DBbox(TP(0,0),TP(width,width));
   bool center_line_only = !wsquare.visible(topCTM() * scrCTM(), visualLimit());
   _clayer->wire(pdata, psize, width, center_line_only);
}

void trend::TrendBase::wiret(const PointVector& pdata, WireWidth width)
{
   // first check whether to draw only the center line
   DBbox wsquare = DBbox(TP(0,0),TP(width,width));
   bool center_line_only = !wsquare.visible(topCTM() * scrCTM(), visualLimit());
   _clayer->wire(pdata, width, center_line_only, *_rmm);
}

void trend::TrendBase::wire (int4b* pdata, unsigned psize, WireWidth width, const SGBitSet* psel)
{
   // first check whether to draw only the center line
   DBbox wsquare = DBbox(TP(0,0),TP(width,width));
   bool center_line_only = !wsquare.visible(topCTM() * scrCTM(), visualLimit());
   _clayer->wire(pdata, psize, width, center_line_only,psel);
}

void trend::TrendBase::wirem(int4b* pdata, unsigned psize, WireWidth width, const SGBitSet* psel)
{
   // first check whether to draw only the center line
   DBbox wsquare = DBbox(TP(0,0),TP(width,width));
   bool center_line_only = !wsquare.visible(topCTM() * scrCTM(), visualLimit());
   _clayer->wire(pdata, psize, width, center_line_only,psel,*_rmm);
}

void trend::TrendBase::grcwire (int4b* pdata, unsigned psize, WireWidth width)
{
   // first check whether to draw only the center line
   DBbox wsquare = DBbox(TP(0,0),TP(width,width));
   bool center_line_only = !wsquare.visible(topCTM() * scrCTM(), visualLimit());
   _grcLayer->wire(pdata, psize, width, center_line_only);
}

void trend::TrendBase::arefOBox(std::string cname, const CTM& trans, const DBbox& overlap, bool selected)
{
   if (selected || (!_drawprop->isCellBoxHidden()))
   {
      TrendRef* cRefBox = DEBUG_NEW TrendRef(cname,
                                               trans * _cellStack.top()->ctm(),
                                               overlap,
                                               _cellStack.size()
                                              );
      _refLayer->addCellOBox(cRefBox, _cellStack.size(), selected);
   }
}

void trend::TrendBase::text (const std::string* txt, const CTM& ftmtrx, const DBbox& ovl, const TP& cor, bool sel)
{
   if (sel)
      _clayer->text(txt, ftmtrx, &ovl, cor, true);
   else if (_drawprop->isTextBoxHidden())
      _clayer->text(txt, ftmtrx, NULL, cor, false);
   else
      _clayer->text(txt, ftmtrx, &ovl, cor, false);
}

void trend::TrendBase::textt(const std::string* txt, const CTM& ftmtrx, const TP& cor)
{
   _clayer->text(txt, ftmtrx*(*_rmm), NULL, cor, false);
}

bool trend::TrendBase::preCheckCRS(const laydata::TdtCellRef* ref, layprop::CellRefChainType& crchain)
{
   crchain = _drawprop->preCheckCRS(ref);
   byte dovLimit = _drawprop->cellDepthView();
   if (0 == dovLimit) return true;
   switch (crchain)
   {
      case layprop::crc_VIEW:
         return (_cellStack.size() <= _drawprop->cellDepthView());
      case layprop::crc_POSTACTIVE:
         return ((_cellStack.size() - _dovCorrection) < _drawprop->cellDepthView());
      case layprop::crc_ACTIVE:
         _dovCorrection = _cellStack.size(); return true;
      default: return true;
   }
   return true;// Dummy statement - to prevent compiler warnings
}

void trend::TrendBase::cleanUp()
{
   for (DataLay::Iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
   {
      delete (*CLAY);
   }
   _data.clear();
   assert(1 == _cellStack.size());
   delete (_cellStack.top()); _cellStack.pop();
   for (RefBoxList::const_iterator CSH = _hiddenRefBoxes.begin(); CSH != _hiddenRefBoxes.end(); CSH++)
      delete (*CSH);
   _activeCS = NULL;
}

void trend::TrendBase::grcCleanUp()
{
   for (DataLay::Iterator CLAY = _grcData.begin(); CLAY != _grcData.end(); CLAY++)
   {
      delete (*CLAY);
   }
}

trend::TrendBase::~TrendBase()
{
   if (_refLayer) delete _refLayer;
   if (_rmm)      delete _rmm;
}

void trend::checkOGLError(std::string loc)
{
   std::ostringstream ost;
   GLenum ogle;
   while ((ogle=glGetError()) != GL_NO_ERROR)
   {
      ost << "OpenGL Error: \"" << gluErrorString(ogle)
          << "\" during " << loc;
      tell_log(console::MT_ERROR,ost.str());
   }
}
