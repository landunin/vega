/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * This file is part of Vega.
 *
 *   Vega is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Vega is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Vega.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  SystusAsc.h
 *
 *  Created on: 28/03/2017
 *      Author: Thomas Abballe
 *     Comment: Regroups classes that modelize parts of the Systus ASC
 *              format, in order to simplify the code in SystusWriter
 */

#ifndef SYSTUSASC_H_
#define SYSTUSASC_H_

#include <map>
#include <vector>
#include <string>
#include <iostream>


namespace vega {

enum SystusTableLabel{
    TL_STANDARD, TL_DIRECT, TL_FILE, TL_FORTRAN, TL_PLASTICITY, TL_PROGRAM, TL_TABLE
};

static const std::map<SystusTableLabel, std::string> stringSystusTableLabel = {
        {SystusTableLabel::TL_STANDARD, "STANDARD"}, {SystusTableLabel::TL_DIRECT, "DIRECT"}, {SystusTableLabel::TL_FILE, "FILE"},
        {SystusTableLabel::TL_FORTRAN, "FORTRAN"}, {SystusTableLabel::TL_PLASTICITY, "PLASTICITY"},
        {SystusTableLabel::TL_PROGRAM, "PROGRAM"}, {SystusTableLabel::TL_TABLE, "TABLE"}
};

std::string SystusTableLabelToString(const SystusTableLabel stl);

/**
 * Generic class for a Systus Table. Only the STANDARD type is really supported, all the others are unused and untested
 */
class SystusTable{
public:
    long unsigned int id;

    SystusTableLabel label;
    long unsigned int type;
    std::vector<double> values;

    SystusTable(long unsigned int id, SystusTableLabel label=SystusTableLabel::TL_STANDARD, long unsigned int type=1);
    virtual ~SystusTable();

    void add(double value);
    friend std::ostream &operator<<(std::ostream &out, const SystusTable& st);

};


/**
 * Modelizes a Systus Matrix (stiffness or mass). They are used by elements X9XX type 0.
 */
class SystusMatrix{
public:
    
    long unsigned int id; /**< Id. Correspond to a "E id" in the material, or "REDUCTION id" in the reduction process.>**/
    int nbDOFS;			  
    int nbNodes;
    int size;
    std::vector<double> values;

    SystusMatrix(long unsigned int id, int nbNodes, int nbDOFS);
    virtual ~SystusMatrix();

    void setValue(int i, int j, int dofi, int dofj, double value);
    /**
     * Print a SystusMatrix to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &out, const SystusMatrix& sm);

};



/**
 * Modelizes a Systus Matrix (stiffness or mass). They are used by elements X9XX type 0.
 */
class SystusMatrices{
public:

    std::vector<SystusMatrix> matrices;
    int nbDOFS;

    SystusMatrices();
    virtual ~SystusMatrices();

    void add(SystusMatrix sm);
    void clear();
    long unsigned int size();
    /**
     * Print SystusMatrices to the output stream, in a ASCII format.
     * To be used by SYSTUS, output file must be translated to BINARY format, using the filematrix tool.
     */
    friend std::ostream &operator<<(std::ostream &out, const SystusMatrices& sms);

};



}
#endif /* SYSTUSASC_H_ */
