/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * Released under the GNU General Public License
 *
 * AsterBuilder.h
 *
 *  Created on: 5 mars 2013
 *      Author: dallolio
 */

#ifndef ASTERBUILDER_H_
#define ASTERBUILDER_H_

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <boost/filesystem.hpp>
#include "AsterModel.h"
#include "../Abstract/Model.h"
#include "../Abstract/SolverInterfaces.h"
#include "../Abstract/ConfigurationParameters.h"

namespace vega {
namespace aster {

class AsterWriterImpl : public Writer{
	string mail_name, sigm_noeu, sigm_elno, sief_elga;
	bool calc_sigm = false;
	static constexpr const double SMALLEST_RELATIVE_COMPARISON = 1e-7;

	void writeExport(AsterModel& model, std::ostream&);
	void writeComm(const AsterModel& model, std::ostream&);
	void writeLireMaillage(const AsterModel&, std::ostream&);
	void writeAffeModele(const AsterModel&, std::ostream&);
	void writeValues(const AsterModel&, std::ostream&);
	void writeMaterials(const AsterModel&, std::ostream&);
	void writeAffeCaraElem(const AsterModel&, std::ostream&);
	void writeAffeCaraElemPoutre(const ElementSet&, std::ostream&);
	void writeAffeCharMeca(const AsterModel&, std::ostream&);
	void writeDefiContact(const AsterModel&, std::ostream&);
	void writeSPC(const AsterModel&, const ConstraintSet&, std::ostream&);
	void writeLIAISON_SOLIDE(const AsterModel&, const ConstraintSet&, std::ostream&);
	void writeRBE3(const AsterModel&, const ConstraintSet&, std::ostream&);
	void writeLMPC(const AsterModel&, const ConstraintSet&, std::ostream&);
	void writeGravity(const LoadSet&, std::ostream&);
	void writeRotation(const LoadSet&, std::ostream&);
	void writeNodalForce(const LoadSet&, std::ostream&);
	void writePression(const LoadSet&, std::ostream&);
	void writeForceCoque(const LoadSet&, std::ostream&);
	void writeForceLine(const LoadSet&, std::ostream&);
	void writeForceSurface(const LoadSet&, std::ostream&);
	void writeCellContainer(const CellContainer& cellContainer, ostream&);
	double writeAnalysis(const AsterModel&, Analysis& analysis, std::ostream&, double debut);
	void writeNodalDisplacementAssertion(const AsterModel&, Assertion&, std::ostream&);
	void writeNodalComplexDisplacementAssertion(const AsterModel&, Assertion&, std::ostream&);
	void writeFrequencyAssertion(Assertion&, std::ostream&);
	void writeLoadset(LoadSet& loadSet, std::ostream& out);
	string writeValue(Value& value, std::ostream& out);
	void writeImprResultats(const AsterModel& asterModel, std::ostream& out);
	void list_concept_name(StepRange& stepRange);
	std::shared_ptr<NonLinearStrategy> getNonLinearStrategy(NonLinearMecaStat& nonLinAnalysis);
	void writeAnalyses(const AsterModel& asterModel, std::ostream& out);

public:
	AsterWriterImpl();
	virtual ~AsterWriterImpl();
	string writeModel(const std::shared_ptr<Model> model_ptr, const ConfigurationParameters&);
private:
	//TODO: That's a dummy function to make the compiler happy. Do you want to write a real one?
	virtual std::string toString() {return "";};

};

}
}
#endif /* ASTERBUILDER_H_ */
