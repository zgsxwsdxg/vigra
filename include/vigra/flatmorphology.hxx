/************************************************************************/
/*                                                                      */
/*               Copyright 1998-2000 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    You may use, modify, and distribute this software according       */
/*    to the terms stated in the LICENSE file included in               */
/*    the VIGRA distribution.                                           */
/*                                                                      */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de                              */
/*                                                                      */
/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */
/*                                                                      */
/************************************************************************/
 
 
#ifndef VIGRA_FLATMORPHOLOGY_HXX
#define VIGRA_FLATMORPHOLOGY_HXX

#include <cmath>
#include <vector>
#include "vigra/utilities.hxx"

namespace vigra {

/** erosion, dilation, and median with disc structuring functions
    @heading Basic Morphologic Operations
*/
//@{

/********************************************************/
/*                                                      */
/*                  discRankOrderFilter                 */
/*                                                      */
/********************************************************/

/** Apply rank order filter with disc structuring function to the image.
    The pixel values of the source image {\bf must} be in the range
    0...255. Radius must be >= 0. Rank must be in the range 0.0 <= rank 
    <= 1.0. The filter acts as a minimum filter if rank = 0.0, 
    as a median if rank = 0.5, and as a maximum filter if rank = 1.0.
    Accessor are used to access the pixel data.
    
    {\bf Declarations:}
    
    pass arguments explicitely:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class DestIterator, class DestAccessor>
        void
        discRankOrderFilter(SrcIterator upperleft1, 
			    SrcIterator lowerright1, SrcAccessor sa,
			    DestIterator upperleft2, DestAccessor da,
			    int radius, float rank)
    }
    \end{verbatim}
    
    
    use argument objects in conjuction with \Ref{Argument Object Factories}:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class DestIterator, class DestAccessor>
        void
        discRankOrderFilter(triple<SrcIterator, SrcIterator, SrcAccessor> src,
			    pair<DestIterator, DestAccessor> dest,
			    int radius, float rank)
    }
    \end{verbatim}
    
    {\bf Usage:}
    
        Include-File:
        \URL[vigra/flatmorphology.hxx]{../include/vigra/flatmorphology.hxx}\\
    Namespace: vigra
    
    \begin{verbatim}
    vigra::CImage src, dest;
    
    // do median filtering
    vigra::discRankOrderFilter(srcImageRange(src), destImage(dest), 10, 0.5);
    \end{verbatim}

    {\bf Required Interface:}
    
    \begin{verbatim}
    SrcIterator src_upperleft;
    DestIterator dest_upperleft;
    int x, y;
    unsigned char value;
    
    SrcAccessor src_accessor;
    DestAccessor dest_accessor;
    
    // value_type of accessor must be convertible to unsigned char
    value = src_accessor(src_upperleft, x, y); 
    
    dest_accessor.set(value, dest_upperleft, x, y);
    \end{verbatim}
    
    {\bf Preconditions:}
    
    \begin{verbatim}
    for all source pixels: 0 <= value <= 255
    
    (rank >= 0.0) && (rank <= 1.0)
    radius >= 0
    \end{verbatim}
    
    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void
discRankOrderFilter(SrcIterator upperleft1, 
                    SrcIterator lowerright1, SrcAccessor sa,
                    DestIterator upperleft2, DestAccessor da,
		    int radius, float rank)
{
    vigra_precondition((rank >= 0.0) && (rank <= 1.0),
            "discRankOrderFilter(): Rank must be between 0 and 1"
	    " (inclusive).");
    
    vigra_precondition(radius >= 0,
            "discRankOrderFilter(): Radius must be >= 0.");
    
    int i, x, y, xmax, ymax, xx, yy;
    int rankpos, winsize, leftsum;
    
    long hist[256];
    
    // prepare structuring function
    std::vector<int> struct_function(radius+1);
    struct_function[0] = radius;
    
    double r2 = (double)radius*radius;
    for(i=1; i<=radius; ++i)
    {
	double r = (double) i - 0.5;
	struct_function[i] = (int)(std::sqrt(r2 - r*r) + 0.5);
    }

    int w = lowerright1.x - upperleft1.x;
    int h = lowerright1.y - upperleft1.y;
    
    SrcIterator ys(upperleft1);
    DestIterator yd(upperleft2);

    for(y=0; y<h; ++y, ++ys.y, ++yd.y)
    {
	SrcIterator xs(ys);
	DestIterator xd(yd);
        
	// first column
	int x0 = 0;
	int y0 = y;
	int x1 = w - 1;
	int y1 = h - y - 1;

	// clear histogram
	for(i=0; i<256; ++i) hist[i] = 0;
	winsize = 0;
	
	// init histogram
	ymax = (y1 < radius) ? y1 : radius;
	for(yy=0; yy<=ymax; ++yy)
	{
	    xmax = (x1 < struct_function[yy]) ? x1 : struct_function[yy];
	    for(xx=0; xx<=xmax; ++xx)
	    {
		hist[sa(xs, Diff2D(xx, yy))]++;
		winsize++;
	    }
	}
	
	ymax = (y0 < radius) ? y0 : radius;
	for(yy=1; yy<=ymax; ++yy)
	{
	    xmax = (x1 < struct_function[yy]) ? x1 : struct_function[yy];
	    for(xx=0; xx<=xmax; ++xx)
	    {
		hist[sa(xs, Diff2D(xx, -yy))]++;
		winsize++;
	    }
	}
    
	// find the desired histogramm bin 
	leftsum = 0;
	if(rank == 0.0)
	{
	    for(i=0; i<256; i++)
	    {
		if(hist[i]) break;
	    }
	    rankpos = i;
	}
	else
	{
	    for(i=0; i<256; i++)
	    {
		if((float)(hist[i]+leftsum) / winsize >= rank) break;
		leftsum += hist[i];
	    }
	    rankpos = i;
	}
	
	da.set(rankpos, xd);
	
	++xs.x;
	++xd.x;
	
	// inner columns
	for(x=1; x<w; ++x, ++xs.x, ++xd.x)
	{
	    x0 = x;
	    y0 = y;
	    x1 = w - x - 1;
	    y1 = h - y - 1;
	    
	    // update histogramm 
	    // remove pixels at left border 
	    yy = (y1 < radius) ? y1 : radius;
	    for(; yy>=0; yy--)
	    {
		unsigned char cur;
		xx = struct_function[yy]+1;
		if(xx > x0) break;
		
		cur = sa(xs, Diff2D(-xx, yy));
		
		hist[cur]--;
		if(cur < rankpos) leftsum--;
		winsize--;
	    }
	    yy = (y0 < radius) ? y0 : radius;
	    for(; yy>=1; yy--)
	    {
		unsigned char cur;
		xx = struct_function[yy]+1;
		if(xx > x0) break;
		
		cur = sa(xs, Diff2D(-xx, -yy));
		
		hist[cur]--;
		if(cur < rankpos) leftsum--;
		winsize--;
	    }
	    
	    // add pixels at right border 
	    yy = (y1 < radius) ? y1 : radius;
	    for(; yy>=0; yy--)
	    {
		unsigned char cur;
		xx = struct_function[yy];
		if(xx > x1) break;
		
		cur = sa(xs, Diff2D(xx, yy));
		
		hist[cur]++;
		if(cur < rankpos) leftsum++;
		winsize++;
	    }
	    yy = (y0 < radius) ? y0 : radius;
	    for(; yy>=1; yy--)
	    {
		unsigned char cur;
		xx = struct_function[yy];
		if(xx > x1) break;
		
		cur = sa(xs, Diff2D(xx, -yy));
		
		hist[cur]++;
		if(cur < rankpos) leftsum++;
		winsize++;
	    }
	
	    // find the desired histogramm bin 
	    if(rank == 0.0)
	    {
		if(leftsum == 0)
		{
		    // search to the right 
		    for(i=rankpos; i<256; i++)
		    {
			if(hist[i]) break;
		    }
		    rankpos = i;
		}
		else
		{
		    // search to the left 
		    for(i=rankpos-1; i>=0; i--)
		    {
			leftsum -= hist[i];
			if(leftsum == 0) break;
		    }
		    rankpos = i;
		}
	    }
	    else  // rank > 0.0 
	    {
		if((float)leftsum / winsize < rank)
		{
		    // search to the right 
		    for(i=rankpos; i<256; i++)
		    {
			if((float)(hist[i]+leftsum) / winsize >= rank) break;
			leftsum+=hist[i];
		    }
		    rankpos = i;
		}
		else
		{
		    /// search to the left 
		    for(i=rankpos-1; i>=0; i--)
		    {
			leftsum-=hist[i];
			if((float)leftsum / winsize < rank) break;
		    }
		    rankpos = i;
		}
	    }
		    
	    da.set(rankpos, xd);
	}
    }
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void
discRankOrderFilter(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                    pair<DestIterator, DestAccessor> dest,
		    int radius, float rank)
{
    discRankOrderFilter(src.first, src.second, src.third,
                        dest.first, dest.second,
			radius, rank);
}

/********************************************************/
/*                                                      */
/*                      discErosion                     */
/*                                                      */
/********************************************************/

/** Apply erosion (minimum) filter with disc of given radius to image.
    This is an abbreviation vor the rank order filter with rank = 0.0.
    See \Ref{discRankOrderFilter}() for more information.
    
    {\bf Declarations:}
    
    pass arguments explicitely:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class DestIterator, class DestAccessor>
        inline void 
        discErosion(SrcIterator upperleft1, 
		    SrcIterator lowerright1, SrcAccessor sa,
		    DestIterator upperleft2, DestAccessor da,
		    int radius)
    }
    \end{verbatim}
    
    
    use argument objects in conjuction with \Ref{Argument Object Factories}:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class DestIterator, class DestAccessor>
        void
        discErosion(triple<SrcIterator, SrcIterator, SrcAccessor> src,
		    pair<DestIterator, DestAccessor> dest,
		    int radius)
    }
    \end{verbatim}

    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline void 
discErosion(SrcIterator upperleft1, 
            SrcIterator lowerright1, SrcAccessor sa,
            DestIterator upperleft2, DestAccessor da,
	    int radius)
{
    vigra_precondition(radius >= 0, "discErosion(): Radius must be >= 0.");
    
    discRankOrderFilter(upperleft1, lowerright1, sa, 
                        upperleft2, da, radius, 0.0);
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void
discErosion(triple<SrcIterator, SrcIterator, SrcAccessor> src,
            pair<DestIterator, DestAccessor> dest,
            int radius)
{
    vigra_precondition(radius >= 0, "discErosion(): Radius must be >= 0.");
    
    discRankOrderFilter(src.first, src.second, src.third,
                        dest.first, dest.second,
			radius, 0.0);
}

/********************************************************/
/*                                                      */
/*                     discDilation                     */
/*                                                      */
/********************************************************/

/** Apply dilation (maximum) filter with disc of given radius to image.
    This is an abbreviation vor the rank order filter with rank = 1.0.
    See \Ref{discRankOrderFilter}() for more information.
    
    {\bf Declarations:}
    
    pass arguments explicitely:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class DestIterator, class DestAccessor>
        inline void 
        discDilation(SrcIterator upperleft1, 
		    SrcIterator lowerright1, SrcAccessor sa,
		    DestIterator upperleft2, DestAccessor da,
		    int radius)
    }
    \end{verbatim}
    
    
    use argument objects in conjuction with \Ref{Argument Object Factories}:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class DestIterator, class DestAccessor>
        void
        discDilation(triple<SrcIterator, SrcIterator, SrcAccessor> src,
		    pair<DestIterator, DestAccessor> dest,
		    int radius)
    }
    \end{verbatim}

    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline void 
discDilation(SrcIterator upperleft1, 
            SrcIterator lowerright1, SrcAccessor sa,
            DestIterator upperleft2, DestAccessor da,
	    int radius)
{
    vigra_precondition(radius >= 0, "discDilation(): Radius must be >= 0.");
    
    discRankOrderFilter(upperleft1, lowerright1, sa, 
                        upperleft2, da, radius, 1.0);
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void
discDilation(triple<SrcIterator, SrcIterator, SrcAccessor> src,
            pair<DestIterator, DestAccessor> dest,
            int radius)
{
    vigra_precondition(radius >= 0, "discDilation(): Radius must be >= 0.");
    
    discRankOrderFilter(src.first, src.second, src.third,
                        dest.first, dest.second,
			radius, 1.0);
}

/********************************************************/
/*                                                      */
/*                      discMedian                      */
/*                                                      */
/********************************************************/

/** Apply median filter with disc of given radius to image.
    This is an abbreviation vor the rank order filter with rank = 0.5.
    See \Ref{discRankOrderFilter}() for more information.
    
    {\bf Declarations:}
    
    pass arguments explicitely:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class DestIterator, class DestAccessor>
        inline void 
        discMedian(SrcIterator upperleft1, 
		    SrcIterator lowerright1, SrcAccessor sa,
		    DestIterator upperleft2, DestAccessor da,
		    int radius)
    }
    \end{verbatim}
    
    
    use argument objects in conjuction with \Ref{Argument Object Factories}:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class DestIterator, class DestAccessor>
        void
        discMedian(triple<SrcIterator, SrcIterator, SrcAccessor> src,
		    pair<DestIterator, DestAccessor> dest,
		    int radius)
    }
    \end{verbatim}

    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline void 
discMedian(SrcIterator upperleft1, 
            SrcIterator lowerright1, SrcAccessor sa,
            DestIterator upperleft2, DestAccessor da,
	    int radius)
{
    vigra_precondition(radius >= 0, "discMedian(): Radius must be >= 0.");
    
    discRankOrderFilter(upperleft1, lowerright1, sa, 
                        upperleft2, da, radius, 0.5);
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void
discMedian(triple<SrcIterator, SrcIterator, SrcAccessor> src,
            pair<DestIterator, DestAccessor> dest,
            int radius)
{
    vigra_precondition(radius >= 0, "discMedian(): Radius must be >= 0.");
    
    discRankOrderFilter(src.first, src.second, src.third,
                        dest.first, dest.second,
			radius, 0.5);
}

/********************************************************/
/*                                                      */
/*            discRankOrderFilterWithMask               */
/*                                                      */
/********************************************************/

/** Apply rank order filter with disc structuring function to the image
    using a mask.
    The pixel values of the source image {\bf must} be in the range
    0...255. Radius must be >= 0. Rank must be in the range 0.0 <= rank 
    <= 1.0. The filter acts as a minimum filter if rank = 0.0, 
    as a median if rank = 0.5, and as a maximum filter if rank = 1.0.
    Accessor are used to access the pixel data.
    
    The mask is only applied to th input image, i.e. the function
    generates an output wherever the current disc contains at least 
    one pixel with mask value 'true'. Source pixels with mask value
    'false' are ignored during the calculation of the rank order.
    
    {\bf Declarations:}
    
    pass arguments explicitely:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class MaskIterator, class MaskAccessor,
	          class DestIterator, class DestAccessor>
        void
        discRankOrderFilterWithMask(SrcIterator upperleft1, 
				    SrcIterator lowerright1, SrcAccessor sa,
				    MaskIterator upperleftm, MaskAccessor mask,
				    DestIterator upperleft2, DestAccessor da,
				    int radius, float rank)
    }
    \end{verbatim}
    
    
    group arguments (use in conjuction with factory functions \Ref{srcRange}(),
    \Ref{mask}(), and \Ref{dest}()):
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class MaskIterator, class MaskAccessor,
	          class DestIterator, class DestAccessor>
        void
        discRankOrderFilterWithMask(triple<SrcIterator, SrcIterator, SrcAccessor> src,
				    pair<MaskIterator, MaskAccessor> mask,
				    pair<DestIterator, DestAccessor> dest,
				    int radius, float rank)
    }
    \end{verbatim}
    
    {\bf Usage:}
    
        Include-File:
        \URL[vigra/flatmorphology.hxx]{../include/vigra/flatmorphology.hxx}\\
    Namespace: vigra
    
    \begin{verbatim}
    vigra::CImage src, dest, mask;
    
    // do median filtering
    vigra::discRankOrderFilterWithMask(srcImageRange(src), 
                                maskImage(mask), destImage(dest), 10, 0.5);
    \end{verbatim}

    {\bf Required Interface:}
    
    \begin{verbatim}
    SrcIterator src_upperleft;
    DestIterator dest_upperleft;
    MaskIterator mask_upperleft;
    int x, y;
    unsigned char value;
    
    SrcAccessor src_accessor;
    DestAccessor dest_accessor;
    MaskAccessor mask_accessor;
		     
    mask_accessor(mask_upperleft, x, y) // convertible to bool
    
    // value_type of accessor must be convertible to unsigned char
    value = src_accessor(src_upperleft, x, y); 
    
    dest_accessor.set(value, dest_upperleft, x, y);
    \end{verbatim}
    
    {\bf Preconditions:}
    
    \begin{verbatim}
    for all source pixels: 0 <= value <= 255
    
    (rank >= 0.0) && (rank <= 1.0)
    radius >= 0
    \end{verbatim}
    
    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class MaskIterator, class MaskAccessor,
	  class DestIterator, class DestAccessor>
void
discRankOrderFilterWithMask(SrcIterator upperleft1, 
                            SrcIterator lowerright1, SrcAccessor sa,
                            MaskIterator upperleftm, MaskAccessor mask,
			    DestIterator upperleft2, DestAccessor da,
			    int radius, float rank)
{
    vigra_precondition((rank >= 0.0) && (rank <= 1.0),
                 "discRankOrderFilter(): Rank must be between 0 and 1"
		 " (inclusive).");
    
    vigra_precondition(radius >= 0, "discRankOrderFilter(): Radius must be >= 0.");
    
    int i, x, y, xmax, ymax, xx, yy;
    int rankpos, winsize, leftsum;
    
    long hist[256];
    
    // prepare structuring function
    std::vector<int> struct_function(radius+1);
    struct_function[0] = radius;
    
    double r2 = (double)radius*radius;
    for(i=1; i<=radius; ++i)
    {
	double r = (double) i - 0.5;
	struct_function[i] = (int)(std::sqrt(r2 - r*r) + 0.5);
    }

    int w = lowerright1.x - upperleft1.x;
    int h = lowerright1.y - upperleft1.y;
        
    SrcIterator ys(upperleft1);
    MaskIterator ym(upperleftm);
    DestIterator yd(upperleft2);

    for(y=0; y<h; ++y, ++ys.y, ++yd.y, ++ym.y)
    {
	SrcIterator xs(ys);
	MaskIterator xm(ym);
	DestIterator xd(yd);
        
	// first column
	int x0 = 0;
	int y0 = y;
	int x1 = w - 1;
	int y1 = h - y - 1;

	// clear histogram
	for(i=0; i<256; ++i) hist[i] = 0;
	winsize = 0;
	leftsum = 0;
	rankpos = 0;
	
	// init histogram
	ymax = (y1 < radius) ? y1 : radius;
	for(yy=0; yy<=ymax; ++yy)
	{
	    xmax = (x1 < struct_function[yy]) ? x1 : struct_function[yy];
	    for(xx=0; xx<=xmax; ++xx)
	    {
	        Diff2D pos(xx, yy);
                if(mask(xm, pos))
		{
		    hist[sa(xs, pos)]++;
		    winsize++;
		}
	    }
	}
	
	ymax = (y0 < radius) ? y0 : radius;
	for(yy=1; yy<=ymax; ++yy)
	{
	    xmax = (x1 < struct_function[yy]) ? x1 : struct_function[yy];
	    for(xx=0; xx<=xmax; ++xx)
	    {
	        Diff2D pos(xx, -yy);
	        if(mask(xm, pos))
		{
		    hist[sa(xs, pos)]++;
		    winsize++;
		}
	    }
	}
    
	// find the desired histogramm bin 
	if(winsize) 
	{
	    if(rank == 0.0)
	    {
		for(i=0; i<256; i++)
		{
		    if(hist[i]) break;
		}
		rankpos = i;
	    }
	    else
	    {
		for(i=0; i<256; i++)
		{
		    if((float)(hist[i]+leftsum) / winsize >= rank) break;
		    leftsum += hist[i];
		}
		rankpos = i;
	    }
	    
	    da.set(rankpos, xd);
	}
	    
	++xs.x;
	++xd.x;
	++xm.x;
	
	// inner columns
	for(x=1; x<w; ++x, ++xs.x, ++xd.x, ++xm.x)
	{
	    x0 = x;
	    y0 = y;
	    x1 = w - x - 1;
	    y1 = h - y - 1;
	    
	    // update histogramm 
	    // remove pixels at left border 
	    yy = (y1 < radius) ? y1 : radius;
	    for(; yy>=0; yy--)
	    {
		unsigned char cur;
		xx = struct_function[yy]+1;
		if(xx > x0) break;
		
	        Diff2D pos(-xx, yy);
	        if(mask(xm, pos))
		{
		    cur = sa(xs, pos);
		    
		    hist[cur]--;
		    if(cur < rankpos) leftsum--;
		    winsize--;
		}
	    }
	    yy = (y0 < radius) ? y0 : radius;
	    for(; yy>=1; yy--)
	    {
		unsigned char cur;
		xx = struct_function[yy]+1;
		if(xx > x0) break;
		
	        Diff2D pos(-xx, -yy);
	        if(mask(xm, pos))
		{
		    cur = sa(xs, pos);
		    
		    hist[cur]--;
		    if(cur < rankpos) leftsum--;
		    winsize--;
		}
	    }
	    
	    // add pixels at right border 
	    yy = (y1 < radius) ? y1 : radius;
	    for(; yy>=0; yy--)
	    {
		unsigned char cur;
		xx = struct_function[yy];
		if(xx > x1) break;
		
	        Diff2D pos(xx, yy);
	        if(mask(xm, pos))
		{
		    cur = sa(xs, pos);
		    
		    hist[cur]++;
		    if(cur < rankpos) leftsum++;
		    winsize++;
		}
	    }
	    yy = (y0 < radius) ? y0 : radius;
	    for(; yy>=1; yy--)
	    {
		unsigned char cur;
		xx = struct_function[yy];
		if(xx > x1) break;
		
	        Diff2D pos(xx, -yy);
	        if(mask(xm, pos))
		{
		    cur = sa(xs, pos);
		    
		    hist[cur]++;
		    if(cur < rankpos) leftsum++;
		    winsize++;
		}
	    }
	
	    // find the desired histogramm bin 
	    if(winsize) 
	    {
		if(rank == 0.0)
		{
		    if(leftsum == 0)
		    {
			// search to the right 
			for(i=rankpos; i<256; i++)
			{
			    if(hist[i]) break;
			}
			rankpos = i;
		    }
		    else
		    {
			// search to the left 
			for(i=rankpos-1; i>=0; i--)
			{
			    leftsum -= hist[i];
			    if(leftsum == 0) break;
			}
			rankpos = i;
		    }
		}
		else  // rank > 0.0 
		{
		    if((float)leftsum / winsize < rank)
		    {
			// search to the right 
			for(i=rankpos; i<256; i++)
			{
			    if((float)(hist[i]+leftsum) / winsize >= rank) break;
			    leftsum+=hist[i];
			}
			rankpos = i;
		    }
		    else
		    {
			/// search to the left 
			for(i=rankpos-1; i>=0; i--)
			{
			    leftsum-=hist[i];
			    if((float)leftsum / winsize < rank) break;
			}
			rankpos = i;
		    }
		}
			
		da.set(rankpos, xd);
	    }
	    else
	    {
	        leftsum = 0;
		rankpos = 0;
	    }
	}
    }
}

template <class SrcIterator, class SrcAccessor,
          class MaskIterator, class MaskAccessor,
	  class DestIterator, class DestAccessor>
void
discRankOrderFilterWithMask(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                            pair<MaskIterator, MaskAccessor> mask,
			    pair<DestIterator, DestAccessor> dest,
			    int radius, float rank)
{
    discRankOrderFilterWithMask(src.first, src.second, src.third,
                        mask.first, mask.second,
                        dest.first, dest.second,
			radius, rank);
}

/********************************************************/
/*                                                      */
/*                 discErosionWithMask                  */
/*                                                      */
/********************************************************/

/** Apply erosion (minimum) filter with disc of given radius to image
    using a mask.
    This is an abbreviation vor the masked rank order filter with 
    rank = 0.0. See \Ref{discRankOrderFilterWithMask}() for more information.
    
    {\bf Declarations:}
    
    pass arguments explicitely:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class MaskIterator, class MaskAccessor,
	          class DestIterator, class DestAccessor>
        inline void 
        discErosionWithMask(SrcIterator upperleft1, 
			    SrcIterator lowerright1, SrcAccessor sa,
			    MaskIterator upperleftm, MaskAccessor mask,
			    DestIterator upperleft2, DestAccessor da,
			    int radius)
    }
    \end{verbatim}
    
    
    group arguments (use in conjuction with factory functions \Ref{srcRange}(),
    \Ref{mask}(), and \Ref{dest}()):
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class MaskIterator, class MaskAccessor,
	          class DestIterator, class DestAccessor>
        inline void 
        discErosionWithMask(triple<SrcIterator, SrcIterator, SrcAccessor> src,
			    pair<MaskIterator, MaskAccessor> mask,
			    pair<DestIterator, DestAccessor> dest,
			    int radius)
    }
    \end{verbatim}

    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class MaskIterator, class MaskAccessor,
	  class DestIterator, class DestAccessor>
inline void 
discErosionWithMask(SrcIterator upperleft1, 
		    SrcIterator lowerright1, SrcAccessor sa,
		    MaskIterator upperleftm, MaskAccessor mask,
		    DestIterator upperleft2, DestAccessor da,
		    int radius)
{
    vigra_precondition(radius >= 0, "discErosionWithMask(): Radius must be >= 0.");
    
    discRankOrderFilterWithMask(upperleft1, lowerright1, sa, 
                                upperleftm, mask,
                                upperleft2, da,
				radius, 0.0);
}

template <class SrcIterator, class SrcAccessor,
          class MaskIterator, class MaskAccessor,
	  class DestIterator, class DestAccessor>
inline void 
discErosionWithMask(triple<SrcIterator, SrcIterator, SrcAccessor> src,
		    pair<MaskIterator, MaskAccessor> mask,
		    pair<DestIterator, DestAccessor> dest,
		    int radius)
{
    vigra_precondition(radius >= 0, "discErosionWithMask(): Radius must be >= 0.");
    
    discRankOrderFilterWithMask(src.first, src.second, src.third,
                        mask.first, mask.second,
                        dest.first, dest.second,
			radius, 0.0);
}

/********************************************************/
/*                                                      */
/*                discDilationWithMask                  */
/*                                                      */
/********************************************************/

/** Apply dilation (maximum) filter with disc of given radius to image
    using a mask.
    This is an abbreviation vor the masked rank order filter with 
    rank = 1.0. See \Ref{discRankOrderFilterWithMask}() for more information.
    
    {\bf Declarations:}
    
    pass arguments explicitely:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class MaskIterator, class MaskAccessor,
	          class DestIterator, class DestAccessor>
        inline void 
        discDilationWithMask(SrcIterator upperleft1, 
			    SrcIterator lowerright1, SrcAccessor sa,
			    MaskIterator upperleftm, MaskAccessor mask,
			    DestIterator upperleft2, DestAccessor da,
			    int radius)
    }
    \end{verbatim}
    
    
    group arguments (use in conjuction with factory functions \Ref{srcRange}(),
    \Ref{mask}(), and \Ref{dest}()):
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class MaskIterator, class MaskAccessor,
	          class DestIterator, class DestAccessor>
        inline void 
        discDilationWithMask(triple<SrcIterator, SrcIterator, SrcAccessor> src,
			    pair<MaskIterator, MaskAccessor> mask,
			    pair<DestIterator, DestAccessor> dest,
			    int radius)
    }
    \end{verbatim}

    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class MaskIterator, class MaskAccessor,
	  class DestIterator, class DestAccessor>
inline void 
discDilationWithMask(SrcIterator upperleft1, 
		    SrcIterator lowerright1, SrcAccessor sa,
		    MaskIterator upperleftm, MaskAccessor mask,
		    DestIterator upperleft2, DestAccessor da,
		    int radius)
{
    vigra_precondition(radius >= 0, "discDilationWithMask(): Radius must be >= 0.");
    
    discRankOrderFilterWithMask(upperleft1, lowerright1, sa, 
                                upperleftm, mask,
                                upperleft2, da,
				radius, 1.0);
}

template <class SrcIterator, class SrcAccessor,
          class MaskIterator, class MaskAccessor,
	  class DestIterator, class DestAccessor>
inline void 
discDilationWithMask(triple<SrcIterator, SrcIterator, SrcAccessor> src,
		    pair<MaskIterator, MaskAccessor> mask,
		    pair<DestIterator, DestAccessor> dest,
		    int radius)
{
    vigra_precondition(radius >= 0, "discDilationWithMask(): Radius must be >= 0.");
    
    discRankOrderFilterWithMask(src.first, src.second, src.third,
                        mask.first, mask.second,
                        dest.first, dest.second,
			radius, 1.0);
}

/********************************************************/
/*                                                      */
/*                 discMedianWithMask                   */
/*                                                      */
/********************************************************/

/** Apply median filter with disc of given radius to image
    using a mask.
    This is an abbreviation vor the masked rank order filter with 
    rank = 0.5. See \Ref{discRankOrderFilterWithMask}() for more information.
    
    {\bf Declarations:}
    
    pass arguments explicitely:
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class MaskIterator, class MaskAccessor,
	          class DestIterator, class DestAccessor>
        inline void 
        discMedianWithMask(SrcIterator upperleft1, 
			    SrcIterator lowerright1, SrcAccessor sa,
			    MaskIterator upperleftm, MaskAccessor mask,
			    DestIterator upperleft2, DestAccessor da,
			    int radius)
    }
    \end{verbatim}
    
    
    group arguments (use in conjuction with factory functions \Ref{srcRange}(),
    \Ref{mask}(), and \Ref{dest}()):
    \begin{verbatim}
    namespace vigra {
        template <class SrcIterator, class SrcAccessor,
	          class MaskIterator, class MaskAccessor,
	          class DestIterator, class DestAccessor>
        inline void 
        discMedianWithMask(triple<SrcIterator, SrcIterator, SrcAccessor> src,
			    pair<MaskIterator, MaskAccessor> mask,
			    pair<DestIterator, DestAccessor> dest,
			    int radius)
    }
    \end{verbatim}

    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class MaskIterator, class MaskAccessor,
	  class DestIterator, class DestAccessor>
inline void 
discMedianWithMask(SrcIterator upperleft1, 
		    SrcIterator lowerright1, SrcAccessor sa,
		    MaskIterator upperleftm, MaskAccessor mask,
		    DestIterator upperleft2, DestAccessor da,
		    int radius)
{
    vigra_precondition(radius >= 0, "discMedianWithMask(): Radius must be >= 0.");
    
    discRankOrderFilterWithMask(upperleft1, lowerright1, sa, 
                                upperleftm, mask,
                                upperleft2, da,
				radius, 0.5);
}

template <class SrcIterator, class SrcAccessor,
          class MaskIterator, class MaskAccessor,
	  class DestIterator, class DestAccessor>
inline void 
discMedianWithMask(triple<SrcIterator, SrcIterator, SrcAccessor> src,
		    pair<MaskIterator, MaskAccessor> mask,
		    pair<DestIterator, DestAccessor> dest,
		    int radius)
{
    vigra_precondition(radius >= 0, "discMedianWithMask(): Radius must be >= 0.");
    
    discRankOrderFilterWithMask(src.first, src.second, src.third,
                        mask.first, mask.second,
                        dest.first, dest.second,
			radius, 0.5);
}

//@}

} // namespace vigra

#endif // VIGRA_FLATMORPHOLOGY_HXX
