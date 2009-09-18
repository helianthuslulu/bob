#ifndef _TORCH5SPRO_IP_LBP_H_
#define _TORCH5SPRO_IP_LBP_H_

#include "ipCore.h"		// <ipLBP> is an <ipCore>
#include "Tensor.h"
namespace Torch
{
	/////////////////////////////////////////////////////////////////////////
	// Torch::ipLBP
	//	This class computes the LBP code at a given location in the image.
	//	The input tensor can be a 2D/3D tensor of any type.
	//	The result is a 1D IntTensor with a single value (the LBP code).
	//      For 3D tensors only the first plane is used.
	//
	//	- PARAMETERS (name, type, default value, description):
	//		"ToAverage"	bool	false	"compute the LBP code to the average"
	//		"AddAvgBit"	bool	false	"add the center bit to the LBP code (considering the average)"
	//		"Uniform"	bool	false	"uniform patterns (at most two bitwise 0-1 or 1-0 transitions)"
	//		"RotInvariant"	bool	false	"rotation invariant patterns"
	//
	// TODO: doxygen header!
	/////////////////////////////////////////////////////////////////////////

	class ipLBP : public ipCore
	{
	public:

		// Constructor
		ipLBP(int P, int R = 1);

		// Destructor
		virtual ~ipLBP();

		// Set the LBP location
		bool			setXY(int x, int y);

		// Set the radius value of the LBP operator
		virtual bool		setR(int R);

		/// Change the region of the input tensor to process - overriden
		virtual void		setRegion(const TensorRegion& region);

		/// Change the model size (if used with some machine) - overriden
		virtual void		setModelSize(const TensorSize& modelSize);

		/////////////////////////////////////////////
		// Access functions

		virtual int		getMaxLabel() = 0;
		int			getR() { return m_R; };
		int			getLBP() const { return *m_lbp; }
		int			getX() const { return m_x; }
		int			getY() const { return m_y; }

		/////////////////////////////////////////////////////////////////

	protected:

		/////////////////////////////////////////////////////////////////

		/// Check if the input tensor has the right dimensions and type - overriden
		virtual bool		checkInput(const Tensor& input) const;

		/// Allocate (if needed) the output tensors given the input tensor dimensions - overriden
		virtual bool		allocateOutput(const Tensor& input);

		/// called when some option was changed - overriden
		virtual void		optionChanged(const char* name);

		// Bilinear interpolation (for different tensor types)
		char			bilinear_interpolation(	const char* src,
								int stride_w, int stride_h,
								float x, float y);
		short			bilinear_interpolation(	const short* src,
								int stride_w, int stride_h,
								float x, float y);
                int			bilinear_interpolation(	const int* src,
								int stride_w, int stride_h,
								float x, float y);
		long			bilinear_interpolation(	const long* src,
								int stride_w, int stride_h,
								float x, float y);
		float			bilinear_interpolation(	const float* src,
								int stride_w, int stride_h,
								float x, float y);
		double			bilinear_interpolation(	const double* src,
								int stride_w, int stride_h,
								float x, float y);

		/////////////////////////////////////////////////////////////////////////
		// IntegralFactors:
		//	- Singleton that stores the scalling factors for some
		//		(model size, subwindow size) pair
		//		AND SHARED them accross multiple ipLBP instances
		// NB: Used for speeding up the scanning process!
		/////////////////////////////////////////////////////////////////////////

		class IntegralFactors
		{
		public:
			// Constructor
			IntegralFactors()
				:	m_model_w(0), m_model_h(0),
					m_sw_w(0), m_sw_h(0),
					m_dx(0), m_dy(0),
					m_cell_w(0), m_cell_w1(0), m_cell_w12(0),
					m_cell_h(0), m_cell_h1(0), m_cell_h12(0)
			{
			}

			// Resize to a new model/subwindow size
			void			resizeModel(int model_w, int model_h);
			void			resizeSW(int sw_w, int sw_h, int stride_w, int stride_h,
							int mask_x, int mask_y, int mask_radius);

			// Access functions
			const int&		getDx() const { return m_dx; }
			const int&		getDy() const { return m_dy; }
			const int&		getCellW() const { return m_cell_w; }
			const int&		getCellW1() const { return m_cell_w1; }
			const int&		getCellW12() const { return m_cell_w12; }
			const int&		getCellH() const { return m_cell_h; }
			const int&		getCellH1() const { return m_cell_h1; }
			const int&		getCellH12() const { return m_cell_h12; }

		private:

//				     <----------------->
//				       w1     w2    w1
//			   	     <-----><---><----->
//			               w12
//			   	     <----------->
//
//				P1 o +-----+-----+-----+ o P4		|			|
//				     |  P2 |     | P3  |		|			|
//				     |     |     |     |		| h1			|
//				     |     |     |     |		|			| h12
//				P5 o +-----+-----+-----+ o P8		|			|
//				     |  P6 |     | P7  |			|		|
//				     |     |     |     |			| h2 		|
//				     |     |     |     |			|		|
//				P9 o +-----+-----+-----+ o P12			|		|
//				     | P10 |     | P11 |
//				     |     |     |     |
//				     |     |     |     |
//				P13o +-----+-----+-----+ o P16
//				     	P14        P15


			// Attributes
			int			m_model_w, m_model_h;		// Model size
			int			m_sw_w, m_sw_h;			// Subwindow size
			int			m_dx, m_dy;			// Displacement from the subwindow top-left corner
			int			m_cell_w, m_cell_w1, m_cell_w12;// As in the figure above
			int			m_cell_h, m_cell_h1, m_cell_h12;// As in the figure above
		};

		/////////////////////////////////////////////////////////////////

	protected:

		/////////////////////////////////////////////////////////////////
		// Attributes

		// LBP operator parameters
		int			m_P, m_R;

		// LBP operator location
		int			m_x, m_y;

		// Input tensor size (to pre-compute the scalling factors)
		int			m_input_w, m_input_h;
		int			m_input_stride_w, m_input_stride_h;

		// Direct (&fast) access to the LBP code
		int*			m_lbp;

		// Conversion tables (to label uniform & rotation invariant,
		//	adding the average bit or normal LBP patterns)
		unsigned short*		m_lut_RI;
		unsigned short*		m_lut_U2;
		unsigned short*		m_lut_U2RI;
		unsigned short*		m_lut_addAvgBit;	// 2 ^ (P + 1)
		unsigned short*		m_lut_normal;		// 2 ^ P

		// Current selected conversion table (for fast accessing)
		unsigned short*		m_crt_lut;

		// Different parameters for computing LBPs (not using <getOption> to speed up computation)
		bool			m_toAverage;
		bool			m_addAvgBit;
		bool			m_uniform;
		bool			m_rot_invariant;

		// Indicate if the model size is different than the subwindow size -> needing interpolation
		bool			m_need_interp;

		// Integral image scalling factors
		IntegralFactors		m_ii_factors;
	};
}

#endif



