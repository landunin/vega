/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * Released under the GNU General Public License
 *
 * Model.cpp
 *
 *  Created on: Jan 11, 2013
 *      Author: dallolio
 */

#include "Model.h"

#include <iostream>
#include <string>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>
#include <boost/unordered_map.hpp>
#include <ciso646>

using namespace std;

namespace vega {

Model::Model(string name, string inputSolverVersion, SolverName inputSolver,
        const ModelConfiguration configuration) :
        name(name), inputSolverVersion(inputSolverVersion), //
        inputSolver(inputSolver), //
        modelType(ModelType::TRIDIMENSIONAL_SI), //
        configuration(configuration), commonLoadSet(*this, LoadSet::ALL, LoadSet::COMMON_SET_ID),
		commonConstraintSet(*this, ConstraintSet::ALL, ConstraintSet::COMMON_SET_ID) {
    this->mesh = shared_ptr<Mesh>(new Mesh(configuration.logLevel, name));
    this->finished = false;
    this->onlyMesh = false;
    this->coordinateSystemStorage = shared_ptr<CoordinateSystemStorage>(new CoordinateSystemStorage(this, configuration.logLevel));

}

Model::~Model() {
}

template<class T>
void Model::Container<T>::add(const T& t) {
    shared_ptr<T> ptr = t.clone();
    add(ptr);
}

template<class T>
void Model::Container<T>::add(shared_ptr<T> ptr) {
    if (find(ptr->getReference())) {
        ostringstream oss;
        oss << *ptr << " is already in the model";
        throw runtime_error(oss.str());
    }
    by_id[ptr->getId()] = ptr;
    if (ptr->isOriginal())
        by_original_ids_by_type[ptr->type][ptr->getOriginalId()] = ptr;
}

template<class T>
void Model::Container<T>::erase(const Reference<T> ref) {
    by_id.erase(ref.id);
    if (ref.has_original_id())
        by_original_ids_by_type[ref.type].erase(ref.original_id);
}

/*
 * Redefining method add for Value to take into account placeHolder
 */
template<>
void Model::Container<Value>::add(const Value& t) {
    shared_ptr<Value> ptr = t.clone();
    if (shared_ptr<Value> ptr_old = find(t)) {
        if (ptr->isPlaceHolder()) { // TODO : make a merge function for placeHolder
            if (ptr->hasParaX())
                ptr_old->setParaX(ptr->getParaX());
            if (ptr_old->hasParaY())
                ptr_old->setParaY(ptr->getParaY());
            ptr = ptr_old;
        } else if (ptr_old->isPlaceHolder()) {
            if (ptr_old->hasParaX())
                ptr->setParaX(ptr_old->getParaX());
            if (ptr_old->hasParaY())
                ptr->setParaY(ptr_old->getParaY());
        } else {
            ostringstream oss;
            oss << t << " is already in the model";
            throw runtime_error(oss.str());
        }
    }
    if (!t.isPlaceHolder()) {
        by_id[t.getId()] = ptr;
    }
    if (t.isOriginal()) {
        by_original_ids_by_type[t.type][t.getOriginalId()] = ptr;
    }
}

template<class T>
shared_ptr<T> Model::Container<T>::find(const Reference<T>& reference) const {
    shared_ptr<T> t;
    if (reference.has_original_id()) {
        auto it = by_original_ids_by_type.find(reference.type);
        if (it != by_original_ids_by_type.end()) {
            auto it2 = it->second.find(reference.original_id);
            if (it2 != it->second.end()) {
                t = it2->second;
            }
        }
    } else if (reference.has_id()) {
        auto it = by_id.find(reference.id);
        if (it != by_id.end()) {
            t = it->second;
        }
    } else {
        assert(false);
    }

    return t;
}

template<class T>
shared_ptr<T> Model::Container<T>::find(int original_id) const {
    shared_ptr<T> t;
    for (auto it = by_original_ids_by_type.begin(); it != by_original_ids_by_type.end(); it++) {
        auto it2 = it->second.find(original_id);
        if (it2 != it->second.end()) {
            t = it2->second;
        }
    }
    return t;
}

template<class T>
shared_ptr<T> Model::Container<T>::get(int id) const {
    shared_ptr<T> t = nullptr;
    auto it = by_id.find(id);
    if (it != by_id.end()) {
    	t = it->second;
    }
    return t;
}



void Model::add(const Analysis& analysis) {
    if (configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Adding " << analysis << endl;
    }
    analyses.add(analysis);
}

void Model::add(const Loading& loading) {
    if (configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Adding " << loading << endl;
    }
    loadings.add(loading);
}

void Model::add(const LoadSet& loadSet) {
    if (configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Adding " << loadSet << endl;
    }
    loadSets.add(loadSet);
}

// This "add" function used shared_ptr because adding an object
// clone it, which does not keep the original id.
//TODO: same type of "add" than the other ?
void Model::add(const shared_ptr<Material> material) {
    if (configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Adding " << *material << endl;
    }
    materials.add(material);
}

void Model::add(const Constraint& constraint) {
    if (configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Adding " << constraint << endl;
    }
    constraints.add(constraint);
}

void Model::add(const ConstraintSet& constraintSet) {
    if (configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Adding " << constraintSet << endl;
    }
    constraintSets.add(constraintSet);
}

void Model::add(const Objective& objective) {
    if (configuration.logLevel >= LogLevel::TRACE) {
        cout << "Adding " << objective << endl;
    }
    objectives.add(objective);
}

void Model::add(const Value& value) {
    if (configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Adding " << value << endl;
    }
    values.add(value);
}

void Model::add(const CoordinateSystem& coordinateSystem) {
    if (configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Adding " << coordinateSystem << endl;
    }
    coordinateSystems.add(coordinateSystem);
    coordinateSystemStorage->add(coordinateSystem);
}

void Model::add(const ElementSet& elementSet) {
    if (configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Adding " << elementSet << endl;
    }
    elementSets.add(elementSet);
}

int Model::findOrReserveCoordinateSystem(int cid){
	if (cid == CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID)
		return CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID;
	int cpos = coordinateSystemStorage->findPositionByUserId(cid);
	if (cpos == CoordinateSystemStorage::UNAVAILABLE_POSITION)
		cpos = coordinateSystemStorage->reserve(cid);
	return cpos;
}


std::shared_ptr<Analysis> Model::getAnalysis(int id) const {
	return analyses.get(id);
}

std::shared_ptr<Loading> Model::getLoading(int id) const {
    return loadings.get(id);
}

std::shared_ptr<LoadSet> Model::getLoadSet(int id)  const {
    return loadSets.get(id);
}

std::shared_ptr<Constraint> Model::getConstraint(int id) const {
    return constraints.get(id);
}

std::shared_ptr<ConstraintSet> Model::getConstraintSet(int id) const {
    return constraintSets.get(id);
}

std::shared_ptr<Objective> Model::getObjective(int id) const {
    return objectives.get(id);
}

std::shared_ptr<Value> Model::getValue(int id) const {
    return values.get(id);
}

std::shared_ptr<CoordinateSystem> Model::getCoordinateSystem(int id) const {
    return coordinateSystems.get(id);
}

std::shared_ptr<ElementSet> Model::getElementSet(int id) const {
    return elementSets.get(id);
}

std::shared_ptr<Material> Model::getMaterial(int id) const {
    return materials.get(id);
}





int Model::addOrFindOrientation(const OrientationCoordinateSystem & ocs){

	int posOrientation = findOrientation(ocs);
	if (posOrientation==0){
		this->add(ocs);
		posOrientation = coordinateSystemStorage->findPositionById(ocs.getId());
	}
	return posOrientation;
}

int Model::findOrientation(const OrientationCoordinateSystem & ocs) const{
	int posOrientation=0;
	for (shared_ptr<CoordinateSystem> coordinateSystem : coordinateSystems) {
		if (coordinateSystem->type==CoordinateSystem::Type::ORIENTATION){
			std::shared_ptr<OrientationCoordinateSystem> mocs = std::static_pointer_cast<OrientationCoordinateSystem>(coordinateSystem);
			if (ocs == *mocs){
				posOrientation = coordinateSystemStorage->findPositionById(mocs->getId());
				break;
			}
		}
	}
	return posOrientation;
}

std::shared_ptr<vega::CoordinateSystem> Model::getCoordinateSystemByPosition(const int pos) const{
	const int cid =  coordinateSystemStorage->getId(pos);
	return getCoordinateSystem(cid);
}



const vector<int> Model::getMaterialsId() const{
	vector<int> v;
	for (auto material : materials){
		v.push_back(material->getId());
	}
	return v;
}

const vector<int> Model::getElementSetsId() const{
	vector<int> v;
	for (auto elementSet : elementSets){
		v.push_back(elementSet->getId());
	}
	return v;
}

template<>
void Model::remove(const Reference<Constraint> constraintReference) {
    for (auto it : constraintReferences_by_constraintSet_ids) {
        for (auto it2 : it.second) {
            if (*it2 == constraintReference)
                constraintReferences_by_constraintSet_ids[it.first].erase(it2);
        }
    }
    for (auto it : constraintReferences_by_constraintSet_original_ids_by_constraintSet_type) {
        for (auto it2 : it.second) {
            for (auto it3 : it2.second) {
                if (*it3 == constraintReference)
                    constraintReferences_by_constraintSet_original_ids_by_constraintSet_type[it.first][it2.first].erase(
                            it3);
            }
        }
    }
    constraints.erase(constraintReference);
}

void Model::remove(const Reference<Constraint> refC, const int idCS, const int originalIdCS, const ConstraintSet::Type csT) {

    const auto & cR = constraintReferences_by_constraintSet_ids[idCS];
    for (auto it2 : cR) {
        if (*it2 == refC){
            constraintReferences_by_constraintSet_ids[idCS].erase(it2);
        }
    }
    if (originalIdCS!= Identifiable<ConstraintSet>::NO_ORIGINAL_ID){
        const auto & cR2 = constraintReferences_by_constraintSet_original_ids_by_constraintSet_type[csT][originalIdCS];
        for (auto it3 : cR2) {
            if (*it3 == refC){
                constraintReferences_by_constraintSet_original_ids_by_constraintSet_type[csT][originalIdCS].erase(it3);
            }
        }
    }
    constraints.erase(refC);
}

template<>
void Model::remove(const Reference<Loading> loadingReference) {
    for (auto it : loadingReferences_by_loadSet_ids) {
        for (auto it2 : it.second) {
            if (*it2 == loadingReference)
                loadingReferences_by_loadSet_ids[it.first].erase(it2);
        }
    }
    for (auto it : loadingReferences_by_loadSet_original_ids_by_loadSet_type) {
        for (auto it2 : it.second) {
            for (auto it3 : it2.second) {
                if (*it3 == loadingReference)
                    loadingReferences_by_loadSet_original_ids_by_loadSet_type[it.first][it2.first].erase(
                            it3);
            }
        }
    }
    loadings.erase(loadingReference);
}

template<>
void Model::remove(const Reference<LoadSet> loadSetReference) {
    shared_ptr<LoadSet> loadSet = loadSets.find(loadSetReference);
    for (auto analysis : analyses) {
        if (analysis->contains(loadSetReference)) {
            if (configuration.logLevel >= LogLevel::DEBUG)
                cout << "Disassociating empty " << *loadSet << " from " << *analysis << endl;
            analysis->remove(loadSetReference);
        }
    }
    loadSets.erase(loadSetReference);
}

template<>
void Model::remove(const Reference<ConstraintSet> constraintSetReference) {
    shared_ptr<ConstraintSet> constraintSet = constraintSets.find(constraintSetReference);
    for (auto analysis : analyses) {
        if (analysis->contains(constraintSetReference)) {
            if (configuration.logLevel >= LogLevel::DEBUG)
                cout << "Disassociating empty " << *constraintSet << " from " << *analysis << endl;
            analysis->remove(constraintSetReference);
        }
    }
    constraintSets.erase(constraintSetReference);
}

template<>
void Model::remove(const Reference<Objective> objectiveReference) {
    shared_ptr<Objective> objective = objectives.find(objectiveReference);
    if (objective && objective->isAssertion()) {
        for (auto analysis : analyses) {
            if (analysis->contains(objectiveReference)) {
                if (configuration.logLevel >= LogLevel::TRACE) {
                    cout << "Disassociating dangling " << *objective << " from  " << *analysis
                            << endl;
                }
                analysis->remove(objectiveReference);
            }
        }
    }
    objectives.erase(objectiveReference);
}

template<>
const shared_ptr<Objective> Model::find(const Reference<Objective> reference) const {
    return objectives.find(reference);
}

template<>
const shared_ptr<Value> Model::find(const Reference<Value> reference) const {
    return values.find(reference);
}

template<>
const shared_ptr<Loading> Model::find(const Reference<Loading> reference) const {
    return loadings.find(reference);
}

template<>
const shared_ptr<LoadSet> Model::find(const Reference<LoadSet> reference) const {
    return loadSets.find(reference);
}

template<>
const shared_ptr<Constraint> Model::find(const Reference<Constraint> reference) const {
    return constraints.find(reference);
}

template<>
const shared_ptr<ConstraintSet> Model::find(const Reference<ConstraintSet> reference) const {
    return constraintSets.find(reference);
}

template<>
const shared_ptr<Analysis> Model::find(const Reference<Analysis> reference) const {
    shared_ptr<Analysis> analysis;
    if (reference.type == Analysis::UNKNOWN && reference.id == Reference<Analysis>::NO_ID)
        // retrieve by original_id
        analysis = analyses.find(reference.original_id);
    else
        analysis = analyses.find(reference);
    return analysis;
}

template<>
const shared_ptr<CoordinateSystem> Model::find(const Reference<CoordinateSystem> reference) const {
    shared_ptr<CoordinateSystem> coordinateSystem;
    if (reference.type == CoordinateSystem::UNKNOWN
            && reference.id == Reference<CoordinateSystem>::NO_ID)
        // retrieve by original_id
        coordinateSystem = coordinateSystems.find(reference.original_id);//PB?
    else
        coordinateSystem = coordinateSystems.find(reference);
    return coordinateSystem;
}

template<>
const shared_ptr<ElementSet> Model::find(const Reference<ElementSet> reference) const {
    shared_ptr<ElementSet> elementSet;
    if (reference.type == ElementSet::UNKNOWN && reference.id == Reference<ElementSet>::NO_ID)
        // retrieve by original_id
        elementSet = elementSets.find(reference.original_id);
    else
        elementSet = elementSets.find(reference);
    return elementSet;
}

void Model::addLoadingIntoLoadSet(const Reference<Loading>& loadingReference,
        const Reference<LoadSet>& loadSetReference) {
    shared_ptr<Reference<Loading>> loadingReference_ptr = loadingReference.clone();
    if (loadSetReference.has_id())
        loadingReferences_by_loadSet_ids[loadSetReference.id].insert(loadingReference_ptr);
    if (loadSetReference.has_original_id())
        loadingReferences_by_loadSet_original_ids_by_loadSet_type[loadSetReference.type][loadSetReference.original_id].insert(
                loadingReference_ptr);
    if (loadSetReference == commonLoadSet.getReference() && !find(commonLoadSet.getReference()))
        add(commonLoadSet); // commonLoadSet is added to the model if needed
    if (!this->find(loadSetReference)) {
        LoadSet loadSet(*this, loadSetReference.type, loadSetReference.original_id);
        this->add(loadSet);
    }
}

const set<shared_ptr<Loading>> Model::getLoadingsByLoadSet(
        const Reference<LoadSet>& loadSetReference) const {
    set<shared_ptr<Loading>> result;
    auto itm = loadingReferences_by_loadSet_ids.find(loadSetReference.id);
    if (itm != loadingReferences_by_loadSet_ids.end()) {
        for (auto itm2 : itm->second) {
            result.insert(find(*itm2));
        }
    }
    auto itm2 = loadingReferences_by_loadSet_original_ids_by_loadSet_type.find(
            loadSetReference.type);
    if (itm2 != loadingReferences_by_loadSet_original_ids_by_loadSet_type.end()) {
        auto itm3 = itm2->second.find(loadSetReference.original_id);
        if (itm3 != itm2->second.end()) {
            for (auto itm4 : itm3->second) {
                result.insert(find(*itm4));
            }
        }
    }
    return result;
}

void Model::addConstraintIntoConstraintSet(const Reference<Constraint>& constraintReference,
        const Reference<ConstraintSet>& constraintSetReference) {
    shared_ptr<Reference<Constraint>> constraintReference_ptr = constraintReference.clone();
    if (constraintSetReference.has_id())
        constraintReferences_by_constraintSet_ids[constraintSetReference.id].insert(
                constraintReference_ptr);
    if (constraintSetReference.has_original_id())
        constraintReferences_by_constraintSet_original_ids_by_constraintSet_type[constraintSetReference.type][constraintSetReference.original_id].insert(
                constraintReference_ptr);
    if (constraintSetReference == commonConstraintSet.getReference()
            && !find(commonConstraintSet.getReference()))
        add(commonConstraintSet); // commonConstraintSet is added to the model if needed
}

const set<shared_ptr<Constraint>> Model::getConstraintsByConstraintSet(
        const Reference<ConstraintSet>& constraintSetReference) const {
    set<shared_ptr<Constraint>> result;
    auto itm = constraintReferences_by_constraintSet_ids.find(constraintSetReference.id);
    if (itm != constraintReferences_by_constraintSet_ids.end()) {
        for (auto itm2 : itm->second) {
            result.insert(find(*itm2));
        }
    }
    auto itm2 = constraintReferences_by_constraintSet_original_ids_by_constraintSet_type.find(
            constraintSetReference.type);
    if (itm2 != constraintReferences_by_constraintSet_original_ids_by_constraintSet_type.end()) {
        auto itm3 = itm2->second.find(constraintSetReference.original_id);
        if (itm3 != itm2->second.end()) {
            for (auto itm4 : itm3->second) {
                result.insert(find(*itm4));
            }
        }
    }
    return result;
}

const set<shared_ptr<ConstraintSet>> Model::getConstraintSetsByConstraint(
        const Reference<Constraint>& constraintReference) const {
    set<shared_ptr<ConstraintSet>> result;
    for (const auto& it : constraintSets) {
        set<shared_ptr<Constraint>> constraints = getConstraintsByConstraintSet(it->getReference());
        bool found = false;
        for (auto& constraint : constraints) {
            if (constraint == nullptr) {
                throw logic_error("Missing constraint declared in constraintSet : " + to_str(*it));
            }
            if (constraint->getReference() == constraintReference) {
                found = true;
                break;
            }
        }
        if (found) {
            result.insert(it);
        }
    }
    return result;
}

const vector<shared_ptr<ConstraintSet>> Model::getActiveConstraintSets() const {
    vector<shared_ptr<ConstraintSet>> result;
    set<shared_ptr<ConstraintSet>> set;
    for (auto analyse : analyses) {
        for (auto constraintSet : analyse->getConstraintSets()) {
            if (set.find(constraintSet) == set.end()) {
                set.insert(constraintSet);
                result.push_back(constraintSet);
            }
        }
    }
    return result;
}

const vector<shared_ptr<LoadSet>> Model::getActiveLoadSets() const {
    vector<shared_ptr<LoadSet>> result;
    set<shared_ptr<LoadSet>> set;
    for (auto analyse : analyses) {
        for (auto loadSet : analyse->getLoadSets()) {
            if (set.find(loadSet) == set.end()) {
                set.insert(loadSet);
                result.push_back(loadSet);
            }
        }
    }
    return result;
}

const vector<shared_ptr<ConstraintSet>> Model::getCommonConstraintSets() const {
    vector<shared_ptr<ConstraintSet>> result;
    map<shared_ptr<ConstraintSet>, int> map;
    for (auto analyse : analyses) {
        for (auto constraintSet : analyse->getConstraintSets()) {
            map[constraintSet] += 1;
        }
    }
    for (auto constraintSet : constraintSets) {
        auto it = map.find(constraintSet);
        if (it != map.end() && it->second == analyses.size())
            result.push_back(constraintSet);
    }
    return result;
}

const vector<shared_ptr<LoadSet>> Model::getCommonLoadSets() const {
    vector<shared_ptr<LoadSet>> result;
    map<shared_ptr<LoadSet>, int> map;
    for (auto analyse : analyses) {
        for (auto loadSet : analyse->getLoadSets()) {
            map[loadSet] += 1;
        }
    }
    for (auto loadSet : loadSets) {
        auto it = map.find(loadSet);
        // DLOAD is never common
        if (it != map.end() && it->second == analyses.size() && loadSet->type != LoadSet::DLOAD)
            result.push_back(loadSet);
    }
    return result;
}

const set<shared_ptr<ConstraintSet>> Model::getUncommonConstraintSets() const {
    set<shared_ptr<ConstraintSet>> result;
    map<shared_ptr<ConstraintSet>, int> map;
    for (auto analyse : analyses) {
        for (auto constraintSet : analyse->getConstraintSets()) {
            map[constraintSet] += 1;
        }
    }
    for (auto constraintSet : constraintSets) {
        auto it = map.find(constraintSet);
        if (it != map.end() && it->second < analyses.size())
            result.insert(constraintSet);
    }
    return result;
}

const set<shared_ptr<LoadSet>> Model::getUncommonLoadSets() const {
    set<shared_ptr<LoadSet>> result;
    map<shared_ptr<LoadSet>, int> map;
    for (auto analyse : analyses) {
        for (auto loadSet : analyse->getLoadSets()) {
            map[loadSet] += 1;
        }
    }
    for (auto loadSet : loadSets) {
        auto it = map.find(loadSet);
        if ((it != map.end() && it->second < analyses.size()) && loadSet->type != LoadSet::DLOAD)
            result.insert(loadSet);
    }
    return result;
}

void Model::generateDiscrets() {

    //rigid constraints
    CellGroup* virtualDiscretTRGroup = nullptr;

    CellGroup* virtualDiscretTGroup = nullptr;

    for (Node node : this->mesh->nodes) {
        DOFS missingDOFS;

        for (auto& analysis : analyses) {

            DOFS requiredDOFS = analysis->findBoundaryDOFS(node.position);
            if (!node.dofs.containsAll(requiredDOFS)) {
                missingDOFS = missingDOFS + requiredDOFS - node.dofs;
            }
        }

        DOFS addedDOFS;
        if (missingDOFS.size() != 0) {
            if (missingDOFS.containsAnyOf(DOFS::ROTATIONS)) {
                //extra dofs added by the DISCRET. They need to be blocked.
                addedDOFS = DOFS::ALL_DOFS - node.dofs - missingDOFS;
                if (virtualDiscretTRGroup == nullptr) {
                    DiscretePoint virtualDiscretTR(*this, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
                    virtualDiscretTRGroup = mesh->createCellGroup("VDiscrTR");
                    virtualDiscretTR.assignCellGroup(virtualDiscretTRGroup);
                    virtualDiscretTR.assignMaterial(getVirtualMaterial());
                    this->add(virtualDiscretTR);
                }
                vector<int> cellNodes;
                cellNodes.push_back(node.id);
                mesh->allowDOFS(node.position, DOFS::ALL_DOFS);
                int cellPosition = mesh->addCell(Cell::AUTO_ID, CellType::POINT1, cellNodes,
                        true);
                virtualDiscretTRGroup->addCell(mesh->findCell(cellPosition).id);
            } else {
                addedDOFS = DOFS::TRANSLATIONS - node.dofs - missingDOFS;
                if (virtualDiscretTGroup == nullptr) {
                    DiscretePoint virtualDiscretT(*this, 0.0, 0.0, 0.0);
                    virtualDiscretTGroup = mesh->createCellGroup("VDiscrT");
                    virtualDiscretT.assignCellGroup(virtualDiscretTGroup);
                    virtualDiscretT.assignMaterial(getVirtualMaterial());
                    this->add(virtualDiscretT);
                }
                int cellPosition = mesh->addCell(Cell::AUTO_ID, CellType::POINT1, { node.id },
                        true);
                virtualDiscretTGroup->addCell(mesh->findCell(cellPosition).id);
                mesh->allowDOFS(node.position, DOFS::TRANSLATIONS);
            }
        }

        for (auto& analysis : analyses) {
            ConstraintSet* spcSet = nullptr;

            DOFS requiredDOFS = analysis->findBoundaryDOFS(node.position);
            if (!node.dofs.containsAll(requiredDOFS)) {
                DOFS extraDOFS = addedDOFS - requiredDOFS - node.dofs;

                if (extraDOFS != DOFS::NO_DOFS) {
                    if (spcSet == nullptr) {
                        spcSet = new ConstraintSet(*this, ConstraintSet::SPC);
                        add(*spcSet);
                    }
                    SinglePointConstraint spc = SinglePointConstraint(*this, extraDOFS);
                    spc.addNodeId(node.id);
                    add(spc);
                    addConstraintIntoConstraintSet(spc, *spcSet);
                    analysis->add(*spcSet);
                    if (configuration.logLevel >= LogLevel::DEBUG) {
                        cout << "Adding virtual spc on node: id: " << node.id << "for " << extraDOFS
                                << endl;
                    }
                }
            }
        }
    }
}

shared_ptr<Material> Model::getOrCreateMaterial(int material_id, bool createIfNotExists) {
    shared_ptr<Material> result = materials.find(material_id);
    if (!result && createIfNotExists) {
        result = shared_ptr<Material>(new Material(this, material_id));
        this->add(result);
    }
    return result;
}

const CellContainer Model::getMaterialAssignment(int materialId) const {
    auto it = material_assignment_by_material_id.find(materialId);
    if (it != material_assignment_by_material_id.end()) {
        return it->second;
    } else {
        //return empty cell container if no assigmnent is found
        return CellContainer(this->mesh);
    }
}

void Model::assignMaterial(int material_id, const CellContainer& materialAssign) {
    auto it = material_assignment_by_material_id.find(material_id);
    if (it != material_assignment_by_material_id.end()) {
        it->second.add(materialAssign);
    } else {
        material_assignment_by_material_id.insert(make_pair(material_id, materialAssign));
    }
}

shared_ptr<Material> Model::getVirtualMaterial() {
    if (!virtualMaterial) {
        virtualMaterial = this->getOrCreateMaterial(Material::NO_ORIGINAL_ID);
        virtualMaterial->addNature(ElasticNature(*this, 1e-12, 0.0));
    }
    return virtualMaterial;
}

const vector<shared_ptr<ElementSet>> Model::filterElements(ElementSet::Type type) const {
    vector<shared_ptr<ElementSet>> result;
    for (auto elementSet : elementSets) {
        if (elementSet->type == type && elementSet->validate())
            result.push_back(elementSet);
    }
    return result;
}

const vector<shared_ptr<Beam>> Model::getBeams() const {
    vector<shared_ptr<Beam>> result;
    for (auto elementSet : elementSets) {
        if (elementSet->isBeam()) {
            shared_ptr<Beam> beam = static_pointer_cast<Beam>(elementSet);
            result.push_back(beam);
        }
    }
    return result;
}


void Model::generateSkin() {
    for (Container<Loading>::iterator it = loadings.begin(); it != loadings.end(); it++) {
        shared_ptr<Loading> loadingPtr = *it;
        if (loadingPtr->applicationType == Loading::ELEMENT) {
            shared_ptr<ElementLoading> elementLoading = static_pointer_cast<ElementLoading>(
                    loadingPtr);
            if (elementLoading->cellDimensionGreatherThan(elementLoading->getLoadingDimension())) {
                switch (loadingPtr->type) {
                case Loading::FORCE_SURFACE: {
                    shared_ptr<ForceSurface> forceSurface = static_pointer_cast<ForceSurface>(
                            loadingPtr);
                    vector<int> faceIds = forceSurface->getApplicationFace();
                    if (faceIds.size() > 0) {
                        vega::Cell cell = generateSkinCell(faceIds, SpaceDimension::DIMENSION_2D);
                        // LD : Workaround for MED name problems, adding single cell groups
                        CellGroup* mappl = this->mesh->createCellGroup(
                                "C" + boost::lexical_cast<string>(cell.id));
                        mappl->addCell(cell.id);
                        forceSurface->clear();
                        forceSurface->add(*mappl);
                        // LD : Workaround for Aster problem : MODELISA6_96
                        //  les 1 mailles imprimées ci-dessus n'appartiennent pas au modèle et pourtant elles ont été affectées dans le mot-clé facteur : !
                        //   ! FORCE_FACE
                        Continuum continuum(*this, &ModelType::TRIDIMENSIONAL_SI);
                        continuum.assignCellGroup(mappl);
                        this->add(continuum);
                    }
                }
                    break;
                default:
                    throw logic_error("generate skin implemented only for pression face");
                }
            }
        }
    }

}

Cell Model::generateSkinCell(const vector<int>& faceIds, const SpaceDimension& dimension) {
    CellType* cellTypeFound = nullptr;
    for (auto typeAndCodePair : CellType::typeByCode) {
        CellType * typeToTest = typeAndCodePair.second;
        if (typeToTest->dimension == dimension && faceIds.size() == typeToTest->numNodes) {
            cellTypeFound = typeToTest;
            break;
        }
    }
    vector<int> externalFaceIds;
    if (cellTypeFound == nullptr) {
        throw logic_error(
                string("CellType not found connections:")
                        + boost::lexical_cast<string>(faceIds.size()));
    }
    int cellPosition = mesh->addCell(Cell::AUTO_ID, *cellTypeFound, faceIds, true);
    return mesh->findCell(cellPosition);
}

void Model::emulateLocalDisplacementConstraint() {
    boost::unordered_map<shared_ptr<Constraint>, set<LinearMultiplePointConstraint*>> linearMultiplePointConstraintsByConstraint;
    // first pass : create LinearMultiplePoint constraints for each constraint that need it
    for (auto it = constraints.begin(); it != constraints.end(); it++) {
        const shared_ptr<Constraint>& constraint = *it;
        if (constraint->type == Constraint::SPC) {
            shared_ptr<SinglePointConstraint> spc = static_pointer_cast<SinglePointConstraint>(
                    constraint);
            for (int nodePosition : spc->nodePositions()) {
                Node node = mesh->findNode(nodePosition, true, this);
                if (node.displacementCS != CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID) {
                    shared_ptr<CoordinateSystem> coordSystem = getCoordinateSystemByPosition(node.displacementCS);
                    coordSystem->updateLocalBase(VectorialValue(node.x, node.y, node.z));
                    DOFS dofs = constraint->getDOFSForNode(nodePosition);
                    for (int i = 0; i < 6; i++) {
                        vega::DOF currentDOF = *DOF::dofByPosition[i];
                        if (dofs.contains(currentDOF)) {
                            VectorialValue participation = coordSystem->vectorToGlobal(
                                    VectorialValue::XYZ[i % 3]);
                            LinearMultiplePointConstraint* lmpc =
                                    new LinearMultiplePointConstraint(*this,
                                            spc->getDoubleForDOF(currentDOF));
                            if (i < 3)
                                lmpc->addParticipation(node.id, participation.x(),
                                        participation.y(), participation.z());
                            else
                                lmpc->addParticipation(node.id, 0, 0, 0, participation.x(),
                                        participation.y(), participation.z());
                            linearMultiplePointConstraintsByConstraint[constraint].insert(lmpc);
                        }
                    }
                    constraint->removeNode(nodePosition);
                }
            }
        }
    }

    // second pass : insert the new LinearMultiplePointConstraints into the model and the constraintSets
    for (auto it : linearMultiplePointConstraintsByConstraint) {
        shared_ptr<Constraint> constraint = it.first;
        const set<shared_ptr<ConstraintSet>> constraintSets = getConstraintSetsByConstraint(
                constraint->getReference());
        for (auto linearMultiplePointConstraint : it.second) {
            add(*linearMultiplePointConstraint);
            for (auto constraintSet : constraintSets) {
                addConstraintIntoConstraintSet(linearMultiplePointConstraint->getReference(),
                        constraintSet->getReference());
            }
            delete linearMultiplePointConstraint;
        }
        if (constraint->nodePositions().size() == 0)
            remove(constraint->getReference());
    }
}

void Model::emulateAdditionalMass() {
    vector<shared_ptr<ElementSet>> newElementSets;
    for (auto elementSet : elementSets) {
        double rho = elementSet->getAdditionalRho();
        if (!is_zero(rho)) {
            // create new elementSet
            shared_ptr<ElementSet> newElementSet = elementSet->clone();
            newElementSet->resetId();
            newElementSets.push_back(newElementSet);
            // assign new material
            shared_ptr<Material> newMaterial(new Material(this));
            newMaterial->addNature(ElasticNature(*this, 0, 0, 0, rho));
            materials.add(newMaterial);
            newElementSet->assignMaterial(newMaterial);
            // copy and assign new cellGroup
            CellGroup* newCellGroup = mesh->createCellGroup(
                    "VAM_" + boost::lexical_cast<string>(newElementSets.size()));
            newElementSet->assignCellGroup(newCellGroup);
            vector<Cell> cells = elementSet->cellGroup->getCells();
            for (auto cell : cells) {
                int cellPosition = mesh->addCell(Cell::AUTO_ID, cell.type, cell.nodeIds, cell.isvirtual,
                        cell.cid, cell.elementId);
                newCellGroup->addCell(mesh->findCell(cellPosition).id);
            }
        }
    }
    for (auto elementSet : newElementSets)
        elementSets.add(elementSet);
}

void Model::generateBeamsToDisplayHomogeneousConstraint() {

    CellGroup* virtualGroupRigid = nullptr;
    CellGroup* virtualGroupRBE3 = nullptr;

    vector<shared_ptr<ConstraintSet>> activeConstraintSets = getActiveConstraintSets();
    for (auto constraintSet : activeConstraintSets) {
        set<shared_ptr<Constraint>> constraints = constraintSet->getConstraints();
        for (auto constraint : constraints) {
            switch (constraint->type) {
            case Constraint::RIGID: {
                if (!virtualGroupRigid) {
                    CircularSectionBeam virtualBeam(*this, 0.001, Beam::EULER, 0.0);
                    virtualBeam.assignMaterial(getVirtualMaterial());
                    virtualGroupRigid = mesh->createCellGroup("VRigid");
                    virtualBeam.assignCellGroup(virtualGroupRigid);
                    this->add(virtualBeam);
                }
                shared_ptr<RigidConstraint> rigid = static_pointer_cast<RigidConstraint>(
                        constraint);
                vector<int> nodes = { 0, 0 };
                nodes[0] = mesh->findNode(rigid->getMaster()).id;
                mesh->allowDOFS(rigid->getMaster(), DOFS::ALL_DOFS);
                for (int slaveNode : rigid->getSlaves()) {
                    nodes[1] = mesh->findNode(slaveNode).id;
                    int cellPosition = mesh->addCell(Cell::AUTO_ID, CellType::SEG2, nodes, true);
                    virtualGroupRigid->addCell(mesh->findCell(cellPosition).id);
                    mesh->allowDOFS(slaveNode, DOFS::ALL_DOFS);
                }
                break;
            }
            case Constraint::RBE3: {
                if (!virtualGroupRBE3) {
                    CircularSectionBeam virtualBeam(*this, 0.001, Beam::EULER, 0.0);
                    virtualBeam.assignMaterial(getVirtualMaterial());
                    virtualGroupRBE3 = mesh->createCellGroup("VRBE3");
                    virtualBeam.assignCellGroup(virtualGroupRBE3);
                    this->add(virtualBeam);
                }
                shared_ptr<RBE3> rbe3 = static_pointer_cast<RBE3>(constraint);
                vector<int> nodes = { 0, 0 };
                nodes[0] = mesh->findNode(rbe3->getMaster()).id;
                mesh->allowDOFS(rbe3->getMaster(), DOFS::ALL_DOFS);
                for (int slaveNode : rbe3->getSlaves()) {
                    nodes[1] = mesh->findNode(slaveNode).id;
                    int cellPosition = mesh->addCell(Cell::AUTO_ID, CellType::SEG2, nodes, true);
                    mesh->allowDOFS(slaveNode, DOFS::ALL_DOFS);
                    virtualGroupRBE3->addCell(mesh->findCell(cellPosition).id);
                }
                break;
            }
            default: {
            }
            }
        }
    }
}

void Model::generateMaterialAssignments() {
    if (configuration.partitionModel) {
        if (this->material_assignment_by_material_id.size() > 0) {
            cerr << "generateMaterialAssignments with PartitionModel is not "
                    << " yet implemented. " << endl
                    << "This method should partition the elementSets"
                    << " and assign materials to elementSets. Useful if output=Nastran" << endl;
        }
    } else {
        //generate or update material assignments from elementSets
        for (shared_ptr<ElementSet> element : elementSets) {
            if (element->material) {
                int mat_id = element->material->getId();
                auto it = material_assignment_by_material_id.find(mat_id);
                if (element->cellGroup != nullptr) {
                    if (it != material_assignment_by_material_id.end()) {
                        it->second.add(*(element->cellGroup));
                    } else {
                        CellContainer assignment(this->mesh);
                        assignment.add(*(element->cellGroup));
                        material_assignment_by_material_id.insert(make_pair(mat_id, assignment));
                    }
                }
            }
        }
    }
}

void Model::removeIneffectives() {
    // remove ineffective loadings from the model
    vector<shared_ptr<Loading>> loadingsToRemove;
    for (auto loading : loadings) {
        if (loading->ineffective())
            loadingsToRemove.push_back(loading);
    }
    for (auto loading : loadingsToRemove) {
        if (configuration.logLevel >= LogLevel::DEBUG)
            cout << "Removed ineffective " << *loading << endl;
        remove(Reference<Loading>(*loading));
    }

    // remove empty loadSets from the model
    vector<Reference<LoadSet>> loadSetSetsToRemove;
    for (auto loadSet : this->loadSets) {
        if (loadSet->size() == 0) {
            loadSetSetsToRemove.push_back(loadSet->getReference());
        }
    }
    for (Reference<LoadSet> loadSetRef : loadSetSetsToRemove) {
        if (configuration.logLevel >= LogLevel::DEBUG)
            cout << "Removed empty loadset " << loadSetRef.id << endl;
        remove(loadSetRef);
    }

    // remove ineffective constraints from the model
    vector<shared_ptr<Constraint>> constraintsToRemove;
    for (auto constraint : constraints) {
        if (constraint->ineffective())
            constraintsToRemove.push_back(constraint);
    }
    for (auto constraint : constraintsToRemove) {
        if (configuration.logLevel >= LogLevel::DEBUG)
            cout << "Removed ineffective " << *constraint << endl;
        remove(Reference<Constraint>(*constraint));
    }

    // remove empty constraintSets from the model
    vector<Reference<ConstraintSet>> constraintSetSetsToRemove;
    for (auto constraintSet : this->constraintSets) {
        if (constraintSet->size() == 0) {
            constraintSetSetsToRemove.push_back(constraintSet->getReference());
        }
    }
    for (Reference<ConstraintSet> constraintSetRef : constraintSetSetsToRemove) {
        if (configuration.logLevel >= LogLevel::DEBUG)
            cout << "Removed empty " << constraintSetRef.id << endl;
        remove(constraintSetRef);
    }

    // remove empty elementSets from the model
    vector<shared_ptr<ElementSet>> elementSetsToRemove;
    for (auto elementSet : elementSets) {
        if (elementSet->cellGroup && elementSet->cellGroup->cellIds.size() == 0)
            elementSetsToRemove.push_back(elementSet);
    }
    for (auto elementSet : elementSetsToRemove) {
        if (configuration.logLevel >= LogLevel::DEBUG)
            cout << "Removed empty " << *elementSet << endl;
        this->elementSets.erase(Reference<ElementSet>(*elementSet));
    }
}

void Model::replaceCombinedLoadSets() {
    for (auto loadSet : this->loadSets) {
        for (auto& kv : loadSet->embedded_loadsets) {
            shared_ptr<LoadSet> otherloadSet = this->find(kv.first);
            if (!otherloadSet) {
                cerr << "Missing loadSet " << to_string(kv.first.id) << endl;
            }
            double coefficient = kv.second;
            for (shared_ptr<Loading> loading : otherloadSet->getLoadings()) {
                shared_ptr<Loading> newLoading = loading->clone();
                newLoading->resetId();
                newLoading->scale(coefficient);
                this->add(*newLoading);
                this->addLoadingIntoLoadSet(*newLoading, *loadSet);
                if (configuration.logLevel >= LogLevel::DEBUG) {
                    cout << "Cloned " << *loading << " into " << *newLoading << " and scaled by "
                            << coefficient << " and assigned to " << *loadSet << endl;
                }
            }
        }
        loadSet->embedded_loadsets.clear();
    }
}

void Model::removeAssertionsMissingDOFS()
{
    vector<shared_ptr<Objective> > objectivesToRemove;
    for (auto& analysis : analyses) {
        for(auto& assertion : analysis->getAssertions()) {
            for(int nodePosition: assertion->nodePositions()) {
                DOFS assertionDOFS = assertion->getDOFSForNode(nodePosition);
                if (assertionDOFS.size() >= 1) {
                    Node node = mesh->findNode(nodePosition);
                    DOFS availableDOFS = node.dofs + analysis->findBoundaryDOFS(nodePosition);
                    if (!availableDOFS.containsAll(assertionDOFS)) {
                        objectivesToRemove.push_back(assertion);
                    }
                }
            }
        }
    }
    for (auto objective : objectivesToRemove) {
        if (configuration.logLevel >= LogLevel::TRACE)
            cout << "Removed ineffective " << *objective << endl;

        remove(Reference<Objective>(*objective));
    }
}

void Model::addDefaultAnalysis()
{
    if (this->analyses.size() == 0 && (loadings.size() > 0 || constraints.size() > 0)) {
        //add an automatic linear analysis
        LinearMecaStat analysis(*this, "", 1);
        add(analysis);
        if (this->configuration.logLevel >= LogLevel::DEBUG) {
            cout << "Default linear analysis added." << endl;
        }
    }
}

void Model::replaceDirectMatrices()
{
    vector<shared_ptr<ElementSet> > elementSetsToRemove;
    int matrix_count = 0;
    map<int, DOFS> addedDofsByNode;
    map<int, DOFS> requiredDofsByNode;
    map<int, DOFS> ownedDofsByNode;
    for (auto elementSetM : elementSets) {
        if (!elementSetM->isMatrixElement()) {
            continue;
        }
        shared_ptr<MatrixElement> matrix = static_pointer_cast<MatrixElement>(elementSetM);
        for (int nodePosition : matrix->nodePositions()) {
            requiredDofsByNode[nodePosition] = DOFS();
            Node node = mesh->findNode(nodePosition);
            DOFS owned;
            for (const auto elementSetI : elementSets) {
                if (elementSetI->cellGroup == nullptr) {
                    continue;
                }
                for (Cell cell : elementSetI->cellGroup->getCells()) {
                    for (int nodeId : cell.nodeIds) {
                        if (nodeId == node.id) {
                            if (elementSetI->isBeam() or elementSetI->isShell()) {
                                owned += DOFS::ALL_DOFS;
                            } else {
                                owned += DOFS::TRANSLATIONS;
                            }
                            break;
                        }
                    }
                }
            }
            ownedDofsByNode[nodePosition] = owned;
        }
        for (auto pair : matrix->nodePairs()) {
            if (pair.first == pair.second) {
                if (matrix->findInPairs(pair.first).size() != 0) {
                    continue; // will be handled by a segment cell with another node
                }
                // single node
                int nodePosition = pair.first;
                Node node = mesh->findNode(nodePosition);
                DOFS requiredDofs = requiredDofsByNode.find(nodePosition)->second;
                shared_ptr<DOFMatrix> submatrix = matrix->findSubmatrix(nodePosition, nodePosition);
                DiscretePoint discrete(*this, {});
                for (auto& kv : submatrix->componentByDofs) {
                    double value = kv.second;
                    const vega::DOF dof1 = kv.first.first;
                    const vega::DOF dof2 = kv.first.second;
                    if (!is_equal(value, 0)) {
                        switch (matrix->type) {
                        case ElementSet::STIFFNESS_MATRIX:
                            discrete.addStiffness(dof1, dof2, value);
                            break;
                        default:
                            throw logic_error("Not yet implemented");
                        }
                        requiredDofs += dof1;
                        requiredDofs += dof2;
                    }
                }
                discrete.assignMaterial(getVirtualMaterial());
                matrix_count++;
                CellGroup* matrixGroup = mesh->createCellGroup(
                        "MTN" + to_string(matrix_count));
                discrete.assignCellGroup(matrixGroup);
                int cellPosition = mesh->addCell(Cell::AUTO_ID, CellType::SEG2, { node.id }, true);
                matrixGroup->addCell(mesh->findCell(cellPosition).id);
                if (discrete.hasRotations()) {
                    addedDofsByNode[nodePosition] = DOFS::ALL_DOFS;
                    mesh->allowDOFS(node.position, DOFS::ALL_DOFS);
                } else {
                    addedDofsByNode[nodePosition] = DOFS::TRANSLATIONS;
                    mesh->allowDOFS(node.position, DOFS::TRANSLATIONS);
                }
                if (this->configuration.logLevel >= LogLevel::DEBUG) {
                    cout << "Creating discrete : " << discrete << " over node id : "
                            << to_string(node.id) << endl;
                }
                this->add(discrete);
            } else {
                // node couple
                Node rowNode = mesh->findNode(pair.first);
                Node colNode = mesh->findNode(pair.second);
                DOFS requiredRowDofs = requiredDofsByNode.find(rowNode.position)->second;
                DOFS requiredColDofs = requiredDofsByNode.find(colNode.position)->second;

                DiscreteSegment discrete(*this);
                CellGroup* matrixGroup = mesh->createCellGroup(
                        "MTL" + to_string(matrix_count));
                matrix_count++;
                int cellPosition = mesh->addCell(Cell::AUTO_ID, CellType::SEG2, { rowNode.id,
                        colNode.id }, true);
                matrixGroup->addCell(mesh->findCell(cellPosition).id);
                discrete.assignMaterial(getVirtualMaterial());
                discrete.assignCellGroup(matrixGroup);
                if (discrete.hasRotations()) {
                    addedDofsByNode[rowNode.position] = DOFS::ALL_DOFS;
                    mesh->allowDOFS(rowNode.position, DOFS::ALL_DOFS);
                    addedDofsByNode[colNode.position] = DOFS::ALL_DOFS;
                    mesh->allowDOFS(colNode.position, DOFS::ALL_DOFS);
                } else {
                    addedDofsByNode[rowNode.position] = DOFS::TRANSLATIONS;
                    mesh->allowDOFS(rowNode.position, DOFS::TRANSLATIONS);
                    addedDofsByNode[colNode.position] = DOFS::TRANSLATIONS;
                    mesh->allowDOFS(colNode.position, DOFS::TRANSLATIONS);
                }
                for (int row_index = 0; row_index < 2; ++row_index) {
                    for (int col_index = 0; col_index < 2; ++col_index) {
                        int rowNodePosition;
                        int colNodePosition;
                        if (row_index == 0) {
                            rowNodePosition = rowNode.position;
                        } else {
                            rowNodePosition = colNode.position;
                        }
                        if (col_index == 0) {
                            colNodePosition = rowNode.position;
                        } else {
                            colNodePosition = colNode.position;
                        }
                        shared_ptr<DOFMatrix> submatrix = matrix->findSubmatrix(rowNodePosition,
                                colNodePosition);
                        for (auto& kv : submatrix->componentByDofs) {
                            // We are disassembling the matrix, so we must divide the value by the segments
                            double value = kv.second / static_cast<int>(matrix->findInPairs(pair.first).size());
                            const DOF rowDof = kv.first.first;
                            const DOF colDof = kv.first.second;
                            if (!is_equal(value, 0)) {
                                switch (matrix->type) {
                                case ElementSet::STIFFNESS_MATRIX:
                                    discrete.addStiffness(row_index, col_index, rowDof, colDof,
                                            value);
                                    break;
                                default:
                                    throw logic_error("Not yet implemented");
                                }
                                requiredRowDofs += rowDof;
                                requiredColDofs += colDof;
                            }
                        }
                    }
                }
                if (this->configuration.logLevel >= LogLevel::DEBUG) {
                    cout << "Creating discrete : " << discrete << " over node ids : "
                            << to_string(rowNode.id) << " and : " << to_string(colNode.id) << endl;
                }
                this->add(discrete);
            }
        }
        elementSetsToRemove.push_back(elementSetM);
    }
    for (auto& kv : addedDofsByNode) {
        int nodePosition = kv.first;
        Node node = this->mesh->findNode(nodePosition);
        DOFS added = kv.second;
        DOFS required;
        auto it = requiredDofsByNode.find(nodePosition);
        if (it == requiredDofsByNode.end()) {
            required = DOFS::NO_DOFS;
        } else {
            required = it->second;
        }

        DOFS owned;
        auto it2 = ownedDofsByNode.find(nodePosition);
        if (it2 == ownedDofsByNode.end()) {
            owned = DOFS::NO_DOFS;
        } else {
            owned = it2->second;
        }

        for (const auto loading : loadings) {
            for (int nodePosition : loading->nodePositions()) {
                required += loading->getDOFSForNode(nodePosition);
            }
        }
        for (const auto constraint : constraints) {
            set<int> constraintNodes = constraint->nodePositions();
            if (constraintNodes.find(nodePosition) == constraintNodes.end()) {
                continue;
            }
            required += constraint->getDOFSForNode(nodePosition);
        }
        DOFS extra = added - owned - required;
        if (extra != DOFS::NO_DOFS) {
            SinglePointConstraint spc = SinglePointConstraint(*this, extra);
            spc.addNodeId(node.id);
            add(spc);
            addConstraintIntoConstraintSet(spc, commonConstraintSet);
            if (configuration.logLevel >= LogLevel::DEBUG) {
                cout << "Adding virtual spc on node id: " << node.id << "for " << extra
                        << endl;
            }
        }
    }
    for (auto elementSet : elementSetsToRemove) {
        if (configuration.logLevel >= LogLevel::DEBUG) {
            cout << "Replaced " << *elementSet << endl;
        }
        this->elementSets.erase(elementSet->getReference());
    }
}

void Model::removeRedundantSpcs()
{
    for (auto analysis : this->analyses) {
        std::unordered_map<std::pair<int, DOF>, double, boost::hash<std::pair<int, int> > > spcvalueByNodeAndDof;
        for (const auto& constraintSet : analysis->getConstraintSets()) {
            const set<shared_ptr<Constraint> > spcs = constraintSet->getConstraintsByType(
                    Constraint::SPC);
            if (spcs.size() == 0) {
                continue;
            }
            for (shared_ptr<Constraint> constraint : spcs) {
                shared_ptr<SinglePointConstraint> spc = static_pointer_cast<SinglePointConstraint>(
                        constraint);
                for (int nodePosition : spc->nodePositions()) {
                    DOFS dofsToRemove;
                    DOFS blockedDofs = spc->getDOFSForNode(nodePosition);
                    for (const DOF dof : blockedDofs) {
                        pair<int, DOF> key = make_pair(nodePosition, dof);
                        double spcValue = spc->getDoubleForDOF(dof);
                        auto entry = spcvalueByNodeAndDof.find(key);
                        if (entry == spcvalueByNodeAndDof.end()) {
                            spcvalueByNodeAndDof[key] = spcValue;
                        } else if (!is_equal(spcValue, entry->second)) {
                            Node node = this->mesh->findNode(nodePosition);
                            throw logic_error(
                                    "In analysis : " + to_str(*analysis) + ", spc : " + to_str(*spc)
                                            + " value : " + to_string(spcValue)
                                            + " different by other spc value : "
                                            + to_string(entry->second) + " on same node id : "
                                            + to_string(node.id) + " and dof : " + dof.label);
                        } else {
                            dofsToRemove = dofsToRemove + dof;
                        }
                    }
                    if (dofsToRemove.size() >= 1) {
                        analysis->removeSPCNodeDofs(*spc, nodePosition, dofsToRemove);
                        if (configuration.logLevel >= LogLevel::DEBUG) {
                            cout << "Removed redundant node position : " << nodePosition
                                    << " from spc : " << *spc << " for analysis : " << *analysis << endl;
                        }
                    }
                }
            }
        }
    }
}


void Model::splitDirectMatrices(const unsigned int sizeMax){


    if (sizeMax<2){
        throw logic_error("Model can't split matrices to a size under 2.");
    }

    vector<shared_ptr<ElementSet>> esToErase;
    vector<shared_ptr<ElementSet>> esToAdd;
    const int sizeStack = (int)sizeMax/2;

    for (auto elementSetM : elementSets) {
        if (!elementSetM->isMatrixElement()) {
            //TODO: Display informative message in debug mode.
            continue;
        }

        shared_ptr<MatrixElement> matrix = static_pointer_cast<MatrixElement>(elementSetM);

        // If matrix is small enough, we do nothing
        if (matrix->nodePositions().size()<=sizeMax){
            //TODO: Display informative in debug mode.
            continue;
        }

        // It's too big, we have work to do
        esToErase.push_back(elementSetM);
        map<int, int> nodeIdOfElement;
        for (auto & v : matrix->nodePositions()){
            nodeIdOfElement[v] = mesh->findNode(v).id;
        }

        // Dummy ElementSet for the splitting
        shared_ptr<ElementSet> dummyElement = elementSetM->clone();
        shared_ptr<MatrixElement> dummyMatrix = static_pointer_cast<MatrixElement>(dummyElement);
        dummyMatrix->clear();


        map<int,int> stackOfNodesByNodes;
        map<pair<int, int>, shared_ptr<ElementSet>>  esToAddByStackNumber;

        // Splitting the matrices, pairs of nodes by pairs of node (I,J).
        for (const auto np : matrix->nodePairs()){

            // We attribute a stack of sizeStack to each node.
            int sI, sJ;
            auto it = stackOfNodesByNodes.find(np.first);
            if (it == stackOfNodesByNodes.end()){
                sI = (int)stackOfNodesByNodes.size()/sizeStack;
                stackOfNodesByNodes[np.first]=sI;
            }else{
                sI = it->second;
            }
            it = stackOfNodesByNodes.find(np.second);
            if (it == stackOfNodesByNodes.end()){
                sJ = (int)stackOfNodesByNodes.size()/sizeStack;
                stackOfNodesByNodes[np.second]=sJ;
            }else{
                sJ = it->second;
            }

            // We attribute a elementSet to the pair (sI, sJ), and create it if needed
            pair<int, int> ps;
            if (sI<sJ){
                ps = make_pair(sI,sJ);
            }else{
                ps = make_pair(sJ,sI);
            }
            auto it2 = esToAddByStackNumber.find(ps);
            shared_ptr<ElementSet> newElementSet = nullptr;
            if (it2 == esToAddByStackNumber.end()){

                newElementSet = dummyElement->clone();
                newElementSet->resetId();
                esToAdd.push_back(newElementSet);
                esToAddByStackNumber[ps]=newElementSet;

                // Special add for the submatrices on the diagonal:
                // One submatrix corresponds to 3 pairs of stacks id
                int stF = ps.first - ps.first%2;
                int stS = ps.second - ps.second%2;
                if (stF == stS){
                    esToAddByStackNumber[make_pair(stF,stF)]=newElementSet;
                    esToAddByStackNumber[make_pair(stF,stF+1)]=newElementSet;
                    esToAddByStackNumber[make_pair(stF+1,stF+1)]=newElementSet;
                }
            }else{
                newElementSet = it2->second;
            }

            // We copy the values
            shared_ptr<MatrixElement> nM = static_pointer_cast<MatrixElement>(newElementSet);
            shared_ptr<DOFMatrix> dM = matrix->findSubmatrix(np.first, np.second);
            for (const auto dof: dM->componentByDofs){
                nM->addComponent(nodeIdOfElement[np.first], dof.first.first, nodeIdOfElement[np.second], dof.first.second, dof.second);
            }

        }

        if (configuration.logLevel >= LogLevel::DEBUG) {
            cout << "Element Matrix "<<matrix->bestId()<< " has been split into the smaller matrices ";
            for (auto es : esToAddByStackNumber){
                cout << es.second->bestId()<<" ";
            }
            cout <<endl;
        }

    }

    // We remove the elementSets corresponding to big matrices
    for (auto es : esToErase){
        this->elementSets.erase(*es);
    }

    // We add the elementSets corresponding to small matrices
    for (auto es : esToAdd){
        this->add(*es);
    }
}



void Model::makeCellsFromDirectMatrices(){

    int idM=0;
    for (auto elementSetM : elementSets) {
        if (!elementSetM->isMatrixElement()) {
            //TODO: Display informative message in debug mode.
            continue;
        }
        // If cells already exists, we do nothing
        if (elementSetM->cellGroup != nullptr){
            //TODO: Display informative message in debug mode.
            continue;
        }

        shared_ptr<MatrixElement> matrix = static_pointer_cast<MatrixElement>(elementSetM);

        // If matrix is void, we do nothing (should not happen)
        if (matrix->nodePositions().size()==0){
            //TODO: Display informative message in debug mode.
            continue;
        }

        // Create a Cell Group
        idM++;
        CellGroup* matrixGroup = mesh->createCellGroup("DM" + to_string(idM), Group::NO_ORIGINAL_ID, "Direct Matrix "+ elementSetM->name);
        matrix->assignCellGroup(matrixGroup);

        // Create a Cell and add it to the Cell Group
        CellType cellType = CellType::POINT1;
        // VERY TEDIOUS, because cells can't be of a variable size, for now :/
        //TODO: Add a version with "variable sized" cells.
        switch (matrix->nodePositions().size()){
        case 1:{
            cellType = CellType::POINT1;
            break;
        }
        case 2:{
            cellType = CellType::SEG2;
            break;
        }
        case 3:{
            cellType = CellType::POLY3;
            break;
        }
        case 4:{
            cellType = CellType::POLY4;
            break;
        }
        case 5:{
            cellType = CellType::POLY5;
            break;
        }
        case 6:{
            cellType = CellType::POLY6;
            break;
        }
        case 7:{
            cellType = CellType::POLY7;
            break;
        }
        case 8:{
            cellType = CellType::POLY8;
            break;
        }
        case 9:{
            cellType = CellType::POLY9;
            break;
        }
        case 10:{
            cellType = CellType::POLY10;
            break;
        }
        case 11:{
            cellType = CellType::POLY11;
            break;
        }
        case 12:{
            cellType = CellType::POLY12;
            break;
        }
        case 13:{
            cellType = CellType::POLY13;
            break;
        }
        case 14:{
            cellType = CellType::POLY14;
            break;
        }
        case 15:{
            cellType = CellType::POLY15;
            break;
        }
        case 16:{
            cellType = CellType::POLY16;
            break;
        }
        case 17:{
            cellType = CellType::POLY17;
            break;
        }
        case 18:{
            cellType = CellType::POLY18;
            break;
        }
        case 19:{
            cellType = CellType::POLY19;
            break;
        }
        case 20:{
            cellType = CellType::POLY20;
            break;
        }
        default:{
            //TODO: Don't work for now, because elements can't be of a variable size, for now :/
            //cellType = CellType::POLYHED;
            throw logic_error("Element size exceed the maximum size : 20.");
        }
        }
        vector<int> vNodeIds;
        for (int nodePosition : matrix->nodePositions()){
            Node node = mesh->findNode(nodePosition);
            vNodeIds.push_back(node.id);
        }

        int cellPosition = mesh->addCell(Cell::AUTO_ID, cellType, vNodeIds, true);
        matrixGroup->addCell(mesh->findCell(cellPosition).id);
        
        if (configuration.logLevel >= LogLevel::DEBUG){
           cout << "Built cells, in cellgroup "<<matrixGroup->getName()<<", for Matrix Elements in "<< elementSetM->name<<"."<<endl;
        }
        
    }
}


void Model::makeCellsFromRBE(){

    shared_ptr<Mesh> mesh = this->mesh;
        vector<shared_ptr<ConstraintSet>> commonConstraintSets = this->getCommonConstraintSets();

    for (const auto& constraintSet : commonConstraintSets) {

        const int idConstraintSet = constraintSet->getId();
        const int originalIdConstraintSet = constraintSet->getOriginalId();
        const ConstraintSet::Type natConstraintSet = constraintSet->type;


        // Translation of RBAR and RBE2 (RBE2 are viewed as an assembly of RBAR)
        // See Systus Reference Analysis Manual: RIGID BODY Element (page 498)
        set<shared_ptr<Constraint>> constraints = constraintSet->getConstraintsByType(Constraint::RIGID);
        for (const auto& constraint : constraints) {
            const std::shared_ptr<RigidConstraint> rbe2 = std::static_pointer_cast<RigidConstraint>(constraint);

            // Creating an elementset, a CellGroup and a dummy rigid material
            shared_ptr<Material> materialRBE2(new Material(this));
            materialRBE2->addNature(RigidNature(*this, 1));
            this->add(materialRBE2);

            CellGroup* group = mesh->createCellGroup("RBE2_"+std::to_string(constraint->getOriginalId()), CellGroup::NO_ORIGINAL_ID, "RBE2");
            Rbar elementsetRbe2(*this, mesh->findNode(rbe2->getMaster()).id);
            elementsetRbe2.assignCellGroup(group);
            elementsetRbe2.assignMaterial(materialRBE2);
            this->add(elementsetRbe2);

            // Creating cells and adding them to the CellGroup
            Node master = mesh->findNode(rbe2->getMaster());
            for (int position : rbe2->getSlaves()){
                Node slave = mesh->findNode(position);
                vector<int> nodes = {master.id, slave.id};
                int cellPosition = mesh->addCell(Cell::AUTO_ID, CellType::SEG2, nodes, true);
                group->addCell(mesh->findCell(cellPosition).id);
            }

            // Removing the constraint from the model.
            remove(constraint->getReference(), idConstraintSet, originalIdConstraintSet, natConstraintSet);
            if (configuration.logLevel >= LogLevel::DEBUG){
                cout << "Building cells in cellgroup "<<group->getName()<<" from "<< *rbe2<<"."<<endl;
            }
        }


        constraints = constraintSet->getConstraintsByType(Constraint::QUASI_RIGID);
        for (const auto& constraint : constraints) {
            std::shared_ptr<QuasiRigidConstraint> rbar = std::static_pointer_cast<QuasiRigidConstraint>(constraint);

            if (!(rbar->isCompletelyRigid())){
                cerr << "QUASI_RIDID constraint not available yet. Constraint "+std::to_string(constraint->bestId())+ " translated as rigid constraint."<<endl;
            }
            if (rbar->getSlaves().size()!=2){
               throw logic_error("QUASI_RIDID constraint must have exactly two slaves.");
            }

            // Creating an elementset, a CellGroup and a dummy rigid material
            shared_ptr<Material> materialRBAR(new Material(this));
            materialRBAR->addNature(RigidNature(*this, 1));
            this->add(materialRBAR);

            // Master Node : first one. Slave Node : second and last one
            Node masterNode = mesh->findNode(*rbar->getSlaves().begin());
            Node slaveNode = mesh->findNode(*rbar->getSlaves().rbegin());
            CellGroup* group = mesh->createCellGroup("RBAR_"+std::to_string(constraint->getOriginalId()), CellGroup::NO_ORIGINAL_ID, "RBAR");

            Rbar elementsetRBAR(*this, masterNode.id);
            elementsetRBAR.assignCellGroup(group);
            elementsetRBAR.assignMaterial(materialRBAR);
            this->add(elementsetRBAR);

            // Creating a cell and adding it to the CellGroup
            vector<int> nodes = {masterNode.id, slaveNode.id};
            int cellPosition = mesh->addCell(Cell::AUTO_ID, CellType::SEG2, nodes, true);
            group->addCell(mesh->findCell(cellPosition).id);

            // Removing the constraint from the model.
            remove(constraint->getReference(), idConstraintSet, originalIdConstraintSet, natConstraintSet);
            if (configuration.logLevel >= LogLevel::DEBUG){
                cout << "Building cells in cellgroup "<<group->getName()<<" from "<< *rbar<<"."<<endl;
            }
        }


        /* Translation of RBE3
         * See Systus Reference Analysis Manual, Section 8.8 "Special Elements",
         * Subsection "Use of Averaging Type Solid Elements", p500.
         */
        constraints = constraintSet->getConstraintsByType(Constraint::RBE3);
        for (const auto& constraint : constraints) {

            const std::shared_ptr<RBE3> rbe3 = std::static_pointer_cast<RBE3>(constraint);
            Node master = mesh->findNode(rbe3->getMaster());
            const DOFS mDOFS = rbe3->getDOFS();

            int nbParts=0;
            map<DOFS, map<double, CellGroup*>> groupByCoefByDOFS;

            for (int position : rbe3->getSlaves()){

                Node slave = mesh->findNode(position);
                vector<int> nodes = {master.id, slave.id};
                int cellPosition = mesh->addCell(Cell::AUTO_ID, CellType::SEG2, nodes, true);

                /* We build a material for each value of "Slave DOFS" and "Slave Coeff" */
                const DOFS sDOFS = rbe3->getDOFSForNode(position);
                const double sCoef = rbe3->getCoefForNode(position);

                CellGroup* groupRBE3 = nullptr;
                auto it = groupByCoefByDOFS.find(sDOFS);
                if (it !=groupByCoefByDOFS.end()){
                    auto it2 = it->second.find(sCoef);
                    if (it2 != it->second.end())
                        groupRBE3 = it2->second;
                }

                if (groupRBE3==nullptr){

                    // Creating an elementset, a CellGroup and a dummy rigid material
                    nbParts++;
                    shared_ptr<Material> materialRBE3(new Material(this));
                    materialRBE3->addNature(RigidNature(*this, Nature::UNAVAILABLE_DOUBLE, sCoef));
                    this->add(materialRBE3);

                    CellGroup* group = mesh->createCellGroup("RBE3_"+to_string(nbParts)+"_"+to_string(constraint->getOriginalId()), CellGroup::NO_ORIGINAL_ID, "RBE3");
                    Rbe3 elementsetRbe3(*this, master.id, mDOFS, sDOFS);
                    elementsetRbe3.assignCellGroup(group);
                    elementsetRbe3.assignMaterial(materialRBE3);
                    this->add(elementsetRbe3);

                    if (configuration.logLevel >= LogLevel::DEBUG){
                        cout << "Building cells in CellGroup "<<group->getName()<<" from "<< *rbe3<<"."<<endl;
                    }
                    groupByCoefByDOFS[sDOFS][sCoef]= group;
                }
                groupRBE3=groupByCoefByDOFS[sDOFS][sCoef];
                groupRBE3->addCell(mesh->findCell(cellPosition).id);
            }

            // Removing the constraint from the model.
            remove(constraint->getReference(), idConstraintSet, originalIdConstraintSet, natConstraintSet);
        }
    }
}

void Model::splitElementsByDOFS(){

    std::vector<ScalarSpring> elementSetsToAdd;
    std::vector<shared_ptr<ElementSet>> elementSetsToRemove;

    for (auto & elementSet : elementSets) {

        switch (elementSet->type){
        case ElementSet::DISCRETE_0D:
        case ElementSet::DISCRETE_1D:
        case ElementSet::NODAL_MASS:
        case ElementSet::CIRCULAR_SECTION_BEAM:
        case ElementSet::RECTANGULAR_SECTION_BEAM:
        case ElementSet::I_SECTION_BEAM:
        case ElementSet::GENERIC_SECTION_BEAM:
        case ElementSet::STRUCTURAL_SEGMENT:
        case ElementSet::SHELL:
        case ElementSet::CONTINUUM:
        case ElementSet::STIFFNESS_MATRIX:
        case ElementSet::MASS_MATRIX:
        case ElementSet::DAMPING_MATRIX:
        case ElementSet::RIGIDSET:
        case ElementSet::RBAR:
        case ElementSet::RBE3:{
            continue;
        }
        
        case ElementSet::SCALAR_SPRING:{
            shared_ptr<ScalarSpring> ss = static_pointer_cast<ScalarSpring>(elementSet);
            if (ss->getNbDOFSSpring()>1){
                int i =1;
                const double stiffness = ss->getStiffness();
                const double damping = ss->getDamping();
                const string name = ss->cellGroup->getName();
                const string comment = ss->cellGroup->getComment();
                if (configuration.logLevel >= LogLevel::DEBUG)
                    cout<< *elementSet << " spring must be split."<<endl;
                for (const auto & it : ss->getCellPositionByDOFS()){
                    ScalarSpring scalarSpring(*this, Identifiable<ElementSet>::NO_ORIGINAL_ID, stiffness, damping);
                    CellGroup* cellGroup = this->mesh->createCellGroup(name+"_"+std::to_string(i), Group::NO_ORIGINAL_ID, comment);
                    scalarSpring.assignCellGroup(cellGroup);
                    for (const int cellPosition : it.second){
                        scalarSpring.addSpring(cellPosition, it.first.first, it.first.second);
                        cellGroup->addCell(this->mesh->findCell(cellPosition).id);
                    }
                    elementSetsToAdd.push_back(scalarSpring);
                    i++;
                }
                elementSetsToRemove.push_back(elementSet);
                this->mesh->removeGroup(name);
            }
            break;
        }
        default: {
            //TODO : throw ModelException("ElementSet type not supported");
            cerr << "Warning in splitElementsByDOFS: " << *elementSet << " not supported" << endl;
        }
        }
    }

    for (auto elementSet : elementSetsToRemove){
        this->elementSets.erase(Reference<ElementSet>(*elementSet));
    }
    for (auto & elementSet : elementSetsToAdd){
         this->add(elementSet);
        }
}

void Model::finish() {
    if (finished) {
        return;
    }

    /* Build the coordinate systems from their definition points */
    for (shared_ptr<CoordinateSystem> coordinateSystem : coordinateSystems) {
        coordinateSystem->build();
    }

    for (shared_ptr<ElementSet> elementSet : elementSets) {
        for (int nodePosition : elementSet->nodePositions()) {
            mesh->allowDOFS(nodePosition,elementSet->getDOFSForNode(nodePosition));
        }
    }
    for (shared_ptr<Analysis> analysis : analyses) {
        for (const auto& boundaryCondition : analysis->getBoundaryConditions()) {
            for(int nodePosition: boundaryCondition->nodePositions()) {
                analysis->addBoundaryDOFS(nodePosition,
                        boundaryCondition->getDOFSForNode(nodePosition));
            }
        }
    }

    removeAssertionsMissingDOFS();

    if (this->configuration.emulateLocalDisplacement) {
        emulateLocalDisplacementConstraint();
    }

    if (this->configuration.displayHomogeneousConstraint) {
        generateBeamsToDisplayHomogeneousConstraint();
    }

    if (this->configuration.createSkin) {
        generateSkin();
    }
    if (this->configuration.emulateAdditionalMass) {
        emulateAdditionalMass();
    }

    if (this->configuration.replaceCombinedLoadSets) {
        replaceCombinedLoadSets();
    }

    if (this->configuration.replaceDirectMatrices) {
        replaceDirectMatrices();
    }

    if (this->configuration.removeRedundantSpcs) {
        removeRedundantSpcs();
    }

    if (this->configuration.removeIneffectives) {
        removeIneffectives();
    }

    if (this->configuration.virtualDiscrets) {
        generateDiscrets();
    }

    if (this->configuration.splitDirectMatrices){
        splitDirectMatrices(this->configuration.sizeDirectMatrices);
    }

    if (this->configuration.makeCellsFromDirectMatrices){
        makeCellsFromDirectMatrices();
    }

    if (this->configuration.makeCellsFromRBE){
        makeCellsFromRBE();
    }

    if (this->configuration.splitElementsByDOFS){
        splitElementsByDOFS();
        }

    assignElementsToCells();
    generateMaterialAssignments();
    addDefaultAnalysis();

    this->mesh->finish();
    finished = true;
}

bool Model::validate() {
    bool meshValid = mesh->validate();

    // Sizes are stocked now, because validation remove invalid objects.
    int sizeMat = materials.size();     string sMat = ( (sizeMat > 1) ? "s are " : " is ");
    int sizeEle = elementSets.size();   string sEle = ( (sizeEle > 1) ? "s are " : " is ");
    int sizeLoa = loadings.size();      string sLoa = ( (sizeLoa > 1) ? "s are " : " is ");
    int sizeLos = loadSets.size();      string sLos = ( (sizeLos > 1) ? "s are " : " is ");
    int sizeCon = constraints.size();   string sCon = ( (sizeCon > 1) ? "s are " : " is ");
    int sizeCos = constraintSets.size();string sCos = ( (sizeCos > 1) ? "s are " : " is ");
    int sizeAna = analyses.size();      string sAna = ( (sizeAna > 1) ? "s are " : " is "); 

    bool validMat = materials.validate();
    bool validEle = elementSets.validate();
    bool validLoa = loadings.validate();
    bool validLos = loadSets.validate();
    bool validCon = constraints.validate();
    bool validCos = constraintSets.validate();
    bool validAna = analyses.validate();

    if (configuration.logLevel >= LogLevel::DEBUG) {
       cout << "The " << sizeMat << " material"     << sMat << (validMat ? "" : "NOT ") << "valid." << endl;
       cout << "The " << sizeEle << " elementSet"   << sEle << (validEle ? "" : "NOT ") << "valid." << endl;
       cout << "The " << sizeLoa << " loading"      << sLoa << (validLoa ? "" : "NOT ") << "valid." << endl;
       cout << "The " << sizeLos << " loadSet"      << sLos << (validLos ? "" : "NOT ") << "valid." << endl;
       cout << "The " << sizeCon << " constraint"   << sCon << (validCon ? "" : "NOT ") << "valid." << endl;
       cout << "The " << sizeCos << " constraintSet"<< sCos << (validCos ? "" : "NOT ") << "valid." << endl;
       cout << "The " << sizeAna << " analyze"      << sAna << (validAna ? "" : "NOT ") << "valid." << endl;
    }
    bool allValid = meshValid && validMat && validEle && validLoa && validLos && validCon
            && validCos && validAna;
    this->afterValidation = true;
    return allValid;
}

void Model::assignElementsToCells() {
    for (shared_ptr<ElementSet> element : elementSets) {
        if (element->cellGroup != nullptr) {
            CellContainer container(mesh);
            container.add(*element->cellGroup);
            mesh->assignElementId(container, element->getId());
        }
    }
}

} /* namespace vega */
