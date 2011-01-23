#ifndef _TORCH5SPRO_STUMP_MACHINE_H_
#define _TORCH5SPRO_STUMP_MACHINE_H_

#include "machine/Machine.h"
#include "machine/Machines.h"

namespace Torch {


	//////////////////////////////////////////////////////////////////////////////////////
	// Torch::StumpMachine:
	//      Process some input using a model (loaded from some file).
	//      The output is a DoubleTensor!
	//
	//      NB: The ouput should be allocated and deallocated by each Machine implementation!
	//
	//	EACH MACHINE SHOULD REGISTER
	//		==> MachineManager::GetInstance().add(new XXXMachine) <==
	//	TO THE MACHINEMANAGER CLASS!!!
	//
	// TODO: doxygen header!
	//////////////////////////////////////////////////////////////////////////////////////

	class StumpMachine : public Machine
	{

	public:

		/// Constructor
		StumpMachine();

		/// Destructor
		virtual ~StumpMachine();

		/// Process the input tensor
		virtual bool 		forward(const Tensor& input);

		// Loading/Saving the content from files (<em>not the options</em>) - overriden
		virtual bool		loadFile(File& file);
		virtual bool		saveFile(File& file) const;

		/// Constructs an empty Machine of this kind
		// (used by <MachineManager>, this object is automatically deallocated)
		virtual Machine*	getAnInstance() const { return manage(new StumpMachine()); }

		// Get the ID specific to each Machine
		virtual int		getID() const { return STUMP_MACHINE_ID; }

		///////////////////////////////////////////////////////////
		// Access functions

		void 			setParams(int direction_, float threshold_);

		///////////////////////////////////////////////////////////

	protected:

		///////////////////////////////////////////////////////////////
		// Attributes

		// feature
		//int feature_id;

		// parameters of the machine
		float threshold;
		int direction;
		bool verbose;
	};

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // REGISTER this machine to the <MachineManager>
        const bool stump_machine_registered = MachineManager::getInstance().add(
                new StumpMachine(), "StumpMachine");
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

#endif