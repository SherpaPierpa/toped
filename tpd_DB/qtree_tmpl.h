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
//        Created: Tue Mar 15 2011
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Database structure in the memory - clipping template
//                 (spin off from quadtree.h)
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================


#ifndef QTREE_TMPL_H_
#define QTREE_TMPL_H_

#include "quadtree.h"

namespace laydata {
   //==============================================================================
      /*! QuadTree class implements the main clipping algorithm of toped. Its main
         purpose is to speed-up the drawing of the database. All objects of type
         DataT or its derivatives - means all layout data - are stored into the
         objects of QuadTree type. Each object of QuadTree class is responsible for a
         rectangular area defined by the _overlap field. This area is dynamic and
         updated with every added, moved or deleted layout object.\n Each QuadTree
         object might be a parent of maximum four child objects of the same QuadTree
         class, so that each of the children is responsible for one out of the
         possible four sub-rectangles. Every layout object is fitted during the
         construction into the smallest possible QuadTree object. The children are
         created dynamically when a new object is about to fit into one of the four
         possible sub-rectangles of the _overlap area. Child QuadTree objects are
         stored into an quads[4] array and each of them is responsible by convention
         for the NW(north-west), NE(north-east), SE(south-east) and SW(south-west)
         sub-rectangles of the _overlap box.\n
         The methods can be split on several groups:
            - add a single layout object - add(), fitInTree()
            - add a group of layout objects - put(), sort(), fitSubTree()
            - object selection - selectInBox(), unselectInBox(), selectFromList(),
              selectAll()
            - design modification - deleteMarked()
            - tree maintenance - validate(), fullValidate(), sort(), resort(),
              tmpStore()
               */
   template <typename DataT>
   class QTreeTmpl {
   public:
      friend class Iterator<DataT>;
      friend class ClipIterator<DataT>;
      friend class DrawIterator<DataT>;
      friend class QTreeTmp;
      typedef  std::list<DataT*>                     TObjList;
      typedef  std::pair<DataT*, SGBitSet>           TObjDataPair;
      typedef  std::list<TObjDataPair>               TObjDataPairList;
      typedef laydata::Iterator<DataT>               Iterator;
      typedef laydata::ClipIterator<DataT>           ClipIterator;
      typedef laydata::DrawIterator<DataT>           DrawIterator;

                           QTreeTmpl();
                          ~QTreeTmpl();
      const Iterator       begin();
      const ClipIterator   begin(const DBbox&);
      const DrawIterator   begin(const layprop::DrawProperties&, const CtmQueue&);
      const Iterator       end();
      void                 openGlDraw(layprop::DrawProperties&, const TObjDataPairList*, bool) const;
      void                 openGlRender(tenderer::TopRend&, const TObjDataPairList*) const;
//      void                 visible_shapes(laydata::ShapeList*, const DBbox&, const CTM&, const CTM&, unsigned long&);
      short                clipType(tenderer::TopRend&) const;
      void                 add(DataT* shape);
      void                 selectFromList(TObjDataPairList*, TObjDataPairList*);
      bool                 deleteMarked(SH_STATUS stat=sh_selected, bool partselect=false);
      bool                 deleteThis(DataT*);
/*      DataT*               getfirstover(const TP);
      DataT*               getnextover(const TP, laydata::DataT*, bool& check);*/
      bool                 getObjectOver(const TP pnt, DataT*& prev);
      void                 validate();
      bool                 fullValidate();
      void                 resort(DataT* newdata = NULL);
      void                 resort(TObjList&);
      bool                 empty() const;
      void                 freeMemory();
      /*! Return the overlapping box*/
      DBbox                overlap() const   {return _overlap;}
      /*! Mark the tree as invalid*/
      void                 invalidate();
      /*! Return the status of _invalid flag*/
      bool                 invalid() const;
   private:
      void                 sort(TObjList&);
      bool                 fitInTree(DataT* shape);
      char                 fitSubTree(const DBbox&, DBbox*);
      void                 tmpStore(TObjList& store);
      byte                 biggest(int8b* array) const;
      void                 updateOverlap(const DBbox& hovl);
      byte                 sequreQuad(QuadIdentificators);
      void                 removeQuad(QuadIdentificators);
      DBbox                _overlap;//! The overlapping box of the quad
      /*! A pointers to four child QuadTree structures*/
      QTreeTmpl**          _subQuads;
      /*! Pointer to the first DataT stored in this QuadTree*/
      DataT**              _data;
      QuadProps            _props;
   };
}

#endif /* QTREE_TMPL_H_ */
