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
/** \file
   Toped is a graphical application and naturally the graphic rendering is a primary
   objective in terms of quality and speed. The Toped rENDERER (TENDERER) is the first
   serious attempt in this project to utilize the power of todays graphic hardware via
   openGL library. The original rendering (which is preserved) is implementing the
   simplest possible rendering approach and is using the most basic openGL functions to
   achieve maximum compatibility. The goal of the TrendBase is to be a base for future
   updates in terms of graphic effects, 3D rendering, shaders etc.

   It must be clear that the main rendering acceleration is coming from the structure
   of the Toped database. The QuadTree dominates by far any other rendering optimisation
   especially on big databases where the speed really matters. This is the main reason
   behind the fact that the original renderer demonstrates comparable results with
   TrendBase. There are virtually no enhancements possible there though. It should be
   noted also that each rendering view in this context is unique. Indeed, when the user
   changes the visual window, the data stream from the Toped DB traverser can't be
   predicted. Different objects will be streamed out depending on the location and
   size of the visual window, but also depending on the current visual properties -
   colours, lines, fills, layer status, cell depth, visual details etc. The assumption
   though (and I hope it's a fact) is that the overall amount of the data is optimal
   with respect to the quality of the image.

   Both renderers are sinking data from the Toped QuadTree data base and depend heavily
   on its property to filter-out quickly and effectively all invisible objects. The
   Toped DB is generally a hierarchical cell tree. Each cell contains a linear list
   of layers which in turn contains a QuadTree of the layout objects. The main task
   of the TrendBase is to convert the data stream from the DB traversing into arrays
   of vertexes (VBOs) convenient for the graphical hardware.

   This is done in 3 steps:
      1. Traversing and sorting - data coming from the Toped DB traversing is sorted
         in the dedicated structures. Also some vital data statistics is done and some
         references gathered which will be used during the following steps
      2. VBO generation - the buffers are created and the sorted data is copied there.
      3. Drawing
   \verbatim
                     Layer 1                Layer 2                     Layer N
                   -------------         --------------              --------------
                  |   TrendLay   |      |  TrendLay    |            |  TrendLay    |
                  |              |      |              |            |              |
                  |  ----------  |      |  ----------  |            |  ----------  |
      cell_A      | | TrendTV  | |      | | TrendTV  | |     |      | | TrendTV  | |
                  |  ----------  |      |  ----------  |     |      |  ----------  |
                  |  ----------  |      |  ----------  |     |      |  ----------  |
      cell_B      | | TrendTV  | |      | | TrendTV  | |     |      | | TrendTV  | |
                  |  ----------  |      |  ----------  |     |      |  ----------  |
                  |    ______    |      |    ______    |     |      |    ______    |
                  |              |      |              |     |      |              |
                  |  ----------  |      |  ----------  |     |      |  ----------  |
      cell_Z      | | TrendTV  | |      | | TrendTV  | |     |      | | TrendTV  | |
                  |  ----------  |      |  ----------  |            |  ----------  |
                   --------------        --------------              --------------
                          v                     v                           v
                          v                     v                           v
                   -------------         --------------              --------------
                  |  ---------  |       |  ----------  |     |      |  ----------  |
                  | |point VBO| |       | | point VBO| |     |      | | point VBO| |
                  |  ---------  |       |  ----------  |            |  ----------  |
                  |  ---------  |       |  ----------  |     |      |  ----------  |
                  | |index VBO| |       | | index VBO| |     |      | | index VBO| |
                  |  ---------  |       |  ----------  |            |  ----------  |
                   -------------         --------------              --------------
                   ----------------------------------------------------------------
                  |                 index of selected objects VBO                  |
                   ----------------------------------------------------------------
   \endverbatim
   Speed:
   The first step is the most time consuming from all three, but this is mainly the
   Toped DB traversing which is assumed optimal. The last one is the quickest one
   and the speed there can be improved even more if shaders are used. The speed of
   the second step depends entirely on the implementation of the TrendBase.

   Memory usage:
   Only the first step consumes memory but it is minimised. There is no data copying
   instead data references are stored only. Technically the second step does consume
   memory of course, but this is the memory consumed by the graphic driver to map the
   VBO in the CPU memory. The TrendBase copies the data directly in the VBOs

   Note, that the flow described above allows the screen to be redrawn very quickly
   using the third step only. This opens the door to all kinds of graphical effects.

   As shown on the graph above the TrendBase ends-up with a number of VBOs for
   drawing. They are generated "per layer" basis. For each used layer there might
   be maximum two buffers:
   - point VBO - containing the vertexes of all objects on this layer across all
      cells
   - index VBO - containing the tesselation indexes of all non-convex polygons
      on this layer across all cells


   As a whole all vertex data is copied only once. Index data (polygon tesselation)
   is copied only once as well. Wire tesselation is done on the fly and data is
   stored directly in the index VBOs. Selected objects are indexed on the fly and
   the results are stored directly in the dedicated buffer. The index VBO
   containing the selected objects is common for all layers (as shown above)

*/
#ifndef BASETREND_H
#define BASETREND_H

#include <GL/glew.h>
#include "drawprop.h"

#define TENDERER_USE_FLOATS
#ifdef TENDERER_USE_FLOATS
   #define TNDR_GLDATAT GLfloat
   #define TNDR_GLENUMT GL_FLOAT
#else
   #define TNDR_GLDATAT GLint
   #define TNDR_GLENUMT GL_INT
#endif

// to cast properly the indices parameter in glDrawElements when
// drawing from VBO
#define VBO_BUFFER_OFFSET(i) ((char *)NULL + (i))

//=============================================================================
//
//
//
//=============================================================================
typedef std::list<word> TeselVertices;

class TeselChunk {
   public:
                        TeselChunk(const TeselVertices&, GLenum, unsigned);
                        TeselChunk(const TeselChunk*, unsigned);
                        TeselChunk(const int*, unsigned, unsigned);
                        TeselChunk(const TeselChunk&); //copy constructor
                       ~TeselChunk();
      GLenum            type() const      {return _type;}
      word              size() const      {return _size;}
      const unsigned*   index_seq() const {return _index_seq;}
   private:
      unsigned*         _index_seq;  // index sequence
      word              _size;       // size of the index sequence
      GLenum            _type;
};

typedef std::list<TeselChunk> TeselChain;

class TeselTempData {
   public:
                        TeselTempData(unsigned);
                        TeselTempData(TeselChain* tc);
      void              setChainP(TeselChain* tc)  {_the_chain = tc;}
      void              newChunk(GLenum type)      {_ctype = type; _cindexes.clear();}
      void              newIndex(word vx)          {_cindexes.push_back(vx);}
      void              storeChunk();
      word              num_ftrs()                 { return _all_ftrs;}
      word              num_ftfs()                 { return _all_ftfs;}
      word              num_ftss()                 { return _all_ftss;}
   private:
      TeselChain*       _the_chain;
      GLenum            _ctype;
      TeselVertices     _cindexes;
      word              _all_ftrs;
      word              _all_ftfs;
      word              _all_ftss;
      unsigned          _offset;
};

class TessellPoly {
   public:
                        TessellPoly();
      void              tessellate(const int4b* pdata, unsigned psize);
      const TeselChain* tdata() const              { return &_tdata;  }
      word              num_ftrs() const           { return _all_ftrs;}
      word              num_ftfs() const           { return _all_ftfs;}
      word              num_ftss() const           { return _all_ftss;}
      bool              valid() const              { return (0 < (_all_ftrs + _all_ftfs + _all_ftss));}
      void              num_indexs(unsigned&, unsigned&, unsigned&) const;
      static GLUtriangulatorObj* tenderTesel; //! A pointer to the OpenGL object tesselator
#ifdef WIN32
      static GLvoid CALLBACK teselVertex(GLvoid *, GLvoid *);
      static GLvoid CALLBACK teselBegin(GLenum, GLvoid *);
      static GLvoid CALLBACK teselEnd(GLvoid *);
#else
      static GLvoid     teselVertex(GLvoid *, GLvoid *);
      static GLvoid     teselBegin(GLenum, GLvoid *);
      static GLvoid     teselEnd(GLvoid *);
#endif
   private:
      TeselChain        _tdata;
      word              _all_ftrs;
      word              _all_ftfs;
      word              _all_ftss;
};

//=============================================================================
//
//
//
//=============================================================================
namespace trend {
   /**
   *  Text reference boxes
   */
   class TextOvlBox {
      public:
                           TextOvlBox(const DBbox&, const CTM&);
         virtual          ~TextOvlBox() {}
         virtual unsigned  cDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual void      drctDrawContour();
      private:
         int4b             _obox[8];
   };

   /**
   * Text objects
   */
   class TrendText {
      public:
                           TrendText(const std::string*, const CTM&);
         void              draw(bool);
      private:
         const std::string* _text;
         real              _ftm[16]; //! Font translation matrix
   };

   /**
      Represents convex polygons. Those are mainly the boxes from the data base.
      Stores a reference to the vertex array of the original object and its size.
      The only non-trivial method is cDataCopy which transfers the vertex data
      from the original object to a VBO mapped in the CPU memory
      This class is also a base for the Tender* class hierarchy.
   */
   class TrendCnvx {
      public:
                           TrendCnvx(const int4b* pdata, unsigned psize) :
                                    _cdata(pdata), _csize(psize){}
         virtual          ~TrendCnvx() {};
         virtual unsigned  cDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual void      drctDrawContour();
         virtual void      drctDrawFill();
         unsigned          csize()     {return _csize;}
      protected:
         const int4b*      _cdata;  //! the vertexes of the object contour
         unsigned          _csize;  //! the number of vertexes in _cdata
   };

   class TrendBox : public TrendCnvx {
      public:
                           TrendBox(const int4b* pdata) : TrendCnvx(pdata, 4) {}
         virtual unsigned  cDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual void      drctDrawContour();
         virtual void      drctDrawFill();
   };

   /**
      Represents non-convex polygons - most of the poly objects in the DB. Inherits
      TrendCnvx. The only addition is the tesselation data (_tdata) which is
      utilised if the object is to be filled.
   */
   class TrendNcvx : public TrendCnvx {
      public:
                           TrendNcvx(const int4b* pdata, unsigned psize) :
                                    TrendCnvx(pdata, psize), _tdata(NULL) {}
         virtual          ~TrendNcvx(){};
         void              setTeselData(const TessellPoly* tdata) {_tdata = tdata;}
         virtual void      drctDrawFill();
         virtual const TeselChain* tdata()              {return _tdata->tdata();}
      private:
         const TessellPoly*    _tdata; //! polygon tesselation data
   };

   /**
      Holds the wires from the data base. This class is not quite trivial and it
      causes all kinds of troubles in the overall class hierarchy. It inherits
      TrendNcvx (is it a good idea?). Theoretically in the general case this is a
      non-convex polygon. The wire as a DB object is very specific though. Only the
      central line is stored. The contour is calculated if required on the fly from
      the build-in methods called from the constructor. Instead of using general
      purpose tesselation (slow!), the tesselation data is extracted directly from
      the contour data virtually without calculations. This is the only class in the
      hierarchy which generates vertex and index data which means that it has to be
      properly cleaned-up.
   */
   class TrendWire : public TrendNcvx {
      public:
                           TrendWire(int4b*, unsigned, const WireWidth, bool);
         virtual          ~TrendWire();
         void              Tesselate();
         virtual unsigned  lDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual void      drctDrawFill();
         virtual void      drctDrawCLine();
         unsigned          lsize()                 {return _lsize;}
         bool              center_line_only()      {return _celno;}
         virtual const TeselChain* tdata()               {return _tdata;}
      protected:
         int4b*            _ldata; //! the vertexes of the wires central line
         unsigned          _lsize; //! the number of vertexes in the central line
         bool              _celno; //! indicates whether the center line only shall be drawn
         TeselChain*       _tdata; //! wire tesselation data
   };

   typedef enum {lstr, llps, lnes} SlctTypes;

   /**
      Very small pure virtual class which primary purpose is to minimize the
      memory usage of the TrendBase by reusing the vertex data. It also optimises
      the amount of data transfered to the graphic card as a whole.
      Here is the idea.

      In most of the rendering cases there will be no data selected in the
      active cell so this class will not be used at all. When a data is
      selected though, the graphical objects have to be rendered twice - once
      as normal objects and the second time - to highlight them in some way.
      There are several ways to achieve this:
         - to store the selected objects in a separate tree during traversing
         and sorting and then to render them separately. This however implies
         that the memory usage and data transfer to the GPU will be doubled
         for the selected objects.
         - to introduce additional selection related fields in all Tender classes
         and to alter the processing depending on the values of those fields.
         This will increase the overall memory usage of the TrendBase, because
         every graphical object will have more fields which will be unused
         actually most of the time. Besides it will (theoretically) slow down
         the processing, because will introduce conditional statements in all
         stages of the object processing.
         - The introduction of this class is an attempt to utilise the best of
         the ideas above, but without their drawbacks. A selected object with
         the corresponding selection related fields will be generated only on
         demand. The classes which deal with selected objects will inherit
         their respective parents and this class which will bring the
         additional selection related fields and methods. Selected objects
         will be rendered twice. Once - together with all other objects and
         then once again - to highlight them. Note though, that it will happen
         virtually without additional memory usage, because the second pass
         will reuse the data in their parent objects and will just index it.
         The amount of additional data to the GPU is also minimised,
         because the only additional data transfered is the index array of
         the selected vertexes. Vertex data is already there and will be reused.
   */
   class TrendSelected {
      public:
                           TrendSelected(const SGBitSet* slist) :
                              _slist(slist), _offset(0) {}
         virtual          ~TrendSelected() {}
         bool              partSelected() {return (NULL != _slist);}
         virtual SlctTypes type() = 0;
         virtual unsigned  ssize() = 0;
         virtual unsigned  sDataCopy(unsigned*, unsigned&) = 0;
         virtual void      drctDrawSlctd() = 0;
      protected:
         const SGBitSet*   _slist;  //! A bit set array with selected vertexes
         unsigned          _offset; //! The offset of the first vertex in the point VBO
   };

   class TextSOvlBox : public TextOvlBox, public TrendSelected {
      public:
                           TextSOvlBox(const DBbox& box, const CTM& mtrx) :
                              TextOvlBox(box, mtrx), TrendSelected(NULL) {}
         virtual          ~TextSOvlBox() {}
         virtual unsigned  cDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual SlctTypes type() { return llps;}
         virtual unsigned  ssize(){ return 4;}
         virtual unsigned  sDataCopy(unsigned*, unsigned&);
         virtual void      drctDrawSlctd();
   };


   /**
      Holds a selected or partially selected convex polygon. Its primary purpose is
      to generate the indexes for the contour (_cdata) data of its primary parent.
      Implements the virtual methods of its parents. Doesn't have any own fields or
      methods.
      The re-implementation of the cDataCopy() method inherited from the primary parent
      worths to be mentioned. It updates the inherited _offset field which is vital
      for the consequent indexing of the selected vertexes.
   */
   class TrendSCnvx : public TrendCnvx, public TrendSelected {
      public:
                           TrendSCnvx(int4b* pdata, unsigned psize, const SGBitSet* slist) :
                              TrendCnvx(pdata, psize), TrendSelected(slist) {}
         virtual unsigned  cDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual SlctTypes type() { return ((NULL == _slist) ? llps : lnes);}
         virtual unsigned  ssize();
         virtual unsigned  sDataCopy(unsigned*, unsigned&);
         virtual void      drctDrawSlctd();
   };

   class TrendSBox : public TrendBox, public TrendSelected {
      public:
                           TrendSBox(const int4b* pdata, const SGBitSet* slist) :
                              TrendBox(pdata), TrendSelected(slist) {}
         virtual unsigned  cDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual SlctTypes type() { return ((NULL == _slist) ? llps : lnes);}
         virtual unsigned  ssize();
         virtual unsigned  sDataCopy(unsigned*, unsigned&);
         virtual void      drctDrawSlctd();
   };

   /**
      Holds a selected or partially selected non-convex polygon. Its primary purpose
      is to generate the indexes for the contour (_cdata) data of its primary parent.
      Implements the virtual methods of its parents. Doesn't have any own fields or
      methods.
      The re-implementation of the cDataCopy() has the same function as described in
      TrendSCnvx class
   */
   class TrendSNcvx : public TrendNcvx, public TrendSelected  {
      public:
                           TrendSNcvx(const int4b* pdata, unsigned psize, const SGBitSet* slist) :
                              TrendNcvx(pdata, psize), TrendSelected(slist) {}
         virtual unsigned  cDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual SlctTypes type() { return ((NULL == _slist) ? llps : lnes);}
         virtual unsigned  ssize();
         virtual unsigned  sDataCopy(unsigned*, unsigned&);
         virtual void      drctDrawSlctd();
   };

   /**
      Holds a selected or partially selected wires. Like its primary parent this is
      not quite trivial class. Because of the specifics of the wire storage it has
      to store an additional VBO offset parameter for the central line. Note that
      unlike other selected objects, this one is indexing primary the central line
      data (_ldata) of its parent and in addition the contour data (_cdata) in
      some of the selection cases. The class implements all virtual methods of its
      parents.
      Here the re-implementation of two methods has to be highlighted. cDataCopy()
      updates inherited _offset field. lDataCopy in turn updates _loffset field.
      Both of them vital for proper indexing of the selected vertexes.
   */
   class TrendSWire : public TrendWire, public TrendSelected {
      public:
                           TrendSWire(int4b* pdata, unsigned psize, const WireWidth width, bool clo, const SGBitSet* slist) :
                              TrendWire(pdata, psize, width, clo), TrendSelected(slist), _loffset(0u) {}
         virtual unsigned  cDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual unsigned  lDataCopy(TNDR_GLDATAT*, unsigned&);
         virtual SlctTypes type() { return ((NULL == _slist) ? lstr : lnes);}
         virtual unsigned  ssize();
         virtual unsigned  sDataCopy(unsigned*, unsigned&);
         virtual void      drctDrawSlctd();
      private:
         unsigned          _loffset;
   };

   /**
   *  Cell reference boxes & reference related data
   */
   class TrendRef {
      public:
                           TrendRef(std::string, const CTM&, const DBbox&, word);
                           TrendRef();
         std::string       name()         {return _name;}
         real* const       translation()  {return _translation;}
         CTM&              ctm()          {return _ctm;}
         word              alphaDepth()   {return _alphaDepth;}
         unsigned          cDataCopy(TNDR_GLDATAT*, unsigned&);
         void              drctDrawContour();
      private:
         std::string       _name;
         real              _translation[16];
         CTM               _ctm;
         TNDR_GLDATAT      _obox[8];
         word              _alphaDepth;
   };

   typedef std::list<TrendCnvx*>      SliceObjects;
   typedef std::list<TrendNcvx*>      SlicePolygons;
   typedef std::list<TrendWire*>      SliceWires;
   typedef std::list<TrendSelected*>  SliceSelected;
   typedef std::list<TrendRef*>       RefBoxList;

   /**
      TENDERer Translation View - is the most fundamental class of the TrendBase.
      It sorts and stores a layer slice of the cell data. Most of the memory
      consumption during the first processing step (traversing and sorting) is
      concentrated in this class and naturally most of the TrendBase data processing
      happens in the methods of this class. The input data stream is sorted in 4
      "bins" in a form of Tender object lists. The corresponding objects are
      created by register* methods. Vertex or index data is not copied during this
      process, instead only data references are stored. The exception here are the
      wire objects which generate their contour data during the construction. Each
      object is sorted in exactly one bin depending whether it's going to be filled
      or not. The exception as always is the wire object which also goes to the
      line bin because of its central line.
      \verbatim
      ---------------------------------------------------------------
      | TrendBase |    not filled      |        filled      |        |
      |   data    |--------------------|--------------------|  enum  |
      | (vertexes)|  box | poly | wire |  box | poly | wire |        |
      |-----------|------|------|------|------|------|------|--------|
      | contour   |  x   |  x   |  x   |      |      |      |  cont  |
      | line      |      |      |  x   |      |      |  x   |  line  |
      | convex    |      |      |      |  x   |      |      |  cnvx  |
      | non-convex|      |      |      |      |  x   |  x   |  ncvx  |
      --------------------------------------------------------------
      \endverbatim

      The sorting step gathers also essential data statistics which will be used
      later to constitute the virtual buffers, to copy the data into the
      appropriate locations of the corresponding buffers and to draw the appropriate
      portions of the data using the appropriate openGL functions. This data is
      stored in the class fields in the form of four arrays with size of four each.
      One position per bin is allocated in each of those arrays (_alvrtxs[4],
      _alobjvx[4], _sizesvx[4], _firstvx[4]) indicating the overall number of
      vertexes, the total number of objects, the size of each object in terms of
      vertexes, and the offset of each first object vertex in the buffer

      If the non-convex data is to be filled, it is sorted further in another four
      index bins. The data statistics is also split further into four to accommodate
      that additional level of detail. This is necessary because the polygon
      tesselation generates 3 types of index data. In addition the wire tesselation
      is using yet another type of index data and all the above has to be sorted and
      stored. The array fields _alindxs[4], _alobjix[4], _sizesix[4], _firstix[4]
      hold a position for each of the four index bins. They represent the overall
      amount of indexes, the total number of indexed objects, the size of each
      object in terms of indexes and the offset of each first object index in the
      buffer.It has to be noted also that the openGL tesselation (used for
      non-convex polygons) produces a number of chunks of index data and each of
      them can be of any of the first 3 types. Wire tesselation produces a single
      chunk and it is always from one and the same type.  The table below summaries
      the indexing.

      \verbatim
      -----------------------------------------------------------------
      | Tesselation    | non convex object  |        |      openGL      |
      |         data   |--------------------|  enum  |     drawing      |
      |   (indexes)    |  polygon |  wire   |        |      method      |
      |----------------|----------|---------|--------|------------------|
      | triangles      |     x    |         |  ftrs  | GL_TRIANGLES     |
      | triangle fan   |     x    |         |  ftfs  | GL_TRIANGLE_FAN  |
      | triangle strip |     x    |         |  ftss  | GL_TRIANGLE_STRIP|
      | quadratic strip|          |    x    |  fqss  | GL_QUAD_STRIP    |
      -----------------------------------------------------------------
      \endverbatim

      The collect() method implements the second step of the processing namely
      data collection (copy). It puts all vertex data in the linear vertex buffer
      structured in the following way

      \verbatim
      -------------------------------------------------------------------------
      ... || cont | line  | cnvx | ncvx || cont | line  | cnvx | ncvx || ...
      ... ||----------------------------||----------------------------|| ...
      ... || this TrendTV object data   ||  next TrendTV object data  || ...
      -------------------------------------------------------------------------
      \endverbatim

      and the corresponding index data in another index buffer with the following
      structure

      \verbatim
      -------------------------------------------------------------------------
      ... || ftrs | ftfs  | ftss | fqss || ftrs | ftfs  | ftss | fqss || ...
      ... ||----------------------------||----------------------------|| ...
      ... ||  this TrendTV index data   ||  next TrendTV index data   || ...
      -------------------------------------------------------------------------
      \endverbatim

      The index data relates to the ncvx bin only and if the latter is not existing
      the corresponding portion of the index buffer will be with the length 0

      The actual drawing process refers to the ready VBOs which by this moment
      should be stored in the GPU memory. It then implements relatively simple
      algorithm calling the corresponding openGL functions for each portion of
      the VBOs using also as additional parameters the same additional data
      gathered during the traversing and sorting step.

      \verbatim
       -------------------------------------------------------------------
      |      |                                           |       VBO      |
      | data |            openGL function                |----------------|
      | type |                                           | vertex | index |
      |------|-------------------------------------------|--------|-------|
      | cont | glMultiDrawArrays(GL_LINE_LOOP  ...)      |    x   |       |
      |------|-------------------------------------------|--------|-------|
      | line | glMultiDrawArrays(GL_LINE_STRIP ...)      |    x   |       |
      |------|-------------------------------------------|--------|-------|
      | cnvx | glMultiDrawArrays(GL_LINE_LOOP  ...)      |    x   |       |
      |      | glMultiDrawArrays(GL_QUADS      ...)      |    x   |       |
      |------|-------------------------------------------|--------|-------|
      | ncvx | glMultiDrawArrays(GL_LINE_LOOP  ...)      |    x   |       |
      |      | glMultiDrawElements(GL_TRIANGLES     ...) |    x   |   x   |
      |      | glMultiDrawElements(GL_TRIANGLE_FAN  ...) |    x   |   x   |
      |      | glMultiDrawElements(GL_TRIANGLE_STRIP...) |    x   |   x   |
      |      | glMultiDrawElements(GL_QUAD_STRIP    ...) |    x   |   x   |
      -------------------------------------------------------------------
      \endverbatim
   */
   class TrendTV {
      public:
         enum {fqss, ftrs, ftfs, ftss} NcvxTypes;
         enum {cont, line, cnvx, ncvx} ObjtTypes;
         typedef std::list<TrendText*> TrendStrings;
         typedef std::list<TextOvlBox*> RefTxtList;
                           TrendTV(TrendRef* const, bool, bool, unsigned, unsigned);
         virtual          ~TrendTV();
         void              registerBox   (TrendCnvx*);
         void              registerPoly  (TrendNcvx*, const TessellPoly*);
         void              registerWire  (TrendWire*);
         void              registerText  (TrendText*, TextOvlBox*);

         virtual void      collect(TNDR_GLDATAT*, unsigned int*)  {assert(false);}
         virtual void      draw(layprop::DrawProperties*) = 0;
         virtual void      drawTexts(layprop::DrawProperties*) = 0;
         TrendRef*         swapRefCells(TrendRef*);

         unsigned          num_total_points();
         unsigned          num_total_indexs();
         unsigned          num_total_strings()  {return _num_total_strings;}
         bool              reusable() const     {return _reusable;}
         bool              filled() const       {return _filled;}
         std::string       cellName()           {return _refCell->name();}
      protected:
         TrendRef*        _refCell;
         // collected data lists
         SliceObjects      _cont_data; //! Contour data
         SliceWires        _line_data; //! Line data
         SliceObjects      _cnvx_data; //! Convex polygon data (Only boxes are here at the moment. TODO - all convex polygons)
         SlicePolygons     _ncvx_data; //! Non convex data
         TrendStrings      _text_data; //! Text (strings)
         RefTxtList        _txto_data; //! Text overlapping boxes
         // vertex related data
         unsigned          _alvrtxs[4]; //! array with the total number of vertexes
         unsigned          _alobjvx[4]; //! array with the total number of objects that will be drawn with vertex related functions
         // index related data for non-convex polygons
         unsigned          _alindxs[4]; //! array with the total number of indexes
         unsigned          _alobjix[4]; //! array with the total number of objects that will be drawn with index related functions
         //
         unsigned          _num_total_strings;
         bool              _filled;
         bool              _reusable;
   };

   /**
      Very small class which holds the reusable TrendTV chunks. Here is what it is.
      The view usually contains relatively small number of cells which are referenced
      multiply times. They do contain the same data, but it should be visualized
      using different translation matrices. The simplest case is an array of cells
      object. It would be really a waste of CPU time to traverse those cells every
      time they are referenced. This also means that the amount of processing data
      will be determined by the number of references, not by the number of unique
      cells which in turn will slow-down the rendering and will consume much more
      memory and bandwidth to transfer it to the GPU.

      Here is the alternative approach. Each cell layer (chunk) is checked during
      the traversing for its location. There can be 3 cases:

         - the chunk is outside the view window - it is then discarded and is not
         reaching the TrendBase at all.
         - the chunk is partially inside the view window - it is traversed, but
         its data is considered unique and no further attempts to reuse it.
         - the chunk is entirely inside the view window

      The latter case is the interesting one. Before creating a new chunk of data
      (TrendTV object), TrendBase is checking whether it already exists
      (TrendLay::chunkExists). If it does - it is registered here, a new TrendReTV
      object is created and the traversing of the current layer is skipped. If the
      chunk doesn't exists, then a TrendTV object is created, but it is also
      registered as "reusable" and the traversing continues as normal.

      This class requires only a draw() method which takes care to replace temporary
      the translation matrix of the referenced TrendTV and then calls TrendTV::draw()
   */
   class TrendReTV {
      public:
                           TrendReTV(TrendTV* const chunk, TrendRef* const refCell):
                              _chunk(chunk), _refCell(refCell) {}
         virtual          ~TrendReTV() {}
         virtual void      draw(layprop::DrawProperties*) = 0;
         virtual void      drawTexts(layprop::DrawProperties*) = 0;
      protected:
         TrendTV*  const   _chunk;
         TrendRef* const  _refCell;

   };

   /**
      Toper RENDering LAYer represents a DB layer in the rendering view. It
      generates and stores all the TrendTV objects (one per DB cell).

      A new TederTV object is created by the newSlice() method during the DB
      traversing. A reference to it is stored in the _layData list as well as
      in the _cslice field. All subsequent calls to box(), poly() and wire()
      methods create the corresponding object of class Trend* and store them
      calling the corresponding methods of _cslice object (of TrendTV class).
      If the data is selected then an object of the overloaded class is created
      and a reference to it is stored in _slct_data list as well.

      TrendLay keeps track of the total current amount of vertexes
      (_num_total_points) and indexes (_num_total_indexs) in order to initialise
      properly the offset fields of the generated TrendTV objects. When a new
      slice of class TrendTV is created, the current slice is post processed
      (method ppSlice()) in order to update the fields mentioned above. The
      final values stored in those fields are used by the child classes later
      to reserve eventually a proper amount of memory for the VBO's belonging
      to this layer.

      The rendering of the selected objects is done in this class. TrendTV does
      hold the object vertex data, but it is completely unaware of selection
      related properties. The primary reason for this is that data can be
      selected only in one of the generated TrendTV objects, because only one
      of them eventually belongs to the active cell. Having an additional VBO
      per TrendTV object for selected data would be a waste of course, because
      all but one of them will be empty anyway. Instead a single VBO is generated
      for the entire rendering view with the indexes of all selected vertexes in
      the active cell across all layers.

      Selected objects will be rendered twice. Once - together with all other
      objects in the TrendTV objects and again here, in this class in order to
      highlight them. An object can be fully or partially selected and depending
      on this it shall be rendered using the corresponding data structure:

      \verbatim
       --------------------------------------------------------------
      |  TrendBase  |  fully selected    | partially selected |        |
      |    data   |--------------------|--------------------|  enum  |
      | (indexes) |  box | poly | wire |  box | poly | wire |        |
      |-----------|------|------|------|------|------|------|--------|
      |line strips|      |      |      |   x  |   x  |   x  |  lstr  |
      | contours  |  x   |  x   |      |      |      |      |  llps  |
      |   lines   |      |      |  x   |      |      |      |  lnes  |
      --------------------------------------------------------------
      \endverbatim

      In the manner similar to TrendTV class, TrendLay sorts the selected objects
      in three "bins" and gathers some statistics about each of them. One position
      per bin is stored in each of the array fields (_asindxs[3], _asobjix[3],
      _sizslix[3], _fstslix[3]) representing the overall amount of indexes, the
      total number of indexed objects, the size of each object in terms of indexes
      and the offset of each first object index in the buffer. Additionally every
      selected object will need to store the offset of its vertex data within the
      slice representing the active cell in the vertex buffer. This is done by the
      selected objects themselves when vertex VBOs are collected via the
      re-implementation of cDataCopy virtual methods in the TenderS* classes.
      TrendLay doesn't take part in this process, TrendTV class is not aware of
      it either - it happens "naturally" thanks to the class hierarchy.

      The collectSelected() method implements the second step of the processing of
      selected data. It must be called after the collect() method, because the
      latter is the one that indirectly gathers the object vertex offsets described
      previously. collectSelected() puts all index data in the linear vertex buffer
      structured in the following way:

      \verbatim
      -------------------------------------------------------------------------
      ... ||   lstr  |   llps  |  lnes  ||   lstr  |   llps  |  lnes  || ...
      ... ||----------------------------||----------------------------|| ...
      ... ||  this TrendLay index data ||  next TrendLay index data || ...
      -------------------------------------------------------------------------
      \endverbatim

      The drawing of the corresponding portion of the selected data buffer is done
      also in this class by the drawSelected() method. For this we need the vertex
      buffer and a proper offset in it which will point to the beginning of the data
      slice belonging to the active cell. This offset is captured during the data
      traversing in the field _stv_array_offset. Having done that, the drawing is
      now trivial

      ----------------------------------------------------------------
      |      |                                        |       VBO      |
      | data |            openGL function             |----------------|
      | type |                                        | vertex | index |
      |------|----------------------------------------|--------|-------|
      | lstr | glMultiDrawElements(GL_LINE_STRIP ...) |    x   |   x   |
      |------|----------------------------------------|--------|-------|
      | llps | glMultiDrawElements(GL_LINE_LOOP  ...) |    x   |   x   |
      |------|----------------------------------------|--------|-------|
      | lnes | glMultiDrawElements(GL_LINES      ...) |    x   |   x   |
      ----------------------------------------------------------------
   */
   class TrendLay {
      public:
         typedef std::list<TrendTV*>     TrendTVList;
         typedef std::list<TrendReTV*>   TrendReTVList;
         typedef std::map<std::string, TrendTV*> ReusableTTVMap;

                           TrendLay();
         virtual          ~TrendLay();
         void              box  (const int4b*);
         void              box  (const int4b*,                               const SGBitSet*);
         void              poly (const int4b*, unsigned, const TessellPoly*);
         void              poly (const int4b*, unsigned, const TessellPoly*, const SGBitSet*);
         void              wire (int4b*, unsigned, WireWidth, bool);
         void              wire (int4b*, unsigned, WireWidth, bool, const SGBitSet*);
         void              text (const std::string*, const CTM&, const DBbox*, const TP&, bool);
         virtual void      newSlice(TrendRef* const, bool, bool /*, bool, unsigned*/) = 0;
         virtual void      newSlice(TrendRef* const, bool, bool, unsigned slctd_array_offset) = 0;
         virtual bool      chunkExists(TrendRef* const, bool) = 0;
         void              ppSlice();
         virtual void      draw(layprop::DrawProperties*) = 0;
         virtual void      drawSelected() = 0;
         virtual void      drawTexts(layprop::DrawProperties*) = 0;
         virtual void      collect(bool, GLuint, GLuint) { assert(false); }
         virtual void      collectSelected(unsigned int*) { assert(false); }
         unsigned          total_points() {return _num_total_points;}
         unsigned          total_indexs() {return _num_total_indexs;}
         unsigned          total_slctdx();
         unsigned          total_strings(){return _num_total_strings;}

      protected:
         void              registerSBox  (TrendSBox*);
         void              registerSPoly (TrendSNcvx*);
         void              registerSWire (TrendSWire*);
         void              registerSOBox (TextSOvlBox*);
         ReusableTTVMap    _reusableFData; // reusable filled chunks
         ReusableTTVMap    _reusableCData; // reusable contour chunks
         TrendTVList       _layData;
         TrendReTVList     _reLayData;
         TrendTV*          _cslice;    //!Working variable pointing to the current slice
         unsigned          _num_total_points;
         unsigned          _num_total_indexs;
         unsigned          _num_total_slctdx;
         unsigned          _num_total_strings;
         // Data related to selected objects
         SliceSelected     _slct_data;
         // index related data for selected objects
         unsigned          _asindxs[3]; //! array with the total number of indexes of selected objects
         unsigned          _asobjix[3]; //! array with the total number of selected objects
   };

   /**
      Responsible for visualising the overlap boxes of the references. Pure virtual and
      relatively trivial class. One object of this class should be created only. All
      reference boxes are processed in a single VBO. This includes the selected ones.
   */
   class TrendRefLay {
      public:
                           TrendRefLay();
         virtual          ~TrendRefLay();
         void              addCellOBox(TrendRef*, word, bool);
         virtual void      collect(GLuint)  { assert(false); }
         virtual void      draw(layprop::DrawProperties*) = 0;
         unsigned          total_points();
         unsigned          total_indexes();
      protected:
         RefBoxList        _cellRefBoxes;
         RefBoxList        _cellSRefBoxes;
         // vertex related data
         unsigned          _alvrtxs; //! total number of vertexes
         unsigned          _alobjvx; //! total number of objects that will be drawn with vertex related functions
         // index related data for selected boxes
         unsigned          _asindxs; //! total number of selected vertexes
         unsigned          _asobjix; //! total number of objects that will be drawn with index related functions
   };

   //-----------------------------------------------------------------------------
   //
   typedef laydata::LayerContainer<TrendLay*> DataLay;
   typedef std::stack<TrendRef*> CellStack;

   /**
      Toped RENDerer BASE is the front-end class, the interface to the rest of the
      world. Pure virtual. All data collection and render views are initiated using
      an object of this class. There should be only one object of this class at a
      time. The object of this class must be destroyed before the next data collection
      or rendering view is invoked. The data gathered from the previous view must be
      considered invalid. The object structure created by this class is shown in
      the documentation of the module.
   */
   class TrendBase {
      public:
                           TrendBase( layprop::DrawProperties* drawprop, real UU );
         virtual          ~TrendBase();
         virtual void      grid( const real, const std::string ) = 0;
         virtual void      zeroCross( ) = 0;
         virtual void      setLayer(const LayerDef&, bool) = 0;
         virtual void      setHvrLayer(const LayerDef&) = 0;
         virtual void      setGrcLayer(bool, const LayerDef&) = 0;
         virtual bool      chunkExists(const LayerDef&, bool) = 0;
         void              pushCell(std::string, const CTM&, const DBbox&, bool, bool);
         void              popCell()                              {_cellStack.pop();}
         const CTM&        topCTM() const                         {return  _cellStack.top()->ctm();}
         void              box  (const int4b* pdata)              {_clayer->box(pdata);}
         void              box  (const int4b* pdata, const SGBitSet* ss){_clayer->box(pdata, ss);}
         void              poly (const int4b* pdata, unsigned psize, const TessellPoly* tpoly)
                                                                  {_clayer->poly(pdata, psize, tpoly);}
         void              poly (const int4b* pdata, unsigned psize, const TessellPoly* tpoly, const SGBitSet* ss)
                                                                  {_clayer->poly(pdata, psize, tpoly, ss);}
         void              grcpoly(int4b* pdata, unsigned psize);
         void              wire (int4b*, unsigned, WireWidth);
         void              wire (int4b*, unsigned, WireWidth, const SGBitSet*);
         void              grcwire (int4b*, unsigned, WireWidth);
         void              arefOBox(std::string, const CTM&, const DBbox&, bool);
         void              text (const std::string*, const CTM&, const DBbox&, const TP&, bool);
         virtual bool      collect() = 0;
         virtual bool      grcCollect() = 0;
         virtual void      draw() = 0;
         virtual void      grcDraw() = 0;
         virtual void      cleanUp();
         virtual void      grcCleanUp();

         LayerDef          getTenderLay(const LayerDef& laydef)
                                                         {return _drawprop->getTenderLay(laydef)   ;}
         void              setState(layprop::PropertyState state)
                                                         {        _drawprop->setState(state)       ;}
         bool              layerHidden(const LayerDef& laydef) const
                                                         {return _drawprop->layerHidden(laydef)    ;}
         const CTM&        scrCTM() const                {return _drawprop->scrCtm()               ;}
         word              visualLimit() const           {return _drawprop->visualLimit()          ;}
         const DBbox&      clipRegion() const            {return _drawprop->clipRegion()           ;}
         void              postCheckCRS(const laydata::TdtCellRef* ref)
                                                         {        _drawprop->postCheckCRS(ref)     ;}
         bool              preCheckCRS(const laydata::TdtCellRef*, layprop::CellRefChainType&);
         void              initDrawRefStack(laydata::CellRefStack* crs)
                                                         {       _drawprop->initDrawRefStack(crs)  ;}
         void              clearDrawRefStack()           {       _drawprop->clearDrawRefStack()    ;}
         bool              adjustTextOrientation() const {return _drawprop->adjustTextOrientation();}
         layprop::DrawProperties*&   drawprop()          {return _drawprop                         ;}
      protected:
         layprop::DrawProperties*   _drawprop;
         real              _UU;
         DataLay           _data;            //!All editable data for drawing
         DataLay           _grcData;         //!All GRC      data for drawing
         TrendLay*         _clayer;          //!Working variable pointing to the current edit slice
         TrendLay*         _grcLayer;        //!Working variable pointing to the current GRC  slice
         TrendRefLay*      _refLayer;
         CellStack         _cellStack;       //!Required during data traversing stage
         unsigned          _cslctd_array_offset; //! Current selected array offset
         //
         TrendRef*         _activeCS;
         byte              _dovCorrection;   //! Cell ref Depth of view correction (for Edit in Place purposes)
         RefBoxList        _hiddenRefBoxes;  //! Those cRefBox objects which didn't ended in the TrendRefLay structures
   };

   void checkOGLError(std::string);
}


#endif //BASETREND_H
